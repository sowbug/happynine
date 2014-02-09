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

#include <iostream>

#include "base58.h"

#include "bigint.h"
#include "crypto.h"
#include "types.h"

#define BASE58_ALPHABET "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

inline unsigned int countLeading0s(const bytes_t& data) {
  unsigned int i = 0;
  for (; (i < data.size()) && (data[i] == 0); i++);
  return i;
}

inline unsigned int countLeading0s(const std::string& numeral,
                                   char zeroSymbol) {
  unsigned int i = 0;
  for (; (i < numeral.size()) && (numeral[i] == zeroSymbol); i++);
  return i;
}

std::string Base58::toBase58(const bytes_t& bytes) {
  if (bytes.size() == 0) {
    return std::string();
  }

  unsigned int leading_zeroes = countLeading0s(bytes);
  if (leading_zeroes == bytes.size()) {
    leading_zeroes--;
  }
  const std::string leading0s(leading_zeroes, '1');
  const BigInt bn(bytes);
  const std::string base58 = bn.getInBase(58, BASE58_ALPHABET);
  return leading0s + base58;
}

std::string Base58::toBase58Check(const bytes_t& bytes) {
  if (bytes.size() == 0) {
    return std::string();
  }

  bytes_t digest(Crypto::DoubleSHA256(bytes));
  bytes_t payload(bytes);
  payload.insert(payload.end(), &digest[0], &digest[4]);

  return toBase58(payload);
}

bytes_t Base58::fromBase58(const std::string s) {
  BigInt bn(s, 58, BASE58_ALPHABET);
  bytes_t bytes = bn.getBytes();
  bytes_t leading0s(countLeading0s(s, '1'), 0);
  bytes.insert(bytes.begin(), leading0s.begin(), leading0s.end());

  return bytes;
}

bytes_t Base58::fromBase58Check(const std::string s) {
  BigInt bn(s, 58, BASE58_ALPHABET);
  bytes_t bytes = bn.getBytes();
  if (bytes.size() < 4) return bytes_t();
  bytes_t checksum(bytes.end() - 4, bytes.end());
  bytes.assign(bytes.begin(), bytes.end() - 4);
  bytes_t leading0s(countLeading0s(s, '1'), 0);
  bytes.insert(bytes.begin(), leading0s.begin(), leading0s.end());

  bytes_t digest(Crypto::DoubleSHA256(bytes));

  digest.assign(digest.begin(), digest.begin() + 4);
  if (digest != checksum) return bytes_t();

  return bytes;
}

bytes_t Base58::fromAddress(const std::string addr_b58) {
  const bytes_t addr_bytes_with_version(Base58::fromBase58Check(addr_b58));
  return bytes_t(addr_bytes_with_version.begin() + 1,
                 addr_bytes_with_version.end());
}

// https://en.bitcoin.it/wiki/Technical_background_of_Bitcoin_addresses
bytes_t Base58::toHash160(const bytes_t& bytes) {
  if (bytes.size() == 0) {
    return bytes_t();
  }

  // 2. Perform SHA-256 hashing on the public key
  // 3. Perform RIPEMD-160 hashing on the result of SHA-256
  return Crypto::SHA256ThenRIPE(bytes);
}

std::string Base58::hash160toAddress(const bytes_t& bytes) {
  if (bytes.size() != 20) {
    return std::string();
  }
  bytes_t ripe_digest = bytes;

  // 4. Add version byte in front of RIPEMD-160 hash (0x00 for Main Network)
  bytes_t version(1, 0);
  ripe_digest.insert(ripe_digest.begin(), version.begin(), version.end());

  return toBase58Check(ripe_digest);
}

std::string Base58::toAddress(const bytes_t& bytes) {
  if (bytes.size() == 0) {
    return std::string();
  }
  return hash160toAddress(toHash160(bytes));
}

// http://procbits.com/2013/08/27/generating-a-bitcoin-address-with-javascript
std::string Base58::toPrivateKey(const bytes_t& bytes) {
  if (bytes.size() == 0) {
    return std::string();
  }

  bytes_t private_key_bytes(1, 0x80);

  private_key_bytes.insert(private_key_bytes.end(),
                           bytes.begin(),
                           bytes.end());
  bytes_t compressed_marker(1, 0x01);
  private_key_bytes.insert(private_key_bytes.end(),
                           compressed_marker.begin(),
                           compressed_marker.end());
  return toBase58Check(private_key_bytes);
}
