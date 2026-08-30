#ifndef IDENTITY_H
#define IDENTITY_H
#include <stdint.h>

#define IDENTITY_PRIV_SIZE 32
#define IDENTITY_PUB_SIZE  32

/** @brief A node's asymmetric identity (private + public key pair). */
typedef struct {
    uint8_t private_key[IDENTITY_PRIV_SIZE];
    uint8_t public_key[IDENTITY_PUB_SIZE];
} priotps_identity_t;

/**
 * @brief Generate a new identity (random seed -> BLAKE2s -> X25519).
 * @param id Output identity structure
 */
void identity_generate(priotps_identity_t *id);

/**
 * @brief Save identity to a binary file (64 bytes: priv||pub).
 * @param id       Identity to save
 * @param filepath File path to write
 * @return 0 on success, -1 on error
 */
int identity_save(const priotps_identity_t *id, const char *filepath);

/**
 * @brief Load identity from a binary file.
 * @param id       Output identity structure
 * @param filepath File path to read
 * @return 0 on success, -1 on error
 */
int identity_load(priotps_identity_t *id, const char *filepath);

/**
 * @brief Load identity from file, generating a new one if not found.
 * If the file does not exist, generates a new identity and saves it.
 * @param id       Output identity structure
 * @param filepath File path
 * @return 0 on success, -1 on error
 */
int identity_load_or_generate(priotps_identity_t *id, const char *filepath);

/**
 * @brief Zero out identity memory (call before freeing).
 * @param id Identity to wipe
 */
void identity_wipe(priotps_identity_t *id);

#endif /* IDENTITY_H */
