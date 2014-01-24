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
#include <string>
#include <set>

#include "tx.h"
#include "types.h"

class Credentials;
class Node;

class Wallet {
 public:
  Wallet(Credentials& credentials);
  virtual ~Wallet();

  // Root nodes
  bool DeriveRootNode(const bytes_t& seed, bytes_t& ext_prv_enc);
  bool GenerateRootNode(bytes_t& ext_prv_enc);
  bool ImportRootNode(const std::string& ext_prv_b58, bytes_t& ext_prv_enc);
  // Doesn't need wallet unlocked.
  bool SetRootNode(const std::string& ext_pub_b58, const bytes_t& ext_prv_enc);

  // Child nodes
  bool DeriveChildNode(const std::string& path,
                       bool isWatchOnly,
                       Node** node,
                       bytes_t& ext_prv_enc);
  bool AddChildNode(const std::string& ext_pub_b58,
                    const bytes_t& /*ext_prv_enc*/,
                    uint32_t public_address_count,
                    uint32_t change_address_count);

  // Transactions
  void HandleTxStatus(const bytes_t& hash, uint32_t height);
  void HandleTx(const bytes_t& tx_bytes);
  bool CreateTx(const tx_outs_t& recipients,
                uint64_t fee,
                uint32_t change_index,
                bool should_sign,
                bytes_t& tx);

  // Module-to-client
  typedef struct {
    bytes_t hash160;
    uint64_t value;
    bool is_public;  // false = change address
  } address_status_t;  // TODO: this might just be an unspent txo
  typedef std::vector<address_status_t> address_statuses_t;
  bool GetAddressStatusesToReport(address_statuses_t& statuses);

  typedef bytes_t tx_request_t;
  typedef std::vector<tx_request_t> tx_requests_t;
  bool GetTxRequestsToReport(tx_requests_t& requests);

  // Utilities
  bool hasRootNode() { return !root_ext_prv_enc_.empty(); }

  Node* GetRootNode();   // We retain ownership!
  void ClearRootNode();  // TODO(miket): implement and use

 private:
  void set_root_ext_keys(const bytes_t& ext_pub, const bytes_t& ext_prv_enc);

  bool IsPublicAddressInWallet(const bytes_t& hash160);
  bool IsChangeAddressInWallet(const bytes_t& hash160);
  bool IsAddressInWallet(const bytes_t& hash160);

  void AddTx(const Transaction& transaction);
  bool DoesTxExist(const bytes_t& hash);
  Transaction& GetTx(const bytes_t& hash);
  tx_outs_t GetUnspentTxos();

  Credentials& credentials_;
  bytes_t root_ext_pub_;
  bytes_t root_ext_prv_enc_;
  Node* root_node_;
  address_statuses_t address_statuses_;
  tx_requests_t tx_requests_;

  std::set<bytes_t> public_addresses_in_wallet_;
  std::set<bytes_t> change_addresses_in_wallet_;

  typedef std::map<bytes_t, Transaction> tx_hashes_to_txs_t;
  tx_hashes_to_txs_t tx_hashes_to_txs_;

  DISALLOW_EVIL_CONSTRUCTORS(Wallet);
};

#endif  // #if !defined(__WALLET_H__)
