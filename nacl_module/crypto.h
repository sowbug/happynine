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

#include "types.h"

class Crypto {
 public:
  // Fills the given vector with "cryptographically strong
  // pseudo-random bytes" using OpenSSL's RAND_bytes(). If the
  // underlying method fails, returns false.
  static bool GetRandomBytes(bytes_t& bytes);

  // Takes a passphrase and salt and derives a key from them. Returns
  // true if successful.
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
                      const bytes_t& plaintext,
                      bytes_t& ciphertext);
  static bool Decrypt(const bytes_t& key,
                      const bytes_t& ciphertext,
                      bytes_t& plaintext);

  static bool Sign(const bytes_t& key,
                   const bytes_t& digest,
                   bytes_t& signature);

  static bytes_t DoubleSHA256(const bytes_t& input);
  static bytes_t SHA256ThenRIPE(const bytes_t& input);
};
