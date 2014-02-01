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

#if !defined(__CHILD_NODE_H__)
#define __CHILD_NODE_H__

#include <map>
#include <memory>
#include <string>
#include <set>

#include "tx.h"
#include "types.h"

class AddressWatcher;
class Node;

class ChildNode : public KeyProvider {
 public:
  ChildNode(Node* node, AddressWatcher* address_watcher,
            uint32_t public_address_gap, uint32_t change_address_gap);
  virtual ~ChildNode();

  // Returns hash160.
  bytes_t GetNextUnusedChangeAddress();

  const Node* node() const { return node_; }

  // Indicates that we've seen a transaction for this address.
  void NotifyPublicAddressUsed(uint32_t index);
  void NotifyChangeAddressUsed(uint32_t index);

  uint32_t public_address_count() const { return public_address_count_; }
  uint32_t change_address_count() const { return change_address_count_; }

  bool CreateTx(const tx_outs_t& recipients,
                const tx_outs_t& unspent_txos,
                uint64_t fee,
                bool should_sign,
                bytes_t& tx);

  // KeyProvider overrides
  virtual bool GetKeysForAddress(const bytes_t& hash160,
                                 bytes_t& public_key,
                                 bytes_t& key);

  bool IsAddressWatched(const bytes_t& hash160) const;

 private:
  void GenerateAddressBunch(uint32_t start, uint32_t count,
                            bool is_public);
  void CheckPublicAddressGap(uint32_t index);
  void CheckChangeAddressGap(uint32_t index);
  void ResetGaps();

  void GenerateAllSigningKeys();

  // What we use to generate addresses.
  const Node* node_;
  AddressWatcher* address_watcher_;

  // The size of a new bunch of contiguous addresses.
  const uint32_t public_address_gap_;
  const uint32_t change_address_gap_;

  // The base for address indexes.
  const uint32_t public_address_start_;
  const uint32_t change_address_start_;

  // The number of addresses we've allocated.
  uint32_t public_address_count_;
  uint32_t change_address_count_;

  uint32_t next_change_address_index_;

  std::set<bytes_t> watched_addresses_;

  std::map<bytes_t, bytes_t> signing_public_keys_;
  std::map<bytes_t, bytes_t> signing_keys_;

  DISALLOW_EVIL_CONSTRUCTORS(ChildNode);
};

#endif  // #if __CHILD_NODE_H__
