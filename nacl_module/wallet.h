#include "master_key.h"
#include "types.h"

// Portions taken from https://github.com/CodeShark/CoinClasses.
// Copyright (c) 2013 Eric Lombrozo.
class Wallet {
 public:
  Wallet(const MasterKey& master_key);
  virtual ~Wallet();

  bool is_private_key() const {
    return (key_.size() == 33 && key_[0] == 0x00);
  }

  const bytes_t& key() const { return key_; }
  const bytes_t& public_key() const { return public_key_; }
  const bytes_t& fingerprint() const { return fingerprint_; }

 private:
  void set_key(const bytes_t& new_key);
  void update_fingerprint();

  MasterKey master_key_;
  bytes_t key_;
  bytes_t public_key_;
  bytes_t fingerprint_;
};
