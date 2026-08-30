/**
 * @file bench_ascon.c
 * @brief Benchmark program for ASCON-AEAD128 encryption and decryption.
 */

#include "ascon.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#define ITERATIONS 1000

double get_time_diff_us(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1e6 + (end->tv_nsec - start->tv_nsec) / 1e3;
}

void bench_ascon_op(int is_encrypt, size_t payload_size) {
    uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    uint8_t nonce[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    
    uint8_t pt[2048] = {0};
    uint8_t ct[2048 + 16] = {0};
    uint8_t out[2048] = {0};
    
    /* Pre-encrypt for decryption benchmark */
    if (!is_encrypt) {
        ascon_aead128_encrypt(ct, pt, payload_size, NULL, 0, nonce, key);
    }
    
    struct timespec start, end;
    double total_us = 0;
    double min_us = 1e9;
    double max_us = 0;
    
    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        if (is_encrypt) {
            ascon_aead128_encrypt(ct, pt, payload_size, NULL, 0, nonce, key);
        } else {
            ascon_aead128_decrypt(out, ct, payload_size + 16, NULL, 0, nonce, key);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double diff = get_time_diff_us(&start, &end);
        total_us += diff;
        if (diff < min_us) min_us = diff;
        if (diff > max_us) max_us = diff;
    }
    
    double avg_us = total_us / ITERATIONS;
    double throughput_mbps = (payload_size * 8.0 * ITERATIONS) / total_us;
    
    printf("%zu,%s,%d,%.2f,%.2f,%.2f,%.2f\n", 
           payload_size, 
           is_encrypt ? "encrypt" : "decrypt", 
           ITERATIONS, 
           avg_us, min_us, max_us, throughput_mbps);
}

int main(void) {
    size_t sizes[] = {32, 64, 128, 256, 512, 1024, 2048};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("payload_size,operation,iterations,avg_us,min_us,max_us,throughput_mbps\n");
    
    for (int i = 0; i < num_sizes; i++) {
        bench_ascon_op(1, sizes[i]);
        bench_ascon_op(0, sizes[i]);
    }
    
    return 0;
}
