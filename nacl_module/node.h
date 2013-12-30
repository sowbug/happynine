#include "types.h"

class Node {
 public:
  Node(const bytes_t& key,
       const bytes_t& chain_code,
       uint32_t version,
       unsigned int depth,
       uint32_t parent_fingerprint,
       uint32_t child_num);
  virtual ~Node();

  bool is_private() const { return is_private_; }
  const uint32_t version() const { return version_; }
  const bytes_t& secret_key() const { return secret_key_; }
  const bytes_t& public_key() const { return public_key_; }
  const uint32_t fingerprint() const { return fingerprint_; }
  const unsigned int depth() const { return depth_; }
  const bytes_t& chain_code() const { return chain_code_; }

  std::string toString() const;
  bytes_t toSerialized() const;

 private:
  void set_key(const bytes_t& new_key);
  void set_chain_code(const bytes_t& new_code);
  void update_fingerprint();

  bool is_private_;
  uint32_t version_;
  bytes_t secret_key_;
  bytes_t public_key_;
  bytes_t chain_code_;
  uint32_t fingerprint_;
  unsigned int depth_;
  uint32_t parent_fingerprint_;
  uint32_t child_num_;
};
