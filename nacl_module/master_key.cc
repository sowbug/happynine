#include <string>

#include "master_key.h"

#include "openssl/hmac.h"
#include "openssl/ripemd.h"
#include "openssl/sha.h"

MasterKey::MasterKey(const bytes_t& seed) {
  const std::string BIP0032_HMAC_KEY("Bitcoin seed");
  unsigned char* digest = HMAC(EVP_sha512(),
                               BIP0032_HMAC_KEY.c_str(),
                               BIP0032_HMAC_KEY.size(),
                               &seed[0],
                               seed.size(),
                               NULL,
                               NULL);
  secret_key_ = bytes_t(digest, digest + 32);
  chain_code_ = bytes_t(digest + 32, digest + 64);
}

MasterKey::~MasterKey() {
}
