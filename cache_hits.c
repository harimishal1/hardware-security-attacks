#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"   // must provide rdtscp(), clflush(), lfence()

#define SAMPLES 10000

int main(void) {
    const size_t CACHELINE = 64;
    uint8_t *buf;
    if (posix_memalign((void **)&buf, CACHELINE, CACHELINE) != 0) {
        perror("posix_memalign");
        return 1;
    }
    memset(buf, 0, CACHELINE);

    volatile uint8_t sink = 0;

    // Measure cache hits
    for (int i = 0; i < SAMPLES; i++) {
        *buf;
        lfence();
        uint64_t t1 = rdtscp();
        sink ^= *buf;
        uint64_t t2 = rdtscp();
        printf("%d,hit,%lu\n", i, t2 - t1);
    }

    // Measure cache misses
    for (int i = 0; i < SAMPLES; i++) {
        clflush(buf);
        lfence();
        uint64_t t1 = rdtscp();
        sink ^= *buf;
        uint64_t t2 = rdtscp();
        printf("%d,miss,%lu\n", i, t2 - t1);
    }

    free(buf);
    if (sink == 0x42) printf("sink=%d\n", sink); 
    return 0;
}
