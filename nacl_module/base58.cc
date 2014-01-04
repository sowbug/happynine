#include "base58.h"

#include "bigint.h"
#include "openssl/ripemd.h"
#include "openssl/sha.h"
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

std::string Base58::toBase58Check(const bytes_t& bytes) {
  if (bytes.size() == 0) {
    return std::string();
  }

  bytes_t digest;
  digest.resize(SHA256_DIGEST_LENGTH);

  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &bytes[0], bytes.size());
  SHA256_Final(&digest[0], &sha256);
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &digest[0], digest.capacity());
  SHA256_Final(&digest[0], &sha256);

  bytes_t payload(bytes);
  payload.insert(payload.end(), &digest[0], &digest[4]);

  const BigInt bn(payload);
  const std::string base58check = bn.getInBase(58, BASE58_ALPHABET);
  const std::string leading0s(countLeading0s(payload), '1');
  return leading0s + base58check;
}

bytes_t Base58::fromBase58Check(const std::string s) {
  BigInt bn(s, 58, BASE58_ALPHABET);
  bytes_t bytes = bn.getBytes();
  if (bytes.size() < 4) return bytes_t();
  bytes_t checksum(bytes.end() - 4, bytes.end());
  bytes.assign(bytes.begin(), bytes.end() - 4);
  bytes_t leading0s(countLeading0s(s, '1'), 0);
  bytes.insert(bytes.begin(), leading0s.begin(), leading0s.end());

  bytes_t digest;
  digest.resize(SHA256_DIGEST_LENGTH);

  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &bytes[0], bytes.size());
  SHA256_Final(&digest[0], &sha256);
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &digest[0], digest.capacity());
  SHA256_Final(&digest[0], &sha256);

  digest.assign(digest.begin(), digest.begin() + 4);
  if (digest != checksum) return bytes_t();
  //  uint8_t version = bytes[0];

  return bytes;
}

// https://en.bitcoin.it/wiki/Technical_background_of_Bitcoin_addresses
std::string Base58::toAddress(const bytes_t& bytes) {
  if (bytes.size() == 0) {
    return std::string();
  }

  bytes_t digest;
  digest.resize(SHA256_DIGEST_LENGTH);

  // 2. Perform SHA-256 hashing on the public key
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &bytes[0], bytes.size());
  SHA256_Final(&digest[0], &sha256);

  // 3. Perform RIPEMD-160 hashing on the result of SHA-256
  bytes_t ripe_digest;
  ripe_digest.resize(RIPEMD160_DIGEST_LENGTH);
  RIPEMD160_CTX ripemd;
  RIPEMD160_Init(&ripemd);
  RIPEMD160_Update(&ripemd, &digest[0], SHA256_DIGEST_LENGTH);
  RIPEMD160_Final(&ripe_digest[0], &ripemd);

  // 4. Add version byte in front of RIPEMD-160 hash (0x00 for Main Network)
  bytes_t version(1, 0);
  ripe_digest.insert(ripe_digest.begin(), version.begin(), version.end());

  return toBase58Check(ripe_digest);
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
