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

#include "credentials.h"

#include <iostream>  // cerr

#include "crypto.h"

const uint32_t KEY_SIZE = 32;

// echo -n "Happynine Copyright 2014 Mike Tsao." | sha256sum
const bytes_t PASSPHRASE_CHECK =
  unhexlify(
            "df3bc110ce022d64a20503502a9edfd8acda8a39868e5dff6601c0bb9b6f9cf9"
            );

Credentials::Credentials() {
}

Credentials& Credentials::GetSingleton() {
  static Credentials singleton;
  return singleton;
}

void Credentials::Load(const bytes_t& salt,
                       const bytes_t& check,
                       const bytes_t& encrypted_ephemeral_key) {
  salt_ = salt;
  check_ = check;
  encrypted_ephemeral_key_ = encrypted_ephemeral_key;
}

bool Credentials::SetPassphrase(const std::string& passphrase,
                                bytes_t& salt,
                                bytes_t& check,
                                bytes_t& encrypted_ephemeral_key) {
  if (isPassphraseSet()) {
    // We're changing an existing passphrase.
    if (isLocked()) {
      return false;
    }
  } else {
    // We're setting a new passphrase. Generate a new ephemeral key.
    ephemeral_key_.resize(KEY_SIZE);
    Crypto::GetRandomBytes(ephemeral_key_);
  }

  // Generate a new salt.
  salt.resize(KEY_SIZE);
  Crypto::GetRandomBytes(salt);

  // Generate the new key.
  bytes_t key(KEY_SIZE, 0);
  if (!Crypto::DeriveKey(passphrase, salt, key)) {
    return false;
  }

  // Determine check.
  check.resize(KEY_SIZE);
  if (!Crypto::Encrypt(key, PASSPHRASE_CHECK, check)) {
    return false;
  }

  // Encrypt the ephemeral key.
  if (!Crypto::Encrypt(key, ephemeral_key_, encrypted_ephemeral_key)) {
    return false;
  }

  // Save what we've just done.
  Load(salt, check, encrypted_ephemeral_key);

  return true;
}

bool Credentials::Unlock(const std::string& passphrase) {
  if (!isLocked()) {
    return false;  // in case someone uses result to verify a passphrase
  }

  // Generate the purported key.
  bytes_t key(KEY_SIZE, 0);
  if (!Crypto::DeriveKey(passphrase, salt_, key)) {
    return false;
  }

  // Does the check match?
  bytes_t check_decrypted;
  if (!Crypto::Decrypt(key, check_, check_decrypted)) {
    return false;
  }
  if (check_decrypted != PASSPHRASE_CHECK) {
    return false;
  }

  // Check is good. Decrypt the ephemeral key.
  if (!Crypto::Decrypt(key, encrypted_ephemeral_key_, ephemeral_key_)) {
    return false;
  }

  return true;
}

bool Credentials::Lock() {
  ephemeral_key_.clear();
  return true;
}
