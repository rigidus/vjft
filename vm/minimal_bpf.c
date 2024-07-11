/* minimal_bpf.c  */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <stdint.h>

enum bpf_helper_id {
    BPF_FUNC_return_42 = 0,
};

static uint64_t bpf_return_42(uint64_t param) {
    uint64_t ret;
    asm volatile (
        /* "r1 = r0 \n" */
        "call %c[func_id] \n"
        : "=r" (ret)                                          /* OutputOperands */
        : "r" (param), [func_id] "i" (BPF_FUNC_return_42)     /* InputOperands */
        : "memory", "r1", "r2", "r3", "r4", "r5"              /* Clobbers */
        /* : GotoLabels (need add goto to asm volatile clause )  */
        );
    return ret;
}

int bpf_prog1(void *ctx) {
    return bpf_return_42(4);
    /* return bpf_return_42(); */
}
