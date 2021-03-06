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

#if !defined(__CREDENTIALS_H__)
#define __CREDENTIALS_H__

#include <string>

#include "types.h"

class Credentials {
 public:
  Credentials();

  void Load(const bytes_t& salt,
            const bytes_t& check,
            const bytes_t& encrypted_ephemeral_key);
  bool SetPassphrase(const std::string& passphrase,
                     bytes_t& salt,
                     bytes_t& check,
                     bytes_t& encrypted_ephemeral_key);
  bool Unlock(const std::string& passphrase);
  bool Lock();

  bool isLocked() { return ephemeral_key_.empty(); }
  bool isPassphraseSet() { return !check_.empty(); }

  const bytes_t& ephemeral_key() { return ephemeral_key_; }

 private:
  bytes_t salt_;
  bytes_t check_;
  bytes_t encrypted_ephemeral_key_;
  bytes_t ephemeral_key_;

  DISALLOW_EVIL_CONSTRUCTORS(Credentials);
};

#endif  // #if !defined(__CREDENTIALS_H__)
