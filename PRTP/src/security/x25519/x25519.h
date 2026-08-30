#ifndef X25519_H
#define X25519_H
#include <stdint.h>

#define X25519_KEY_SIZE 32

/**
 * @brief Generate X25519 public key from private key.
 * Computes pub = scalar_mult(priv, basepoint).
 * @param pub  Output: 32-byte public key
 * @param priv Input: 32-byte private key (clamped internally)
 */
void x25519_public_key(uint8_t pub[X25519_KEY_SIZE],
                        const uint8_t priv[X25519_KEY_SIZE]);

/**
 * @brief Compute X25519 shared secret (ECDH).
 * Computes shared = scalar_mult(priv, peer_pub).
 * @param shared    Output: 32-byte shared secret
 * @param priv      Local 32-byte private key (clamped internally)
 * @param peer_pub  Remote 32-byte public key
 */
void x25519(uint8_t shared[X25519_KEY_SIZE],
             const uint8_t priv[X25519_KEY_SIZE],
             const uint8_t peer_pub[X25519_KEY_SIZE]);

/* ============================================================
 * Phase 2 API (descriptive names)
 * ============================================================ */

/**
 * @brief Generate a fresh X25519 key pair from a random seed.
 *
 * Reads 32 bytes from /dev/urandom, hashes them through BLAKE2s to
 * produce the private key, then derives the public key.
 *
 * @param priv_out  Output: 32-byte private key
 * @param pub_out   Output: 32-byte public key
 * @return 0 on success, -1 if /dev/urandom is unavailable
 */
int x25519_generate_keypair(uint8_t priv_out[X25519_KEY_SIZE],
                              uint8_t pub_out[X25519_KEY_SIZE]);

/**
 * @brief Derive X25519 public key from a private key (Phase 2 alias).
 * Identical to x25519_public_key().
 */
void x25519_public_from_private(uint8_t pub[X25519_KEY_SIZE],
                                  const uint8_t priv[X25519_KEY_SIZE]);

/**
 * @brief Compute X25519 ECDH shared secret (Phase 2 alias).
 * Identical to x25519().
 */
void x25519_shared_secret(uint8_t shared[X25519_KEY_SIZE],
                            const uint8_t priv[X25519_KEY_SIZE],
                            const uint8_t peer_pub[X25519_KEY_SIZE]);

#endif /* X25519_H */
