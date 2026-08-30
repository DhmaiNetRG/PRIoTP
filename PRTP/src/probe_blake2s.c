#include <stdio.h>
#include "security/blake2s/blake2s.h"
int main(void) {
    uint8_t h[32]; int i;
    blake2s(h, "", 0);
    printf("empty: "); for(i=0;i<32;i++) printf("%02x",h[i]); printf("\n");
    blake2s(h, "abc", 3);
    printf("abc:   "); for(i=0;i<32;i++) printf("%02x",h[i]); printf("\n");
    return 0;
}
