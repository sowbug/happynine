#include "wallet.h"

#include "bigint.h"
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
}
