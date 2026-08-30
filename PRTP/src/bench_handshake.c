/**
 * @file bench_handshake.c
 * @brief Benchmark program for security primitives and handshake.
 */

#include "security/blake2s/blake2s.h"
#include "security/x25519/x25519.h"
#include "security/ecies/ecies.h"
#include "security/handshake/handshake.h"
#include "security/identity/identity.h"

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define ITERATIONS 1000

double get_time_diff_us(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1e6 + (end->tv_nsec - start->tv_nsec) / 1e3;
}

void print_stats(const char *op, double *times, int iters) {
    double total = 0;
    double min = 1e9, max = 0;
    for (int i = 0; i < iters; i++) {
        total += times[i];
        if (times[i] < min) min = times[i];
        if (times[i] > max) max = times[i];
    }
    double avg = total / iters;
    double sq_diff_sum = 0;
    for (int i = 0; i < iters; i++) {
        sq_diff_sum += (times[i] - avg) * (times[i] - avg);
    }
    double stddev = sqrt(sq_diff_sum / iters);
    
    printf("%s,%d,%.2f,%.2f,%.2f,%.2f\n", op, iters, avg, min, max, stddev);
}

void bench_blake2s() {
    uint8_t in[32] = {1};
    uint8_t out[32];
    double times[ITERATIONS];
    struct timespec start, end;
    
    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        blake2s(out, 32, NULL, 0, in, 32);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times[i] = get_time_diff_us(&start, &end);
    }
    print_stats("BLAKE2s", times, ITERATIONS);
}

void bench_x25519_keygen() {
    uint8_t pub[32], priv[32];
    double times[ITERATIONS];
    struct timespec start, end;
    
    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        x25519_generate_keypair(pub, priv);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times[i] = get_time_diff_us(&start, &end);
    }
    print_stats("X25519_Keygen", times, ITERATIONS);
}

void bench_x25519_shared() {
    uint8_t pub1[32], priv1[32];
    uint8_t pub2[32], priv2[32];
    uint8_t shared[32];
    x25519_generate_keypair(pub1, priv1);
    x25519_generate_keypair(pub2, priv2);
    
    double times[ITERATIONS];
    struct timespec start, end;
    
    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        x25519(shared, priv1, pub2);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times[i] = get_time_diff_us(&start, &end);
    }
    print_stats("X25519_Shared", times, ITERATIONS);
}

void bench_ecies() {
    uint8_t pub[32], priv[32];
    x25519_generate_keypair(pub, priv);
    uint8_t pt[16] = {0x42};
    uint8_t ct[16 + ECIES_OVERHEAD];
    uint8_t out[16];
    size_t ct_len, out_len;
    
    double times_enc[ITERATIONS];
    double times_dec[ITERATIONS];
    struct timespec start, end;
    
    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        ecies_encrypt(pub, pt, 16, ct, &ct_len);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_enc[i] = get_time_diff_us(&start, &end);
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        ecies_decrypt(priv, ct, ct_len, out, &out_len);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_dec[i] = get_time_diff_us(&start, &end);
    }
    print_stats("ECIES_Encrypt", times_enc, ITERATIONS);
    print_stats("ECIES_Decrypt", times_dec, ITERATIONS);
}

void bench_handshake() {
    priotps_identity_t client_id;
    priotps_identity_t server_id;
    identity_load_or_generate(&client_id, "client.bin");
    identity_load_or_generate(&server_id, "server.bin");
    
    priotps_session_table_t server_sessions;
    session_table_init(&server_sessions);
    
    uint8_t sub_pkt[2048];
    size_t sub_len;
    uint8_t sct_pkt[2048];
    size_t sct_len;
    uint8_t ack_pkt[2048];
    size_t ack_len;
    
    double times[ITERATIONS];
    struct timespec start, end;
    
    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        /* Client build subscribe */
        handshake_client_build_subscribe(&client_id, sub_pkt, &sub_len);
        
        /* Server handle subscribe */
        priotps_session_t *server_sess;
        handshake_server_build_response(&server_id, client_id.public_key, &server_sessions, sct_pkt, &sct_len, &server_sess);
        
        /* Client process response */
        uint8_t session_key[16];
        uint8_t srv_pub[32];
        uint32_t sid;
        handshake_client_process_response(&client_id, sct_pkt, sct_len, session_key, srv_pub, &sid);
        
        /* Client build ACK */
        handshake_client_build_ack(&client_id, ack_pkt, &ack_len);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        times[i] = get_time_diff_us(&start, &end);
    }
    print_stats("Handshake_Full", times, ITERATIONS);
    
    identity_wipe(&client_id);
    identity_wipe(&server_id);
    remove("client.bin");
    remove("server.bin");
}

int main(void) {
    printf("operation,iterations,avg_us,min_us,max_us,stddev_us\n");
    bench_blake2s();
    bench_x25519_keygen();
    bench_x25519_shared();
    bench_ecies();
    bench_handshake();
    return 0;
}
