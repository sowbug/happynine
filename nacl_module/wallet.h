#include "master_key.h"
#include "types.h"

class Wallet {
 public:
  Wallet();

  // From a master key.
  explicit Wallet(const MasterKey& master_key);

  // From a serialized key.
  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#serialization-format
  explicit Wallet(const bytes_t& bytes);

  virtual ~Wallet();

  bool GetChildNode(uint32_t i, Wallet& child) const;
  const MasterKey& master_key() const { return master_key_; }

  std::string toString() const;
  bytes_t toSerialized() const;

 private:
  MasterKey master_key_;
  bytes_t public_key_;
  unsigned int depth_;
  uint32_t parent_fingerprint_;
  uint32_t child_num_;
};
