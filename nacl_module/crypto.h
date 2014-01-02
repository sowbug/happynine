#include "types.h"

class Crypto {
 public:
  // Fills the given vector with "cryptographically strong
  // pseudo-random bytes" using OpenSSL's RAND_bytes(). If the
  // underlying method fails, returns false.
  static bool GetRandomBytes(bytes_t& bytes);

  // Using PKCS5_PBKDF2_HMAC_SHA1, takes a passphrase and salt and
  // derives a key from them. Returns true if successful.
  //
  // The key vector should be set to the desired capacity. For example:
  //
  // bytes_t salt(32, 0), key(32, 0);
  // RNG.GetRandomBytes(salt);
  // if (key_deriver.Derive("foo", salt, key)) { go... }
  static bool DeriveKey(const std::string& passphrase,
                        const bytes_t& salt,
                        bytes_t& key);

  static bool Encrypt(const bytes_t& key,
                      const std::string& plaintext,
                      bytes_t& ciphertext);
  static bool Decrypt(const bytes_t& key,
                      const bytes_t& ciphertext,
                      std::string& plaintext);
};
