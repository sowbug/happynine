#include "wallet.h"

#include "bigint.h"
#include "openssl/ripemd.h"
#include "openssl/sha.h"
#include "secp256k1.h"

Wallet::Wallet(const MasterKey& master_key) :
  master_key_(master_key) {
  bytes_t secret_key(master_key_.secret_key());
  {
    BigInt key_num(secret_key);
    // TODO(miket): check key_num validity
  }

  bytes_t new_key;
  new_key.push_back(0x00);
  new_key.insert(new_key.end(), secret_key.begin(), secret_key.end());
  set_key(new_key);
}

Wallet::~Wallet() {
}

void Wallet::set_key(const bytes_t& new_key) {
  key_ = new_key;
  if (is_private_key()) {
    secp256k1_key curvekey;
    curvekey.setPrivKey(bytes_t(key_.begin() + 1, key_.end()));
    public_key_ = curvekey.getPubKey();
  } else {
    public_key_ = key_;
  }
  update_fingerprint();
}

void Wallet::update_fingerprint() {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &public_key_[0], 33);
  SHA256_Final(hash, &sha256);

  unsigned char ripemd_hash[RIPEMD160_DIGEST_LENGTH];
  RIPEMD160_CTX ripemd;
  RIPEMD160_Init(&ripemd);
  RIPEMD160_Update(&ripemd, hash, SHA256_DIGEST_LENGTH);
  RIPEMD160_Final(ripemd_hash, &ripemd);

  fingerprint_ = bytes_t(ripemd_hash, ripemd_hash + 4);
}
