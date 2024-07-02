#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
/* #include <stdint.h> */

SEC("prog")
int bpf_prog1(void *ctx) {
    return 0;
}

char _license[] SEC("license") = "GPL";
