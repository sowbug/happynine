#include <string>

#include "types.h"

class KeyDeriver {
 public:
  KeyDeriver();
  virtual ~KeyDeriver();

  // Using PKCS5_PBKDF2_HMAC_SHA1, takes a std::string representing a
  // passphrase and derives a key from it. Returns true if successful.
  //
  // Generates a salt as well. Returns both key and salt.
  //
  // Both key and salt should be resized to the desired length. For example:
  //
  // bytes_t key, salt;
  // key.resize(32); salt.resize(32);
  // if (key_deriver.Derive("foo", key, salt)) { go... }
  bool Derive(const std::string& passphrase,
              bytes_t& key,
              bytes_t& salt);

  // Pass in what you got from Derive() and it will return true.
  bool Verify(const std::string& passphrase,
              const bytes_t& key,
              const bytes_t& salt);
};
