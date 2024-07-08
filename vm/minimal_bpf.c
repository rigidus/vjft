/* minimal_bpf.c  */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <stdint.h>

static uint64_t bpf_return_42(int param) {
    int ret;
    asm volatile (
        "r1 = r0 \n"
        "call 0 \n"
        : "=r" (ret)      /* OutputOperands */
        : "r" (param)     /* OutputOperands */
        : "memory", "r1", "r2", "r3", "r4", "r5" /* Clobbers */
        /* : GotoLabels (need add goto to asm volatile clause )  */
        );
    return ret;
}

SEC("prog")
int bpf_prog1(void *ctx) {
    return bpf_return_42(4);
}

char _license[] SEC("license") = "GPL";
