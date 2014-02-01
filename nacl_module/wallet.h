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

#include "address_watcher.h"
#include "tx.h"
#include "types.h"

class Address;
class ChildNode;
class Credentials;
class Node;

class Wallet : public KeyProvider, AddressWatcher {
 public:
  Wallet(Credentials& credentials);
  virtual ~Wallet();

  // OVERVIEW OF NODES
  //
  // A master node generates child nodes. A child node generates
  // addresses.
  //
  // When creating, deriving, or importing a node, the result is
  // always an extended, encrypted private key (ext_prv_enc).
  //
  // When restoring a node, the input is either an ext_pub_b58, which
  // is a Base58-encoded BIP 0032 extended public key, or else the
  // ext_prv_enc returned from any of the create/derive/import
  // methods.

  // Create, derive, or import a node. The caller should serialize the
  // result somewhere safe, and then add the result to the wallet.
  bool DeriveMasterNode(const bytes_t& seed, bytes_t& ext_prv_enc);
  bool GenerateMasterNode(bytes_t& ext_prv_enc);
  bool ImportMasterNode(const std::string& ext_prv_b58, bytes_t& ext_prv_enc);
  bool DeriveChildNode(uint32_t id, uint32_t index, bytes_t& ext_prv_enc);
  bool DeriveWatchOnlyChildNode(uint32_t id,
                                const std::string& path,
                                std::string& ext_pub_b58);

  // Add a node to the wallet. Result is zero if failure, or
  // else a temporary ID that is used to refer to the node in the
  // future.
  uint32_t AddNode(const bytes_t& ext_prv_enc);
  uint32_t AddNode(const std::string& ext_pub_b58);

  // Blocks
  void HandleBlockHeader();

  // Transactions
  void HandleTxStatus(const bytes_t& hash, uint32_t height);
  void HandleTx(const bytes_t& tx_bytes);
  bool CreateTx(ChildNode* node,
                const tx_outs_t& recipients,
                uint64_t fee,
                bool should_sign,
                bytes_t& tx);

  // Module-to-client
  bool GetAddressStatusesToReport(Address::addresses_t& statuses);

  typedef bytes_t tx_request_t;
  typedef std::vector<tx_request_t> tx_requests_t;
  bool GetTxRequestsToReport(tx_requests_t& requests);

  // AddressWatcher overrides
  virtual void WatchPublicAddress(const bytes_t& hash160, uint32_t child_num);
  virtual void WatchChangeAddress(const bytes_t& hash160, uint32_t child_num);

  // KeyProvider overrides
  bool GetKeysForAddress(const bytes_t& hash160,
                         bytes_t& public_key,
                         bytes_t& key);

 private:
  void set_master_ext_keys(const bytes_t& ext_pub, const bytes_t& ext_prv_enc);
  void set_child_ext_keys(const bytes_t& ext_pub, const bytes_t& ext_prv_enc);

  bool IsPublicAddressInWallet(const bytes_t& hash160);
  bool IsChangeAddressInWallet(const bytes_t& hash160);
  bool IsAddressInWallet(const bytes_t& hash160);

  void AddTx(Transaction* transaction);
  bool DoesTxExist(const bytes_t& hash);
  Transaction* GetTx(const bytes_t& hash);
  tx_outs_t GetUnspentTxos(const ChildNode* node);

  bool IsWalletLocked() const;

  void RestoreMasterNode(const Node* node);
  void RestoreChildNode(const Node* node);

  void UpdateAddressBalance(const bytes_t& hash160, uint64_t balance);

  void GenerateAllSigningKeys();

  typedef std::map<bytes_t, Address*> hash_to_address_t;
  hash_to_address_t watched_addresses_;
  std::set<bytes_t> addresses_to_report_;

  Node* GetMasterNode(uint32_t id);
  ChildNode* GetChildNode(uint32_t id);

  Credentials& credentials_;

  uint32_t next_node_id_;
  typedef std::map<uint32_t, bytes_t> ext_prv_enc_map_t;
  ext_prv_enc_map_t ids_to_ext_prv_encs_;
  typedef std::map<uint32_t, std::string> ext_pub_b58_map_t;
  ext_pub_b58_map_t ids_to_ext_pub_b58s_;

  typedef std::map<bytes_t, Address*> hash_to_address_t;
  hash_to_address_t watched_addresses_;
  std::set<bytes_t> addresses_to_report_;

  tx_requests_t tx_requests_;

  std::map<bytes_t, bytes_t> signing_public_keys_;
  std::map<bytes_t, bytes_t> signing_keys_;

  typedef std::map<bytes_t, Transaction*> tx_hashes_to_txs_t;
  tx_hashes_to_txs_t tx_hashes_to_txs_;

  DISALLOW_EVIL_CONSTRUCTORS(Wallet);
};

#endif  // #if !defined(__WALLET_H__)
