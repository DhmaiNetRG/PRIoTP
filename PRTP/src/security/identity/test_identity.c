#include "identity.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    priotps_identity_t id1, id2;
    
    // 1. Generate identity
    identity_generate(&id1);
    
    // 2. Save and load
    assert(identity_save(&id1, "test_id.bin") == 0);
    assert(identity_load(&id2, "test_id.bin") == 0);
    
    assert(memcmp(id1.private_key, id2.private_key, 32) == 0);
    assert(memcmp(id1.public_key, id2.public_key, 32) == 0);
    
    // 3. Load or generate
    remove("test_id2.bin");
    priotps_identity_t id3;
    assert(identity_load_or_generate(&id3, "test_id2.bin") == 0);
    
    // 4. Wipe
    identity_wipe(&id1);
    uint8_t zero[32] = {0};
    assert(memcmp(id1.private_key, zero, 32) == 0);
    assert(memcmp(id1.public_key, zero, 32) == 0);
    
    printf("Identity tests passed.\n");
    return 0;
}
