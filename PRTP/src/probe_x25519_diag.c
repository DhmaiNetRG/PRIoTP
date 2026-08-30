/*
 * Isolate exactly where X25519 breaks.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "security/x25519/x25519.h"

int main(void) {
    /* Test 1: scalarmult(1, 9) should give 9 (scalar=1 means clamped to ... actually clamping changes it) */
    /* After clamping: e[0]&=248 clears bits 0,1,2; e[31]&=127 clears bit 255; e[31]|=64 sets bit 254
     * So scalar 1 becomes: 0 after clear bits 0-2, but bit 254 is set = 2^254
     * That's not 1. Use scalar = 2^254 + something. */

    /* Test: the simplest check: does scalarmult(e, basepoint) give wrong result? */
    unsigned char bp[32]={9};
    unsigned char priv_one[32]={0}; priv_one[0]=1;
    unsigned char pub_one[32];
    x25519_public_key(pub_one, priv_one);
    printf("x25519(1,G) = "); for(int i=0;i<32;i++) printf("%02x",pub_one[i]); printf("\n");

    /* Now: what is 2^254 * G? (what priv_one becomes after clamping) */
    /* priv_one[0] = 1 → after e[0]&=248 → e[0]=0 → priv_one=0 but e[31]|=64 sets bit 254 */
    /* So clamped scalar = 2^254 — this is a specific known value */

    /* Test 2: try small scalar where we know the output */
    /* RFC 7748 test vector */
    unsigned char alice_priv[32];
    unsigned char alice_pub_exp[32];
    unsigned char alice_pub_got[32];
    void hex2bin(unsigned char*,const char*,int);

    int i; unsigned int v;
    const char *ah = "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a";
    const char *pe = "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a";
    for(i=0;i<32;i++){sscanf(ah+2*i,"%2x",&v);alice_priv[i]=(unsigned char)v;}
    for(i=0;i<32;i++){sscanf(pe+2*i,"%2x",&v);alice_pub_exp[i]=(unsigned char)v;}

    x25519_public_key(alice_pub_got, alice_priv);
    printf("RFC7748 alice got: "); for(i=0;i<32;i++) printf("%02x",alice_pub_got[i]); printf("\n");
    printf("RFC7748 alice exp: "); for(i=0;i<32;i++) printf("%02x",alice_pub_exp[i]); printf("\n");

    /* Test 3: verify clamping by printing what alice_priv becomes after clamp */
    unsigned char ce[32]; memcpy(ce,alice_priv,32);
    ce[0]&=248; ce[31]&=127; ce[31]|=64;
    printf("Clamped priv: "); for(i=0;i<32;i++) printf("%02x",ce[i]); printf("\n");
    printf("Orig    priv: "); for(i=0;i<32;i++) printf("%02x",alice_priv[i]); printf("\n");

    return 0;
}
