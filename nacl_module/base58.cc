#include "base58.h"

#include "bigint.h"
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
