/**
 * @file ascon.h
 * @brief ASCON-AEAD128 lightweight authenticated encryption primitive.
 *
 * Implements ASCON-AEAD128 as specified in the ASCON v1.2 submission
 * (Dobraunig, Eichlseder, Mendel, Schläffer, 2021) and standardized by
 * NIST as the primary lightweight AEAD algorithm.
 *
 * Algorithm parameters:
 *   Key size   : 128 bits (16 bytes)
 *   Nonce size : 128 bits (16 bytes)
 *   Tag size   : 128 bits (16 bytes)
 *   Rate       : 64 bits  (8 bytes) per permutation round
 *   Rounds (a) : 12  (initialization and finalization)
 *   Rounds (b) : 6   (encryption)
 *
 * This is a standalone, portable C99 implementation.
 * It has zero external dependencies beyond stdint.h and string.h.
 *
 * @note This file is part of the PRIoTPS security extension.
 *       Do NOT modify the ASCON primitive without re-running the
 *       ASCON Known Answer Tests (KATs).
 */

#ifndef ASCON_H
#define ASCON_H

#include <stdint.h>
#include <stddef.h>

/* --------------------------------------------------------------------------
 * ASCON-AEAD128 algorithm constants
 * -------------------------------------------------------------------------- */

#define ASCON_KEY_LEN    16   /**< Key size in bytes (128 bits)         */
#define ASCON_NONCE_LEN  16   /**< Nonce size in bytes (128 bits)       */
#define ASCON_TAG_LEN    16   /**< Authentication tag size in bytes     */
#define ASCON_RATE        8   /**< Permutation rate in bytes (64 bits)  */
#define ASCON_PA_ROUNDS  12   /**< Rounds for init/finalize phases      */
#define ASCON_PB_ROUNDS   6   /**< Rounds for data processing phase     */

/* --------------------------------------------------------------------------
 * ASCON internal state
 * -------------------------------------------------------------------------- */

/**
 * @struct ascon_state
 * @brief Internal 320-bit ASCON permutation state (5 × 64-bit words).
 *
 * Fields x0..x4 correspond to the five 64-bit state words described in
 * the ASCON specification. They are kept in host byte order during
 * computation and converted to/from big-endian only at rate absorption
 * and squeezing boundaries.
 */
struct ascon_state {
    uint64_t x0;  /**< State word 0 */
    uint64_t x1;  /**< State word 1 */
    uint64_t x2;  /**< State word 2 */
    uint64_t x3;  /**< State word 3 */
    uint64_t x4;  /**< State word 4 */
};

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
 * @brief Encrypt plaintext with ASCON-AEAD128.
 *
 * Produces ciphertext of the same length as the plaintext, plus a
 * 16-byte authentication tag appended to the end of @p ciphertext_and_tag.
 *
 * The output buffer must be allocated by the caller with at least
 * (plaintext_len + ASCON_TAG_LEN) bytes.
 *
 * @param ciphertext_and_tag  Output buffer: ciphertext followed by tag.
 * @param plaintext           Input plaintext bytes.
 * @param plaintext_len       Length of plaintext in bytes.
 * @param aad                 Associated data (authenticated but not encrypted).
 * @param aad_len             Length of associated data in bytes (may be 0).
 * @param nonce               16-byte nonce. Must be unique per (key, message).
 * @param key                 16-byte pre-shared key.
 */
void ascon_aead128_encrypt(
    uint8_t       *ciphertext_and_tag,
    const uint8_t *plaintext,
    size_t         plaintext_len,
    const uint8_t *aad,
    size_t         aad_len,
    const uint8_t *nonce,
    const uint8_t *key
);

/**
 * @brief Decrypt and verify ciphertext with ASCON-AEAD128.
 *
 * @p ciphertext_with_tag must contain the raw ciphertext followed
 * immediately by the 16-byte authentication tag (total length =
 * plaintext_len + ASCON_TAG_LEN).
 *
 * The output buffer @p plaintext must be allocated by the caller with at
 * least (ciphertext_with_tag_len - ASCON_TAG_LEN) bytes.
 *
 * @param plaintext           Output plaintext buffer.
 * @param ciphertext_with_tag Input ciphertext bytes followed by auth tag.
 * @param ciphertext_with_tag_len Total length including the tag.
 * @param aad                 Associated data (authenticated but not encrypted).
 * @param aad_len             Length of associated data in bytes (may be 0).
 * @param nonce               16-byte nonce. Must match the nonce used during encrypt.
 * @param key                 16-byte pre-shared key.
 * @return  0 on successful decryption and tag verification.
 * @return -1 on authentication failure. Plaintext output MUST be discarded.
 */
int ascon_aead128_decrypt(
    uint8_t       *plaintext,
    const uint8_t *ciphertext_with_tag,
    size_t         ciphertext_with_tag_len,
    const uint8_t *aad,
    size_t         aad_len,
    const uint8_t *nonce,
    const uint8_t *key
);

/* --------------------------------------------------------------------------
 * Phase 2 API — short-name aliases
 * -------------------------------------------------------------------------- */

/**
 * @brief Encrypt with ASCON-AEAD128 (Phase 2 API name).
 *
 * Alias for ascon_aead128_encrypt(). Produces ciphertext of the same
 * length as plaintext plus a 16-byte authentication tag.
 *
 * Output buffer must hold at least (plaintext_len + ASCON_TAG_LEN) bytes.
 *
 * @param ciphertext_and_tag  Output: ciphertext followed by 16-byte tag
 * @param plaintext           Input plaintext
 * @param plaintext_len       Plaintext length in bytes
 * @param aad                 Associated data (may be NULL if aad_len == 0)
 * @param aad_len             Length of associated data (may be 0)
 * @param nonce               16-byte unique nonce
 * @param key                 16-byte symmetric key
 */
void ascon_encrypt(
    uint8_t       *ciphertext_and_tag,
    const uint8_t *plaintext,
    size_t         plaintext_len,
    const uint8_t *aad,
    size_t         aad_len,
    const uint8_t *nonce,
    const uint8_t *key
);

/**
 * @brief Decrypt with ASCON-AEAD128 (Phase 2 API name).
 *
 * Alias for ascon_aead128_decrypt(). Verifies the authentication tag
 * and decrypts if the tag is valid.
 *
 * @param plaintext           Output plaintext buffer
 * @param ciphertext_with_tag Input: ciphertext + 16-byte tag
 * @param ciphertext_with_tag_len Total length including tag
 * @param aad                 Associated data (may be NULL if aad_len == 0)
 * @param aad_len             Length of associated data (may be 0)
 * @param nonce               16-byte nonce (must match encryption nonce)
 * @param key                 16-byte symmetric key
 * @return  0 on successful authentication and decryption
 * @return -1 on authentication failure (plaintext must be discarded)
 */
int ascon_decrypt(
    uint8_t       *plaintext,
    const uint8_t *ciphertext_with_tag,
    size_t         ciphertext_with_tag_len,
    const uint8_t *aad,
    size_t         aad_len,
    const uint8_t *nonce,
    const uint8_t *key
);

#endif /* ASCON_H */
