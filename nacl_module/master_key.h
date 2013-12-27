#include "types.h"

class MasterKey {
 public:
  MasterKey(const bytes_t& seed);

  const bytes_t& secret_key() const { return secret_key_; };
  const bytes_t& chain_code() const { return chain_code_; };

 private:
  bytes_t secret_key_;
  bytes_t chain_code_;
};
