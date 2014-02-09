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

#include <iostream>  // cerr

#include <algorithm>
#include <istream>
#include <sstream>

#include "base58.h"
#include "blockchain.h"
#include "credentials.h"
#include "crypto.h"
#include "errors.h"
#include "node.h"
#include "node_factory.h"
#include "wallet.h"

Address::Address(const bytes_t& hash160, uint32_t child_num, bool is_public)
  : hash160_(hash160), child_num_(child_num), is_public_(is_public),
    balance_(0) {
}

Wallet::Wallet(Blockchain* blockchain,
               Credentials* credentials,
               const std::string& ext_pub_b58,
               const bytes_t& ext_prv_enc)
  : blockchain_(blockchain), credentials_(credentials),
    ext_pub_b58_(ext_pub_b58), ext_prv_enc_(ext_prv_enc),
    watch_only_node_(RestoreNode(ext_pub_b58_)),
    public_address_gap_(4), change_address_gap_(4),
    public_address_start_(0), change_address_start_(0),
    next_change_address_index_(change_address_start_) {
  ResetGaps();
  CheckPublicAddressGap(0);
  CheckChangeAddressGap(0);
}

void Wallet::ResetGaps() {
  public_address_count_ = 0;
  change_address_count_ = 0;
}

Wallet::~Wallet() {
  for (hash_to_address_t::const_iterator i = watched_addresses_.begin();
       i != watched_addresses_.end();
       ++i) {
    delete i->second;
  }
}

bool Wallet::DeriveRootNode(Credentials* credentials,
                            const bytes_t& seed,
                            bytes_t& ext_prv_enc) {
  if (credentials->isLocked()) {
    return false;
  }

  // If the caller passed in a zero-length seed, as a policy we're going to
  // error out. We don't want a user of this method to forget to supply
  // entropy and ship something that seems to work, but gives the same root
  // node to everyone.
  if (seed.empty()) {
    return false;
  }

  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromSeed(seed));
  if (node.get()) {
    if (Crypto::Encrypt(credentials->ephemeral_key(),
                        node->toSerialized(),
                        ext_prv_enc)) {
      return true;
    }
  }
  return false;
}

bool Wallet::GenerateRootNode(Credentials* credentials,
                              bytes_t& ext_prv_enc) {
  if (credentials->isLocked()) {
    return false;
  }
  bytes_t seed(32, 0);
  if (!Crypto::GetRandomBytes(seed)) {
    return false;
  }
  return DeriveRootNode(credentials, seed, ext_prv_enc);
}

bool Wallet::ImportRootNode(Credentials* credentials,
                            const std::string& ext_prv_b58,
                            bytes_t& ext_prv_enc) {
  if (credentials->isLocked()) {
    return false;
  }
  const bytes_t ext_prv = Base58::fromBase58Check(ext_prv_b58);
  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromExtended(ext_prv));
  if (node.get()) {
    if (Crypto::Encrypt(credentials->ephemeral_key(),
                        ext_prv,
                        ext_prv_enc)) {
      return true;
    }
  }
  return false;
}

bool Wallet::DeriveChildNode(Credentials* credentials,
                             const Node* master_node,
                             const std::string& path,
                             bytes_t& ext_prv_enc) {
  if (credentials->isLocked()) {
    return false;
  }

  std::auto_ptr<Node> node(NodeFactory::
                           DeriveChildNodeWithPath(*master_node, path));
  if (node.get()) {
    if (Crypto::Encrypt(credentials->ephemeral_key(),
                        node->toSerialized(),
                        ext_prv_enc)) {
      return true;
    }
  }
  return false;
}

bool Wallet::DeriveChildNode(const Node* master_node,
                             const std::string& path,
                             std::string& ext_pub_b58) {
  std::auto_ptr<Node> node(NodeFactory::
                           DeriveChildNodeWithPath(*master_node, path));
  if (node.get()) {
    ext_pub_b58 = Base58::toBase58Check(node->toSerializedPublic());
    return true;
  }
  return false;
}

void Wallet::WatchAddress(const bytes_t& hash160,
                          uint32_t child_num,
                          bool is_public) {
  if (IsAddressWatched(hash160)) {
    return;
  }
  Address* address = new Address(hash160, child_num, is_public);
  watched_addresses_[hash160] = address;
}

bool Wallet::IsAddressWatched(const bytes_t& hash160) {
  return watched_addresses_.count(hash160) == 1;
}

void Wallet::UpdateAddressBalance(const bytes_t& hash160, uint64_t balance) {
  if (!IsAddressWatched(hash160)) {
    std::cerr << "oops, update balance of unwatched address" << std::endl;
    return;
  }
  if (watched_addresses_[hash160]->balance() != balance) {
    Address* a = watched_addresses_[hash160];
    a->set_balance(balance);
    if (a->is_public()) {
      CheckPublicAddressGap(a->child_num());
    } else {
      CheckChangeAddressGap(a->child_num());
    }
  }
}

