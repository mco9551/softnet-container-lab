// SPDX-License-Identifier: GPL-2.0
// E6 - DNS Response Latency Monitor
// Author: Comparetto Matthieu - matricola 0383422
// Basic level: timestamp outgoing DNS queries via tc egress hook

#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

#define DNS_PORT   53
#define DNS_QR_BIT 0x8000u

// DNS header as it appears in the packet
struct dnshdr {
    __u16 txn_id;   // unique transaction identifier
    __u16 flags;    // contains QR bit (query=0 / response=1)
    __u16 qdcount;  // Number of questions in Question section
    __u16 ancount;  // Number of answers in the Answer section
    __u16 nscount;  // Number of ressource in Authority section
    __u16 arcount;  // Number of ressource in Additional section
};


// INTERMEDIATE: hash map storing query timestamps
// key   = transaction ID (u16)
// value = timestamp in nanoseconds (u64)
// survives between eBPF executions unlike local variables
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key,   __u16);
    __type(value, __u64);
    __uint(max_entries, 1024);
} dns_timestamps SEC(".maps");

// BASIC : egress hook
// Called on every outgoing packet
// Basic    : logs timestamp to trace_pipe
// Entry point: called by the TC hook on every outgoing packet
SEC("tc/dns_egress")  // attach the programm to a tc hook (when the tc spot a packet he calls our programm, we use TC instead of XDP because tc can spot engress and ingress)
int dns_query_timestamp(struct __sk_buff *skb)
{
    void *data     = (void *)(long)skb->data; // packet's first octet
    void *data_end = (void *)(long)skb->data_end;  // packet's last octet these two datas are useful for bounded the general data

    // Ethernet layer
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return TC_ACT_OK;  //too short for having an ethernet header

    // Ignore non-IPv4 packets
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return TC_ACT_OK;  //not an IPv4

    //  IP layer
    struct iphdr *iph = (struct iphdr *)(eth + 1);
    if ((void *)(iph + 1) > data_end)
        return TC_ACT_OK;  // too short for having a IP header

    // Ignore non-UDP packets
    if (iph->protocol != IPPROTO_UDP)
        return TC_ACT_OK;  // Not an UDP protocol

    // IP header length (variable between 20 and 60 bytes)
    __u32 ip_hlen = (__u32)(iph->ihl) << 2;
    if (ip_hlen < 20 || ip_hlen > 60)
        return TC_ACT_OK;

    // UDP layer
    struct udphdr *udph = (struct udphdr *)((char *)eth + sizeof(*eth) + ip_hlen);
    if ((void *)(udph + 1) > data_end)
        return TC_ACT_OK;  // too short for having a UDP header

    // Keep only packets destined to port 53 (DNS)
    if (udph->dest != bpf_htons(DNS_PORT))
        return TC_ACT_OK;  // not the conventional DNS port

    //  DNS header
    struct dnshdr *dnsh = (struct dnshdr *)(udph + 1);
    if ((void *)(dnsh + 1) > data_end)
        return TC_ACT_OK;  // too short for having a DNS header

    // If QR bit is 1 this is a response, not a query
    __u16 flags = bpf_ntohs(dnsh->flags);
    if (flags & DNS_QR_BIT)
        return TC_ACT_OK;  // ignoring the response

    // This is a DNS query - record timestamp and transaction ID
    __u64 ts     = bpf_ktime_get_ns();  // internal clock of the kernel, very precise
    __u16 txn_id = bpf_ntohs(dnsh->txn_id);  // get the ID of the query

    bpf_printk("[DNS-MONITOR] QUERY id=0x%04x ts=%llu ns\n", txn_id, ts);  //only way to communicate with the USER space

    // Always let the packet pass, we only observe
    return TC_ACT_OK;
}


// INTERMEDIATE LEVEL
// Handles both directions in one function - map is shared automatically
SEC("tc")
int dns_monitor(struct __sk_buff *skb)
{
    void *data     = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end) return TC_ACT_OK;
    if (eth->h_proto != bpf_htons(ETH_P_IP)) return TC_ACT_OK;

    struct iphdr *iph = (struct iphdr *)(eth + 1);
    if ((void *)(iph + 1) > data_end) return TC_ACT_OK;
    if (iph->protocol != IPPROTO_UDP) return TC_ACT_OK;

    __u32 ip_hlen = (__u32)(iph->ihl) << 2;
    if (ip_hlen < 20 || ip_hlen > 60) return TC_ACT_OK;

    struct udphdr *udph = (struct udphdr *)((char *)eth + sizeof(*eth) + ip_hlen);
    if ((void *)(udph + 1) > data_end) return TC_ACT_OK;

    struct dnshdr *dnsh = (struct dnshdr *)(udph + 1);
    if ((void *)(dnsh + 1) > data_end) return TC_ACT_OK;

    __u16 flags  = bpf_ntohs(dnsh->flags);
    __u16 txn_id = bpf_ntohs(dnsh->txn_id);

    // outgoing query : dest port 53, QR bit = 0
    if (udph->dest == bpf_htons(DNS_PORT) && !(flags & DNS_QR_BIT)) {
        __u64 ts = bpf_ktime_get_ns();
        bpf_map_update_elem(&dns_timestamps, &txn_id, &ts, BPF_ANY);
        bpf_printk("[DNS-MONITOR] QUERY id=0x%04x ts=%llu ns\n", txn_id, ts);
    }

    // incoming response : source port 53, QR bit = 1
    if (udph->source == bpf_htons(DNS_PORT) && (flags & DNS_QR_BIT)) {
        __u64 *query_ts = bpf_map_lookup_elem(&dns_timestamps, &txn_id);
        if (query_ts) {
            __u64 rtt_us = (bpf_ktime_get_ns() - *query_ts) / 1000;
            bpf_printk("[DNS-MONITOR] RESPONSE id=0x%04x RTT=%llu us\n",
                       txn_id, rtt_us);
            bpf_map_delete_elem(&dns_timestamps, &txn_id);
        }
    }

    return TC_ACT_OK;
}
char LICENSE[] SEC("license") = "GPL";  // mandatory for a eBPF programm
