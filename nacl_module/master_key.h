#include <string>

#include "types.h"

class MasterKey {
 public:
  MasterKey();
  MasterKey(const bytes_t& seed);
  MasterKey(const bytes_t& key, const bytes_t& chain_code);
  virtual ~MasterKey();

  const bytes_t& secret_key() const { return secret_key_; }
  const bytes_t& public_key() const { return public_key_; }
  const uint32_t fingerprint() const { return fingerprint_; }
  const bytes_t& chain_code() const { return chain_code_; }

  bool is_private() const { return is_private_; }

  std::string toString() const;

 private:
  void set_key(const bytes_t& new_key);
  void set_chain_code(const bytes_t& new_code);
  void update_fingerprint();

  bool is_private_;
  bytes_t secret_key_;
  bytes_t public_key_;
  bytes_t chain_code_;
  uint32_t fingerprint_;
};
