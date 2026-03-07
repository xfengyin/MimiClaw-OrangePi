/*
 * Crypto Header - API Key Encryption for Security
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdbool.h>

#define AES_KEY_SIZE 32
#define AES_IV_SIZE 16
#define MAX_ENCRYPTED_LENGTH 4096

// Function prototypes
int crypto_init(void);
void crypto_close(void);

// Key encryption/decryption
int crypto_encrypt_key(const char *plaintext, char *ciphertext, size_t ciphertext_size);
int crypto_decrypt_key(const char *ciphertext, char *plaintext, size_t plaintext_size);

// Secure storage
int crypto_save_key_to_file(const char *key, const char *filepath);
int crypto_load_key_from_file(const char *filepath, char *key, size_t key_size);

// Hash functions
void crypto_hash_sha256(const char *input, char *output, size_t output_size);

#endif // CRYPTO_H