void Wallet::UpdateAddressTxCount(const bytes_t& hash160, uint64_t tx_count) {
  if (!IsAddressWatched(hash160)) {
    std::cerr << "oops, update tx_count of unwatched address" << std::endl;
    return;
  }
  if (watched_addresses_[hash160]->tx_count() != tx_count) {
    Address* a = watched_addresses_[hash160];
    a->set_tx_count(tx_count);
    if (a->is_public()) {
      CheckPublicAddressGap(a->child_num());
    } else {
      CheckChangeAddressGap(a->child_num());
    }
  }
}

void Wallet::GenerateAddressBunch(uint32_t start, uint32_t count,
                                  bool is_public) {
  const std::string& path_prefix = is_public ?
    "m/0/" : // external path
    "m/1/";  // internal path
  for (uint32_t i = start; i < start + count; ++i) {
    std::stringstream node_path;
    node_path << path_prefix << i;
    std::auto_ptr<Node>
      address_node(NodeFactory::DeriveChildNodeWithPath(*watch_only_node_,
                                                        node_path.str()));
    if (address_node.get()) {
      WatchAddress(Base58::toHash160(address_node->public_key()),
                   i, is_public);
    }
  }
}

void Wallet::CheckPublicAddressGap(uint32_t address_index_used) {
  // Given the highest address we've used, should we allocate a new
  // bunch of addresses?
  uint32_t desired_count = address_index_used + public_address_gap_ -
    public_address_start_ + 1;
  if (desired_count > public_address_count_) {
    // Yes, it's time to allocate.
    GenerateAddressBunch(public_address_start_ + public_address_count_,
                         public_address_gap_, true);
    public_address_count_ += public_address_gap_;
  }
}

void Wallet::CheckChangeAddressGap(uint32_t address_index_used) {
  // Given the highest address we've used, should we allocate a new
  // bunch of addresses?
  uint32_t desired_count = address_index_used + change_address_gap_ -
    change_address_start_ + 1;
  if (desired_count > change_address_count_) {
    // Yes, it's time to allocate.
    GenerateAddressBunch(change_address_start_ + change_address_count_,
                         change_address_gap_, false);
    change_address_count_ += change_address_gap_;
  }
  if (address_index_used >= next_change_address_index_) {
    next_change_address_index_ = address_index_used + 1;
    if (next_change_address_index_ < change_address_start_) {
      next_change_address_index_ = change_address_start_;
    }
  }
}

Node* Wallet::RestoreNode(Credentials* credentials,
                          const bytes_t& ext_prv_enc) {
  bytes_t ext_prv;
  if (!Crypto::Decrypt(credentials->ephemeral_key(),
                       ext_prv_enc,
                       ext_prv)) {
    return NULL;
  }
  return NodeFactory::CreateNodeFromExtended(ext_prv);
}

Node* Wallet::RestoreNode(const std::string& ext_pub_b58) {
  const bytes_t ext_pub = Base58::fromBase58Check(ext_pub_b58);
  return NodeFactory::CreateNodeFromExtended(ext_pub);
}

bytes_t Wallet::GetNextUnusedChangeAddress() {
  // TODO(miket): we should probably increment
  // next_change_address_index_ here, because a transaction might take
  // a while to confirm, and if the user issues several transactions
  // in a row, they'll all go to the same change address.
  std::stringstream node_path;
  node_path << "m/1/" << next_change_address_index_;  // internal path
  std::auto_ptr<Node> address_node(NodeFactory::
                                   DeriveChildNodeWithPath(*watch_only_node_,
                                                           node_path.str()));
  if (address_node.get()) {
    return Base58::toHash160(address_node->public_key());
  }
  return bytes_t();
}

bool Wallet::GetKeysForAddress(const bytes_t& hash160,
                               bytes_t& public_key,
                               bytes_t& key) {
  if (signing_keys_.count(hash160) == 0) {
    return false;
  }
  public_key = signing_public_keys_[hash160];
  key = signing_keys_[hash160];
  return true;
}

void Wallet::GenerateAllSigningKeys(Node* signing_node) {
  for (uint32_t i = public_address_start_;
       i < public_address_start_ + public_address_count_;
       ++i) {
    std::stringstream node_path;
    node_path << "m/0/" << i;  // external path
    std::auto_ptr<Node> node(NodeFactory::
                             DeriveChildNodeWithPath(*signing_node,
                                                     node_path.str()));
    if (node.get()) {
      bytes_t hash160(Base58::toHash160(node->public_key()));
      signing_public_keys_[hash160] = node->public_key();
      signing_keys_[hash160] = node->secret_key();
    }
  }
  for (uint32_t i = change_address_start_;
       i < change_address_start_ + change_address_count_;
       ++i) {
    std::stringstream node_path;
    node_path << "m/1/" << i;  // internal path
    std::auto_ptr<Node> node(NodeFactory::
                             DeriveChildNodeWithPath(*signing_node,
                                                     node_path.str()));
    if (node.get()) {
      bytes_t hash160(Base58::toHash160(node->public_key()));
      signing_public_keys_[hash160] = node->public_key();
      signing_keys_[hash160] = node->secret_key();
    }
  }
}

