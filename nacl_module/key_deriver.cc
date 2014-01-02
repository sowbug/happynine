#include "key_deriver.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

const int ROUNDS = 32768;

KeyDeriver::KeyDeriver() {
}

KeyDeriver::~KeyDeriver() {
}

bool KeyDeriver::Derive(const std::string& passphrase,
                        bytes_t& key,
                        bytes_t& salt) {
  if (passphrase.size() == 0 || key.size() < 32 || salt.size() < 32) {
    return false;
  }
  bytes_t passphrase_bytes(passphrase.begin(), passphrase.end());

  if (RAND_bytes(&salt[0], salt.capacity()) == 1) {
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
  }
  return false;
}

bool KeyDeriver::Verify(const std::string& passphrase,
                        const bytes_t& key,
                        const bytes_t& salt) {
  if (passphrase.size() == 0 || key.size() < 32 || salt.size() < 32) {
    return false;
  }
  bytes_t passphrase_bytes(passphrase.begin(), passphrase.end());
  bytes_t derived_key(key.capacity(), 0);
  int error = PKCS5_PBKDF2_HMAC_SHA1((const char*)&passphrase_bytes[0],
                                     passphrase_bytes.size(),
                                     &salt[0],
                                     salt.capacity(),
                                     ROUNDS,
                                     derived_key.capacity(),
                                     &derived_key[0]);
  if (error == 1 && key == derived_key) {
    return true;
  }
  return false;
}
