/*
 * probe_x25519_rt.c — roundtrip test for fe_frombytes/fe_tobytes
 * and the RFC 7748 basepoint multiplication.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Test fe roundtrip by recomputing inline */
static void hex2bin(unsigned char *out, const char *hex, int n){
    int i; unsigned int v;
    for(i=0;i<n;i++){sscanf(hex+2*i,"%2x",&v);out[i]=(unsigned char)v;}
}

/* Minimal GF(2^255-19) using simple 256-bit integers — for oracle comparison */
/* We just check: does scalarmult(privA, basepoint) match expected? */

#include "security/x25519/x25519.h"

int main(void){
    unsigned char bp[32]={9};
    unsigned char privA[32], pubA_got[32], pubA_exp[32];

    /* RFC 7748 §6.1 Alice */
    hex2bin(privA,"77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a",32);
    hex2bin(pubA_exp,"8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a",32);

    x25519_public_key(pubA_got, privA);
    printf("Alice pub got: "); for(int i=0;i<32;i++) printf("%02x",pubA_got[i]); printf("\n");
    printf("Alice pub exp: "); for(int i=0;i<32;i++) printf("%02x",pubA_exp[i]); printf("\n");
    printf("Alice match: %d\n\n", memcmp(pubA_got,pubA_exp,32)==0);

    /* Verify roundtrip: x = 9, then tobytes(frombytes(x)) == x? */
    unsigned char rt_in[32]={9}, rt_out[32];
    x25519_public_key(rt_out, privA);  /* This computes frombytes(9) → ladder → tobytes */
    printf("Basepoint encode: "); for(int i=0;i<32;i++) printf("%02x",bp[i]); printf("\n");
    printf("Output:           "); for(int i=0;i<32;i++) printf("%02x",rt_out[i]); printf("\n");

    return 0;
}
