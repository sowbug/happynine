#include "crypto.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

const int ROUNDS = 32768;

bool Crypto::GetRandomBytes(bytes_t& bytes) {
  return RAND_bytes(&bytes[0], bytes.capacity()) == 1;
}

bool Crypto::DeriveKey(const std::string& passphrase,
                       const bytes_t& salt,
                       bytes_t& key) {
  if (passphrase.size() == 0 || key.size() < 32 || salt.size() < 32) {
    return false;
  }
  bytes_t passphrase_bytes(passphrase.begin(), passphrase.end());

  int error = PKCS5_PBKDF2_HMAC_SHA1((const char*)&passphrase_bytes[0],
                                     passphrase_bytes.size(),
                                     &salt[0],
                                     salt.capacity(),
                                     ROUNDS,
                                     key.capacity(),
                                     &key[0]);
  if (error == 1) {
    return true;
  }
  return false;
}

const size_t BLOCK_SIZE = 256 / 8;

bool Crypto::Encrypt(const bytes_t& key,
                     const bytes_t& plaintext,
                     bytes_t& ciphertext) {
  if (plaintext.capacity() % BLOCK_SIZE != 0) {
    return false;
  }

  EVP_CIPHER_CTX ctx;
  EVP_CIPHER_CTX_init(&ctx);

  bytes_t iv(BLOCK_SIZE, 0);

  if (!GetRandomBytes(iv)) {
    return false;
  }
  if (!EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, &key[0], &iv[0])) {
    return false;
  }
  EVP_CIPHER_CTX_set_padding(&ctx, 0);
  ciphertext.resize(plaintext.capacity());
  int buffer_size = ciphertext.capacity();
  if (!EVP_EncryptUpdate(&ctx,
                         &ciphertext[0],
                         &buffer_size,
                         &plaintext[0],
                         plaintext.capacity())) {
    return false;
  }

  // We don't call EVP_EncryptFinal_ex because we have already checked
  // that our plaintext is a multiple of the block size.

  EVP_CIPHER_CTX_cleanup(&ctx);

  // Stick the IV at the start of the ciphertext.
  ciphertext.insert(ciphertext.begin(), iv.begin(), iv.end());

  return true;
}

bool Crypto::Decrypt(const bytes_t& key,
                     const bytes_t& ciphertext,
                     bytes_t& plaintext) {
  if (ciphertext.capacity() < BLOCK_SIZE) {
    return false;
  }
  if (ciphertext.capacity() % BLOCK_SIZE != 0) {
    return false;
  }

  EVP_CIPHER_CTX ctx;
  EVP_CIPHER_CTX_init(&ctx);

  if (!EVP_DecryptInit_ex(&ctx,
                          EVP_aes_256_cbc(),
                          NULL,
                          &key[0],
                          &ciphertext[0])) {
    return false;
  }

  EVP_CIPHER_CTX_set_padding(&ctx, 0);

  // Make room for everything but the IV
  plaintext.resize(ciphertext.capacity() - BLOCK_SIZE);
  int buffer_size = plaintext.capacity();
  if (!EVP_DecryptUpdate(&ctx,
                         &plaintext[0],
                         &buffer_size,
                         &ciphertext[BLOCK_SIZE],
                         ciphertext.capacity() - BLOCK_SIZE)) {
    return false;
  }

  // We don't call EVP_DecryptFinal_ex because we have already checked
  // that our ciphertext is a multiple of the block size.

  EVP_CIPHER_CTX_cleanup(&ctx);

  return true;
}
