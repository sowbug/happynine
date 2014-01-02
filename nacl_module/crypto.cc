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

bool Crypto::Encrypt(const bytes_t& /*key*/,
                     const std::string& /*plaintext*/,
                     bytes_t& /*ciphertext*/) {
  return false;
}

bool Crypto::Decrypt(const bytes_t& /*key*/,
                     const bytes_t& /*ciphertext*/,
                     std::string& /*plaintext*/) {
  return false;
}
