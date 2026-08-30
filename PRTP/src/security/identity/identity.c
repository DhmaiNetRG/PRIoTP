#include "identity.h"
#include "../blake2s/blake2s.h"
#include "../x25519/x25519.h"
#include <stdio.h>
#include <string.h>

void identity_generate(priotps_identity_t *id) {
    uint8_t seed[32];
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        fread(seed, 1, 32, f);
        fclose(f);
    } else {
        memset(seed, 0x42, 32); 
    }
    
    uint8_t digest[32];
    blake2s(digest, seed, 32);
    
    memcpy(id->private_key, digest, 32);
    x25519_public_key(id->public_key, digest);
}

int identity_save(const priotps_identity_t *id, const char *filepath) {
    FILE *f = fopen(filepath, "wb");
    if (!f) return -1;
    
    if (fwrite(id->private_key, 1, 32, f) != 32) {
        fclose(f);
        return -1;
    }
    if (fwrite(id->public_key, 1, 32, f) != 32) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    return 0;
}

int identity_load(priotps_identity_t *id, const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;
    
    if (fread(id->private_key, 1, 32, f) != 32) {
        fclose(f);
        return -1;
    }
    if (fread(id->public_key, 1, 32, f) != 32) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    return 0;
}

int identity_load_or_generate(priotps_identity_t *id, const char *filepath) {
    if (identity_load(id, filepath) == 0) {
        return 0;
    }
    identity_generate(id);
    return identity_save(id, filepath);
}

void identity_wipe(priotps_identity_t *id) {
    if (id) {
        memset(id, 0, sizeof(priotps_identity_t));
    }
}
