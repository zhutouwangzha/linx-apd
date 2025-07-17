#include "bpf_check.h"
#include "get_pt_regs.h"

#define ETH_P_IP 0x0800

static bool is_tcp(struct ethhdr *eth, void *data_end)
{
    if ((void *)(eth + 1) > data_end) {
        return false;
    }

    if (bpf_ntohs(eth->h_proto) != ETH_P_IP) {
        return false;
    }

    struct iphdr *ip = (struct iphdr *)(eth + 1);

    if ((void *)(ip + 1) > data_end) {
        return false;
    }

    if (ip->protocol != IPPROTO_TCP) {
        return false;
    }

    return true;
}

SEC("xdp")
int xdp_pass(struct xdp_md *ctx)
{
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    struct ethhdr *eth = data;

    if (!is_tcp(eth, data_end)) {
        return XDP_PASS;
    }

    struct iphdr *ip = (struct iphdr *)(eth + 1);

    int ip_hdr_len = ip->ihl * 4;
    if (ip_hdr_len < sizeof(struct iphdr)) {
        return XDP_PASS;
    }

    if ((void *)ip + ip_hdr_len > data_end) {
        return XDP_PASS;
    }

    struct tcphdr *tcp = (struct tcphdr *)((unsigned char *)ip + ip_hdr_len);

    if ((void *)(tcp + 1) > data_end) {
        return XDP_PASS;
    }

    const int tcp_header_bytes = 32;

    if ((void *)tcp + tcp_header_bytes > data_end) {
        return XDP_PASS;
    }

    // void *ringbuf_space = bpf_ringbuf_reserve(&rb, tcp_header_bytes, 0);
    // if (!ringbuf_space) {
    //     return XDP_PASS;
    // }

    // for (int i = 0; i < tcp_header_bytes; ++i) {
    //     unsigned char byte = *((unsigned char *)tcp + i);
    //     ((unsigned char *)ringbuf_space)[i] = byte;
    // }

    // bpf_ringbuf_submit(ringbuf_space, 0);

    bpf_printk("Captured TCP header (%d bytes)", tcp_header_bytes);

    return XDP_PASS;
}
