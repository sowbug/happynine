// Copyright 2014 Mike Tsao <mike@sowbug.com>

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
  uint32_t version() const { return version_; }
  const bytes_t& hex_id() const { return hex_id_; }
  uint32_t fingerprint() const { return fingerprint_; }
  const bytes_t& secret_key() const { return secret_key_; }
  const bytes_t& public_key() const { return public_key_; }
  const bytes_t& chain_code() const { return chain_code_; }
  unsigned int depth() const { return depth_; }
  uint32_t parent_fingerprint() const { return parent_fingerprint_; }
  uint32_t child_num() const { return child_num_; }

  std::string toString() const;
  bytes_t toSerialized(bool public_if_available) const;
  bytes_t toSerialized() const;
  bytes_t toSerializedPublic() const;
  bytes_t toSerializedPrivate() const;

 private:
  void set_key(const bytes_t& new_key);
  void set_chain_code(const bytes_t& new_code);
  void update_fingerprint();

  bool is_private_;
  uint32_t version_;
  bytes_t hex_id_;
  uint32_t fingerprint_;
  bytes_t secret_key_;
  bytes_t public_key_;
  bytes_t chain_code_;
  unsigned int depth_;
  uint32_t parent_fingerprint_;
  uint32_t child_num_;
};
