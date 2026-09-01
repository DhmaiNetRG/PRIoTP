#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "PRTP/src/ascon.h"

// Hex string to bytes
void hex2bytes(const char *hex, uint8_t *out, size_t out_len) {
    for (size_t i = 0; i < out_len; i++) {
        sscanf(hex + 2*i, "%2hhx", &out[i]);
    }
}

int main() {
    const char *key_hex = "bf473b6dd5d29bc4d9b4e2d4483c0716";
    const char *nonce_hex = "00040000000000010000000000000000";
    // Python ciphertext and tag:
    const char *ct_hex = "b4504a3eed4dc38f6bcdf793d32becfca29556817a58a41285608dbb4eefc68863edf7d35e53dac31ae679e3fd950f867027a777304646a1e838f6eafe7e8732ae26da0f5de0eea559165c02f535fedf43d0e364cb03ef9655dca9db634da7f73b512f2c1880b85926ff178b8eb18f9bab49c4fb7de4f1f10d3dabeceb146baec47e7fd4fdb562bc4d9c09238e5e403ba10155cf229993f22bcffc670bf6ef2128a33238c95ec551955a7002cbb8e62e9516d287c850aaa65a8c7d814f4938c90753";
    const char *tag_hex = "d6140ab4cce878ba0f38f85e06bcd719";
    
    uint8_t key[16], nonce[16];
    hex2bytes(key_hex, key, 16);
    hex2bytes(nonce_hex, nonce, 16);
    
    size_t pt_len = 194;
    uint8_t ct_full[194 + 16];
    hex2bytes(ct_hex, ct_full, 194);
    hex2bytes(tag_hex, ct_full + 194, 16);
    
    uint8_t pt[194];
    
    int res = ascon_aead128_decrypt(pt, ct_full, 194 + 16, NULL, 0, nonce, key);
    if (res != 0) {
        printf("DECRYPTION FAILED!\n");
        return 1;
    }
    
    printf("Recovered Plaintext (hex): ");
    for (size_t i = 0; i < pt_len; i++) printf("%02x", pt[i]);
    printf("\n");

    // Now re-encrypt
    uint8_t new_ct[194 + 16];
    ascon_aead128_encrypt(new_ct, pt, 194, NULL, 0, nonce, key);
    
    printf("Re-encrypted Ciphertext (hex): ");
    for (size_t i = 0; i < 194; i++) printf("%02x", new_ct[i]);
    printf("\n");
    printf("Re-encrypted Tag (hex): ");
    for (size_t i = 0; i < 16; i++) printf("%02x", new_ct[194 + i]);
    printf("\n");
    
    return 0;
}
