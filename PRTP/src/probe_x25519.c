#include <stdio.h>
#include <string.h>
#include "security/x25519/x25519.h"
static void hex2bin(unsigned char *out, const char *hex, int n) {
    int i; unsigned int v;
    for(i=0;i<n;i++){sscanf(hex+2*i,"%2x",&v);out[i]=(unsigned char)v;}
}
int main(void){
    unsigned char priv[32], pub[32], expected[32];
    hex2bin(priv,  "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a",32);
    hex2bin(expected,"8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a",32);
    x25519_public_key(pub,priv);
    printf("got: "); for(int i=0;i<32;i++) printf("%02x",pub[i]); printf("\n");
    printf("exp: "); for(int i=0;i<32;i++) printf("%02x",expected[i]); printf("\n");
    printf("match: %d\n", memcmp(pub,expected,32)==0);

    /* DH symmetry test with fixed keys */
    unsigned char privA[32], pubA[32], privB[32], pubB[32], sAB[32], sBA[32];
    for(int i=0;i<32;i++) privA[i]=(unsigned char)(i+1);
    for(int i=0;i<32;i++) privB[i]=(unsigned char)(i+33);
    x25519_public_key(pubA, privA);
    x25519_public_key(pubB, privB);
    x25519(sAB, privA, pubB);
    x25519(sBA, privB, pubA);
    printf("DH symmetric: %d\n", memcmp(sAB,sBA,32)==0);
    return 0;
}
