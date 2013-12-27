#include "master_key.h"
#include "types.h"

class Wallet {
 public:
  Wallet(const MasterKey& master_key);
  virtual ~Wallet();

  bool GetChildNode(uint32_t i, Wallet& child) const;
  const MasterKey& master_key() const { return master_key_; }

  std::string toString() const;

 private:
  MasterKey master_key_;
  bytes_t public_key_;
  uint32_t version_;
  unsigned char depth_;
  uint32_t parent_fingerprint_;
  uint32_t child_num_;
};
