#include <sstream>
#include <string>

#include "master_key.h"

#include "openssl/hmac.h"
#include "openssl/ripemd.h"
#include "openssl/sha.h"
#include "secp256k1.h"

MasterKey::MasterKey() {
  bytes_t bytes;
  set_key(bytes);
  set_chain_code(bytes);
}

MasterKey::MasterKey(const bytes_t& seed) {
  const std::string BIP0032_HMAC_KEY("Bitcoin seed");
  bytes_t digest;
  digest.resize(EVP_MAX_MD_SIZE);
  HMAC(EVP_sha512(),
       BIP0032_HMAC_KEY.c_str(),
       BIP0032_HMAC_KEY.size(),
       &seed[0],
       seed.size(),
       &digest[0],
       NULL);
  set_key(bytes_t(&digest[0], &digest[32]));
  set_chain_code(bytes_t(&digest[32], &digest[64]));
}

MasterKey::MasterKey(const bytes_t& key, const bytes_t& chain_code) {
  set_key(key);
  set_chain_code(chain_code);
}

MasterKey::~MasterKey() {
}

void MasterKey::set_key(const bytes_t& new_key) {
  secret_key_.clear();
  // TODO(miket): check key_num validity
  is_private_ = (new_key.size() == 32) ||
    (new_key.size() == 33 && new_key[0] == 0x00);
  version_ = is_private_ ? 0x0488ADE4 : 0x0488B21E;
  if (is_private()) {
    if (new_key.size() == 32) {
      secret_key_.push_back(0x00);
    }
    secret_key_.insert(secret_key_.end(), new_key.begin(), new_key.end());
    secp256k1_key curvekey;
    curvekey.setPrivKey(secret_key_);
    public_key_ = curvekey.getPubKey();
  } else {
    public_key_ = new_key;
  }
  update_fingerprint();
}

void MasterKey::set_chain_code(const bytes_t& new_code) {
  chain_code_ = new_code;
}

void MasterKey::update_fingerprint() {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &public_key_[0], public_key_.size());
  SHA256_Final(hash, &sha256);

  unsigned char ripemd_hash[RIPEMD160_DIGEST_LENGTH];
  RIPEMD160_CTX ripemd;
  RIPEMD160_Init(&ripemd);
  RIPEMD160_Update(&ripemd, hash, SHA256_DIGEST_LENGTH);
  RIPEMD160_Final(ripemd_hash, &ripemd);

  fingerprint_ = (uint32_t)ripemd_hash[0] << 24 |
    (uint32_t)ripemd_hash[1] << 16 |
    (uint32_t)ripemd_hash[2] << 8 |
    (uint32_t)ripemd_hash[3];
}

std::string MasterKey::toString() const {
  std::stringstream ss;
  ss << "version: " << std::hex << version_ << std::endl
     << "fingerprint: " << std::hex << fingerprint_ << std::endl
     << "secret_key: " << to_hex(secret_key_) << std::endl
     << "public_key: " << to_hex(public_key_) << std::endl
     << "chain_code: " << to_hex(chain_code_) << std::endl
    ;
  return ss.str();
}
