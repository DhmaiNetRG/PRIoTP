#ifndef ECIES_H
#define ECIES_H
#include <stdint.h>
#include <stddef.h>

#define ECIES_OVERHEAD  24   /* 8-byte nonce + 16-byte tag */

/**
 * @brief Encrypt message using private key.
 * Decryptable with the corresponding public key via ecies_decrypt_with_pub().
 * @param msg      Plaintext input
 * @param msg_len  Plaintext length in bytes
 * @param priv_key 32-byte private key
 * @param out      Output buffer (must be >= msg_len + ECIES_OVERHEAD)
 * @param out_len  Set to total output bytes on success
 * @return 0 on success, -1 on error
 */
int ecies_encrypt_with_priv(const uint8_t *msg, size_t msg_len,
                              const uint8_t priv_key[32],
                              uint8_t *out, size_t *out_len);

/**
 * @brief Decrypt message using public key.
 * Used to decrypt ciphertext produced by ecies_encrypt_with_priv().
 * @param ct      Ciphertext (nonce || ciphertext || tag)
 * @param ct_len  Total ciphertext length
 * @param pub_key 32-byte public key (corresponding to priv used to encrypt)
 * @param out     Output buffer (must be >= ct_len - ECIES_OVERHEAD)
 * @param out_len Set to plaintext bytes on success
 * @return 0 on success, -1 on authentication failure
 */
int ecies_decrypt_with_pub(const uint8_t *ct, size_t ct_len,
                             const uint8_t pub_key[32],
                             uint8_t *out, size_t *out_len);

/**
 * @brief Encrypt message using public key.
 * Decryptable only with the corresponding private key via ecies_decrypt_with_priv().
 * @param msg      Plaintext input
 * @param msg_len  Plaintext length in bytes
 * @param pub_key  32-byte public key
 * @param out      Output buffer (must be >= msg_len + ECIES_OVERHEAD)
 * @param out_len  Set to total output bytes on success
 * @return 0 on success, -1 on error
 */
int ecies_encrypt_with_pub(const uint8_t *msg, size_t msg_len,
                             const uint8_t pub_key[32],
                             uint8_t *out, size_t *out_len);

/**
 * @brief Decrypt message using private key.
 * Used to decrypt ciphertext produced by ecies_encrypt_with_pub().
 * @param ct       Ciphertext (nonce || ciphertext || tag)
 * @param ct_len   Total ciphertext length
 * @param priv_key 32-byte private key
 * @param out      Output buffer (must be >= ct_len - ECIES_OVERHEAD)
 * @param out_len  Set to plaintext bytes on success
 * @return 0 on success, -1 on authentication failure
 */
int ecies_decrypt_with_priv(const uint8_t *ct, size_t ct_len,
                              const uint8_t priv_key[32],
                              uint8_t *out, size_t *out_len);

/* ============================================================
 * Phase 2 API — unified encrypt/decrypt
 * ============================================================ */

/**
 * @brief Encrypt plaintext to a recipient's public key (Phase 2 API).
 *
 * Equivalent to ecies_encrypt_with_pub(). Produces a ciphertext that
 * can only be decrypted by the holder of the corresponding private key.
 *
 * @param msg      Plaintext input
 * @param msg_len  Plaintext length in bytes
 * @param pub_key  Recipient's 32-byte public key
 * @param out      Output buffer (must be >= msg_len + ECIES_OVERHEAD)
 * @param out_len  Set to total output bytes on success
 * @return 0 on success, -1 on error
 */
int ecies_encrypt(const uint8_t *msg, size_t msg_len,
                   const uint8_t pub_key[32],
                   uint8_t *out, size_t *out_len);

/**
 * @brief Decrypt ciphertext with a private key (Phase 2 API).
 *
 * Equivalent to ecies_decrypt_with_priv(). Decrypts ciphertext produced
 * by ecies_encrypt() / ecies_encrypt_with_pub().
 *
 * @param ct       Ciphertext (nonce || ciphertext || tag)
 * @param ct_len   Total ciphertext length
 * @param priv_key 32-byte private key
 * @param out      Output buffer (must be >= ct_len - ECIES_OVERHEAD)
 * @param out_len  Set to plaintext bytes on success
 * @return 0 on success, -1 on authentication failure
 */
int ecies_decrypt(const uint8_t *ct, size_t ct_len,
                   const uint8_t priv_key[32],
                   uint8_t *out, size_t *out_len);

#endif /* ECIES_H */
