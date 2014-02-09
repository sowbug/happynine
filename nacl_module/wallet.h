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

#if !defined(__WALLET_H__)
#define __WALLET_H__

#include <map>
#include <memory>
#include <string>
#include <set>

#include "blockchain.h"
#include "tx.h"
#include "types.h"

class Credentials;
class Node;
struct HistoryItem;

class Address {
 public:
  Address(const bytes_t& hash160, uint32_t child_num, bool is_public);
  const bytes_t& hash160() const { return hash160_; }
  uint32_t child_num() const { return child_num_; }
  bool is_public() const { return is_public_; }
  uint64_t balance() const { return balance_; }
  void set_balance(uint64_t balance) { balance_ = balance; }
  uint64_t tx_count() const { return tx_count_; }
  void set_tx_count(uint64_t tx_count) { tx_count_ = tx_count; }

  typedef std::vector<const Address*> addresses_t;
 private:
  const bytes_t hash160_;
  const uint32_t child_num_;
  const bool is_public_;
  uint64_t balance_;
  uint64_t tx_count_;
};

class Wallet : public KeyProvider {
 public:
  Wallet(Blockchain* blockchain, Credentials* credentials,
         const std::string& ext_pub_b58,
         const bytes_t& ext_prv_enc);
  virtual ~Wallet();

  // Root nodes
  static bool DeriveRootNode(Credentials* credentials,
                             const bytes_t& seed, bytes_t& ext_prv_enc);
  static bool GenerateRootNode(Credentials* credentials,
                               bytes_t& ext_prv_enc);
  static bool ImportRootNode(Credentials* credentials,
                             const std::string& ext_prv_b58,
                             bytes_t& ext_prv_enc);

  // Child nodes
  static bool DeriveChildNode(Credentials* credentials,
                              const Node* master_node,
                              const std::string& path,
                              bytes_t& ext_prv_enc);
  static bool DeriveChildNode(const Node* master_node,
                              const std::string& path,
                              std::string& ext_pub_b58);

  // All nodes
  static Node* RestoreNode(Credentials* credentials,
                           const bytes_t& ext_prv_enc);
  static Node* RestoreNode(const std::string& ext_pub_b58);

  uint32_t public_address_count() const { return public_address_count_; }
  uint32_t change_address_count() const { return change_address_count_; }

  // KeyProvider overrides
  bool GetKeysForAddress(const bytes_t& hash160,
                         bytes_t& public_key,
                         bytes_t& key);

  bool CreateTx(const tx_outs_t& recipients,
                uint64_t fee,
                bool should_sign,
                bytes_t& tx);

  // To be called when we know that something changed in the
  // blockchain.
  void UpdateAddressBalancesAndTxCounts();

  void GetAddresses(Address::addresses_t& addresses);
  void GetHistory(history_t& history);

 protected:
  void UpdateAddressBalance(const bytes_t& hash160, uint64_t balance);
  void UpdateAddressTxCount(const bytes_t& hash160, uint64_t tx_count);

 private:
  bytes_t GetNextUnusedChangeAddress();

  bool IsPublicAddressInWallet(const bytes_t& hash160);
  bool IsChangeAddressInWallet(const bytes_t& hash160);
  bool IsAddressInWallet(const bytes_t& hash160);

  void GenerateAddressBunch(uint32_t start, uint32_t count,
                            bool is_public);
  void CheckPublicAddressGap(uint32_t address_index_used);
  void CheckChangeAddressGap(uint32_t address_index_used);
  void ResetGaps();

  void WatchAddress(const bytes_t& hash160,
                    uint32_t child_num,
                    bool is_public);
  bool IsAddressWatched(const bytes_t& hash160);

  void GenerateAllSigningKeys(Node* signing_node);

  void SetCurrentBlock(uint64_t height);

  typedef std::map<bytes_t, Address*> hash_to_address_t;
  hash_to_address_t watched_addresses_;

  Blockchain* blockchain_;
  Credentials* credentials_;
  const std::string ext_pub_b58_;
  const bytes_t ext_prv_enc_;
  std::auto_ptr<Node> watch_only_node_;

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

  std::map<bytes_t, bytes_t> signing_public_keys_;
  std::map<bytes_t, bytes_t> signing_keys_;

  DISALLOW_EVIL_CONSTRUCTORS(Wallet);
};

#endif  // #if !defined(__WALLET_H__)
