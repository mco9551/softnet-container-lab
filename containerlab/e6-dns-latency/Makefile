# E6 - DNS Response Latency Monitor

CLANG   ?= clang
IFACES  ?= eth0
BPF_SRC  = bpf/dns_latency.bpf.c
BPF_OBJ  = bpf/dns_latency.bpf.o

.PHONY: all load-basic load-intermediate unload clean

all: $(BPF_OBJ)

$(BPF_OBJ): $(BPF_SRC)
	$(CLANG) -O2 -g -Wall \
		-target bpf \
		-D__TARGET_ARCH_x86 \
		-I/usr/include/x86_64-linux-gnu \
		-c $< -o $@
	@echo "[OK] Compiled: $(BPF_OBJ)"

load-basic: $(BPF_OBJ)
	@for iface in $(IFACES); do \
		tc qdisc add dev $$iface clsact 2>/dev/null || true; \
		tc filter add dev $$iface egress bpf da obj $(BPF_OBJ) sec tc/dns_egress verbose; \
		echo "[OK] Basic loaded on $$iface egress"; \
	done

load-intermediate: $(BPF_OBJ)
	@for iface in $(IFACES); do \
		tc qdisc add dev $$iface clsact 2>/dev/null || true; \
		tc filter add dev $$iface egress  bpf da obj $(BPF_OBJ) sec tc verbose; \
		tc filter add dev $$iface ingress bpf da obj $(BPF_OBJ) sec tc verbose; \
		echo "[OK] Intermediate loaded on $$iface"; \
	done

unload:
	@for iface in $(IFACES); do \
		tc qdisc del dev $$iface clsact 2>/dev/null || true; \
		echo "[OK] Unloaded from $$iface"; \
	done

clean:
	rm -f $(BPF_OBJ)
	@echo "[OK] Cleaned"
