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
  return bytes_t(5);
}
