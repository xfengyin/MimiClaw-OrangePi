/*
 * Crypto Implementation - API Key Encryption
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include "crypto.h"
#include "logger.h"

static unsigned char master_key[AES_KEY_SIZE] = {0};
static bool initialized = false;

int crypto_init(void) {
    if (initialized) return 0;
    
    // Generate or load master key
    // In production, this should be derived from a hardware key or secure enclave
    const char *env_key = getenv("MIMICLAW_MASTER_KEY");
    if (env_key) {
        // Derive key from environment variable using SHA-256
        SHA256((unsigned char*)env_key, strlen(env_key), master_key);
    } else {
        // Generate random key (will be different on each restart)
        if (RAND_bytes(master_key, AES_KEY_SIZE) != 1) {
            LOG_ERROR("Failed to generate master key");
            return -1;
        }
        LOG_WARN("Using randomly generated master key. Keys will not persist across restarts.");
    }
    
    initialized = true;
    LOG_INFO("Crypto module initialized");
    return 0;
}

void crypto_close(void) {
    memset(master_key, 0, sizeof(master_key));
    initialized = false;
    LOG_INFO("Crypto module closed");
}

int crypto_encrypt_key(const char *plaintext, char *ciphertext, size_t ciphertext_size) {
    if (!initialized || !plaintext || !ciphertext || ciphertext_size == 0) return -1;
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;
    
    unsigned char iv[AES_IV_SIZE];
    if (RAND_bytes(iv, AES_IV_SIZE) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, master_key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    // Allocate temporary buffer
    int plaintext_len = strlen(plaintext);
    unsigned char *temp = malloc(plaintext_len + AES_BLOCK_SIZE);
    if (!temp) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int len;
    int ciphertext_len = 0;
    
    if (EVP_EncryptUpdate(ctx, temp, &len, (unsigned char*)plaintext, plaintext_len) != 1) {
        free(temp);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, temp + len, &len) != 1) {
        free(temp);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Format: IV (hex) + ":" + ciphertext (hex)
    if (ciphertext_size < (AES_IV_SIZE * 2 + 1 + ciphertext_len * 2 + 1)) {
        free(temp);
        return -1;
    }
    
    // Convert to hex
    for (int i = 0; i < AES_IV_SIZE; i++) {
        sprintf(ciphertext + i * 2, "%02x", iv[i]);
    }
    ciphertext[AES_IV_SIZE * 2] = ':';
    
    for (int i = 0; i < ciphertext_len; i++) {
        sprintf(ciphertext + AES_IV_SIZE * 2 + 1 + i * 2, "%02x", temp[i]);
    }
    ciphertext[AES_IV_SIZE * 2 + 1 + ciphertext_len * 2] = '\0';
    
    free(temp);
    return 0;
}

int crypto_decrypt_key(const char *ciphertext, char *plaintext, size_t plaintext_size) {
    if (!initialized || !ciphertext || !plaintext || plaintext_size == 0) return -1;
    
    // Parse IV and ciphertext
    char *colon = strchr(ciphertext, ':');
    if (!colon) return -1;
    
    int iv_hex_len = colon - ciphertext;
    if (iv_hex_len != AES_IV_SIZE * 2) return -1;
    
    unsigned char iv[AES_IV_SIZE];
    for (int i = 0; i < AES_IV_SIZE; i++) {
        sscanf(ciphertext + i * 2, "%2hhx", &iv[i]);
    }
    
    const char *cipher_hex = colon + 1;
    int cipher_hex_len = strlen(cipher_hex);
    int cipher_len = cipher_hex_len / 2;
    
    unsigned char *cipher = malloc(cipher_len);
    if (!cipher) return -1;
    
    for (int i = 0; i < cipher_len; i++) {
        sscanf(cipher_hex + i * 2, "%2hhx", &cipher[i]);
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(cipher);
        return -1;
    }
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, master_key, iv) != 1) {
        free(cipher);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int len;
    int plaintext_len = 0;
    
    if (EVP_DecryptUpdate(ctx, (unsigned char*)plaintext, &len, cipher, cipher_len) != 1) {
        free(cipher);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len = len;
    
    if (EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext + len, &len) != 1) {
        free(cipher);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;
    plaintext[plaintext_len] = '\0';
    
    free(cipher);
    EVP_CIPHER_CTX_free(ctx);
    
    return 0;
}

void crypto_hash_sha256(const char *input, char *output, size_t output_size) {
    if (!input || !output || output_size < 65) return;
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input, strlen(input), hash);
    
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + i * 2, "%02x", hash[i]);
    }
    output[64] = '\0';
}