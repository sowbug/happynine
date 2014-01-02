#include <string>

#include "types.h"

class KeyDeriver {
 public:
  KeyDeriver();
  virtual ~KeyDeriver();

  // Using PKCS5_PBKDF2_HMAC_SHA1, takes a passphrase and salt and
  // derives a key from them. Returns true if successful.
  //
  // The key vector should be set to the desired capacity. For example:
  //
  // bytes_t salt(32, 0), key(32, 0);
  // RNG.GetRandomBytes(salt);
  // if (key_deriver.Derive("foo", salt, key)) { go... }
  static bool Derive(const std::string& passphrase,
                     const bytes_t& salt,
                     bytes_t& key);
};
