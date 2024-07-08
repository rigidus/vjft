/* minimal_bpf.c  */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <stdint.h>

static uint64_t bpf_return_42(int param) {
    int ret;
    asm volatile (
        /* "mov r0, %1\n" */
        "call 0\n"
        : "=r" (ret)
        : "0" (param)
        : "memory", "r1", "r2", "r3", "r4", "r5"
        );
    return ret;
}

SEC("prog")
int bpf_prog1(void *ctx) {
    return bpf_return_42(24);
}

char _license[] SEC("license") = "GPL";
