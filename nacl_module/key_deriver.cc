#include "key_deriver.h"

#include <openssl/evp.h>

const int ROUNDS = 32768;

KeyDeriver::KeyDeriver() {
}

KeyDeriver::~KeyDeriver() {
}

bool KeyDeriver::Derive(const std::string& passphrase,
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