// static bool SortRecentTransactions(const Wallet::recent_tx_t& a,
//                                    const Wallet::recent_tx_t& b) {
//   if (a.timestamp != b.timestamp) {
//     return a.timestamp > b.timestamp;
//   }
//   return a.value > b.value;
// }

// bool Wallet::GetRecentTransactionsToReport(recent_txs_t& recent_txs) {
//   for (std::set<bytes_t>::const_iterator i = recent_txs_to_report_.begin();
//        i != recent_txs_to_report_.end();
//        ++i) {
//     Transaction* tx = blockchain_->

//     // Check the inputs. If we recognize the sender, add it.
//     for (tx_ins_t::const_iterator j = tx->inputs().begin();
//          j != tx->inputs().end();
//          ++j) {
//       if (!DoesTxExist(j->prev_txo_hash())) {
//         continue;
//       }
//       Transaction* spent_tx = GetTx(j->prev_txo_hash());
//       if (!spent_tx) {
//         continue;
//       }
//       const TxOut* txo = &spent_tx->outputs()[j->prev_txo_index()];
//       const bytes_t& address = txo->GetSigningAddress();
//       if (IsAddressWatched(address)) {
//         recent_tx_t rtx;
//         rtx.hash = spent_tx->hash();
//         rtx.hash160 = address;
//         rtx.value = txo->value();
//         rtx.was_received = false;
//         rtx.timestamp = GetTxTimestamp(rtx.hash);
//         recent_txs.push_back(rtx);
//       }
//     }

//     // Check the outputs. If we recognize the recipient, add it.
//     for (tx_outs_t::const_iterator j = tx->outputs().begin();
//          j != tx->outputs().end();
//          ++j) {
//       const bytes_t& address = j->GetSigningAddress();
//       if (IsAddressWatched(address)) {
//         recent_tx_t rtx;
//         rtx.hash = tx->hash();
//         rtx.hash160 = address;
//         rtx.value = j->value();
//         rtx.was_received = true;
//         rtx.timestamp = GetTxTimestamp(rtx.hash);
//         recent_txs.push_back(rtx);
//       }
//     }
//   }
//   std::sort(recent_txs.begin(), recent_txs.end(), SortRecentTransactions);
//   recent_txs_to_report_.clear();
//   return true;
// }

bool Wallet::CreateTx(const tx_outs_t& recipients,
                      uint64_t fee,
                      bool should_sign,
                      bytes_t& tx) {
  if (should_sign && credentials_->isLocked()) {
    return false;
  }
  TxOut change_txo(0, GetNextUnusedChangeAddress());

  std::auto_ptr<Node> signing_node(RestoreNode(credentials_, ext_prv_enc_));
  GenerateAllSigningKeys(signing_node.get());

  Blockchain::address_set_t addresses;
  tx_outs_t unspent_txos;
  for (hash_to_address_t::const_iterator i = watched_addresses_.begin();
       i != watched_addresses_.end();
       ++i) {
    addresses.insert(i->first);
  }
  blockchain_->GetUnspentTxos(addresses, unspent_txos);

  Transaction transaction;
  int error_code = 0;
  // TODO: should_sign
  tx = transaction.Sign(this,
                        unspent_txos,
                        recipients,
                        change_txo,
                        fee,
                        error_code);
  if (error_code != ERROR_NONE) {
    std::cerr << "CreateTx failed: " << error_code << std::endl;
  }

  signing_public_keys_.clear();
  signing_keys_.clear();

  return error_code == ERROR_NONE;
}

static bool SortAddresses(const Address* a, const Address* b) {
  if (a->is_public() != b->is_public()) {
    return a->is_public() > b->is_public();
  }
  return a->child_num() < b->child_num();
}

void Wallet::GetAddresses(Address::addresses_t& addresses) {
  addresses.clear();

  for (hash_to_address_t::const_iterator i = watched_addresses_.begin();
       i != watched_addresses_.end();
       ++i) {
    i->second->set_balance(blockchain_->GetAddressBalance(i->first));
    i->second->set_tx_count(blockchain_->GetAddressTxCount(i->first));
    addresses.push_back(i->second);
  }
  std::sort(addresses.begin(), addresses.end(), SortAddresses);
}
