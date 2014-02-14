// Copyright 2014 Mike Tsao <mike@sowbug.com>

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "crypto.h"

extern "C" {
#include "scrypt/crypto_scrypt.h"
}

#if defined(DEBUG)
// Substantially quicker because debug isn't optimized
#define SCRYPT_N (256)
#define SCRYPT_R (2)
#define SCRYPT_P (2)
#else
// Params borrowed from BIP 0038
#define SCRYPT_N (16384)
#define SCRYPT_R (8)
#define SCRYPT_P (8)
#endif

#include "secp256k1.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>

const int ROUNDS = 32768;
const size_t AES_BLOCK_SIZE = 256 / 8;

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

  int error = crypto_scrypt(&passphrase_bytes[0],
                            passphrase_bytes.size(),
                            &salt[0],
                            salt.capacity(),
                            SCRYPT_N, SCRYPT_R, SCRYPT_P,
                            &key[0],
                            key.capacity());
  if (error == 0) {
    return true;
  }
  return false;
}

bool Crypto::Encrypt(const bytes_t& key,
                     const bytes_t& plaintext,
                     bytes_t& ciphertext) {
  EVP_CIPHER_CTX ctx;
  EVP_CIPHER_CTX_init(&ctx);

  bytes_t iv(AES_BLOCK_SIZE, 0);

  if (!GetRandomBytes(iv)) {
    return false;
  }
  if (!EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, &key[0], &iv[0])) {
    return false;
  }
  ciphertext.resize(plaintext.size() + AES_BLOCK_SIZE);
  int buffer_size = ciphertext.capacity();
  if (!EVP_EncryptUpdate(&ctx,
                         &ciphertext[0],
                         &buffer_size,
                         &plaintext[0],
                         plaintext.size())) {
    return false;
  }

  int final_size = 0;
  if (!EVP_EncryptFinal_ex(&ctx,
                           &ciphertext[buffer_size],
                           &final_size)) {
    return false;
  }
  ciphertext.resize(buffer_size + final_size);

  EVP_CIPHER_CTX_cleanup(&ctx);

  // Stick the IV at the start of the ciphertext.
  ciphertext.insert(ciphertext.begin(), iv.begin(), iv.end());

  return true;
}

bool Crypto::Decrypt(const bytes_t& key,
                     const bytes_t& ciphertext,
                     bytes_t& plaintext) {
  if (ciphertext.size() < AES_BLOCK_SIZE) {
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

  plaintext.resize(ciphertext.size() - AES_BLOCK_SIZE);
  int buffer_size = plaintext.capacity();
  if (!EVP_DecryptUpdate(&ctx,
                         &plaintext[0],
                         &buffer_size,
                         &ciphertext[AES_BLOCK_SIZE],
                         ciphertext.size() - AES_BLOCK_SIZE)) {
    return false;
  }

  int final_size = 0;
  if (!EVP_DecryptFinal_ex(&ctx, &plaintext[0] + buffer_size, &final_size)) {
    return false;
  }
  plaintext.resize(buffer_size + final_size);

  EVP_CIPHER_CTX_cleanup(&ctx);

  return true;
}

bool Crypto::Sign(const bytes_t& key,
                  const bytes_t& digest,
                  bytes_t& signature) {
  secp256k1_key ec_key;
  ec_key.setPrivKey(key);
  signature = secp256k1_sign(ec_key, digest);
  return true;
}

bytes_t Crypto::DoubleSHA256(const bytes_t& input) {
  bytes_t digest;
  digest.resize(SHA256_DIGEST_LENGTH);

  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &input[0], input.size());
  SHA256_Final(&digest[0], &sha256);
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &digest[0], digest.capacity());
  SHA256_Final(&digest[0], &sha256);

  return digest;
}

bytes_t Crypto::SHA256ThenRIPE(const bytes_t& input) {
  bytes_t digest;
  digest.resize(SHA256_DIGEST_LENGTH);

  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &input[0], input.size());
  SHA256_Final(&digest[0], &sha256);

  bytes_t ripe_digest;
  ripe_digest.resize(RIPEMD160_DIGEST_LENGTH);
  RIPEMD160_CTX ripemd;
  RIPEMD160_Init(&ripemd);
  RIPEMD160_Update(&ripemd, &digest[0], SHA256_DIGEST_LENGTH);
  RIPEMD160_Final(&ripe_digest[0], &ripemd);

  return ripe_digest;
}
