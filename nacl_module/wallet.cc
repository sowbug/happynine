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

#include "address.h"
#include "base58.h"
#include "child_node.h"
#include "credentials.h"
#include "crypto.h"
#include "errors.h"
#include "node.h"
#include "node_factory.h"
#include "wallet.h"

Wallet::Wallet(Credentials& credentials)
  : credentials_(credentials), next_node_id_(1) {
}

Wallet::~Wallet() {
  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    delete i->second;
  }
  for (hash_to_address_t::const_iterator i = watched_addresses_.begin();
       i != watched_addresses_.end();
       ++i) {
    delete i->second;
  }
}

// void Wallet::set_master_ext_keys(const bytes_t& ext_pub,
//                                  const bytes_t& ext_prv_enc) {
//   master_ext_pub_ = ext_pub;
//   master_ext_prv_enc_ = ext_prv_enc;
// }

// void Wallet::set_child_ext_keys(const bytes_t& ext_pub,
//                                 const bytes_t& ext_prv_enc) {
//   child_ext_pub_ = ext_pub;
//   child_ext_prv_enc_ = ext_prv_enc;
//   watched_addresses_.clear();
//   ResetGaps();
//   next_change_address_index_ = change_address_start_;
//   addresses_to_report_.clear();
// }

bool Wallet::IsWalletLocked() const {
  if (credentials_.isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return true;
  }
  return false;
}

bool Wallet::DeriveMasterNode(const bytes_t& seed, bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
    return false;
  }

  // If the caller passed in a zero-length seed, as a policy we're going to
  // error out. We don't want a user of this method to forget to supply
  // entropy and ship something that seems to work, but gives the same master
  // node to everyone.
  if (seed.empty()) {
    return false;
  }

  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromSeed(seed));
  if (!node.get()) {
    return false;
  }
  return Crypto::Encrypt(credentials_.ephemeral_key(),
                         node->toSerialized(),
                         ext_prv_enc);
}

bool Wallet::GenerateMasterNode(bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
    return false;
  }
  bytes_t seed(32, 0);
  if (!Crypto::GetRandomBytes(seed)) {
    return false;
  }
  return DeriveMasterNode(seed, ext_prv_enc);
}

bool Wallet::ImportMasterNode(const std::string& ext_prv_b58,
                              bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
    return false;
  }
  const bytes_t ext_prv = Base58::fromBase58Check(ext_prv_b58);
  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromExtended(ext_prv));
  if (!node.get()) {
    return false;
  }
  return Crypto::Encrypt(credentials_.ephemeral_key(),
                         ext_prv,
                         ext_prv_enc);
}

bool Wallet::DeriveChildNode(uint32_t id,
                             uint32_t index,
                             bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
    return false;
  }
  if (index >= 0x80000000) {
    return false;
  }
  std::auto_ptr<Node> master_node(GetMasterNode(id));
  if (!master_node.get()) {
    return false;
  }

  std::stringstream node_path;
  node_path << "m/" << index << "'/";
  std::auto_ptr<Node> node(NodeFactory::
                           DeriveChildNodeWithPath(master_node.get(),
                                                   node_path.str()));
  if (!node.get()) {
    return false;
  }
  return Crypto::Encrypt(credentials_.ephemeral_key(),
                         node->toSerialized(),
                         ext_prv_enc);
}

bool Wallet::DeriveWatchOnlyChildNode(uint32_t id,
                                      const std::string& path,
                                      std::string& ext_pub_b58) {
  std::auto_ptr<Node> master_node(GetMasterNode(id));
  if (!master_node.get()) {
    return false;
  }
  std::auto_ptr<Node>
    node(NodeFactory::DeriveChildNodeWithPath(master_node.get(),
                                              path));
  if (!node.get()) {
    return false;
  }
  ext_pub_b58 = Base58::toBase58Check(node->toSerializedPublic());
  return true;
}

Node* Wallet::GetMasterNode(uint32_t id) {
  if (IsWalletLocked()) {
    return NULL;
  }
  if (ids_to_ext_prv_encs_.count(id) == 0) {
    return NULL;
  }
  bytes_t ext_prv;
  if (!Crypto::Decrypt(credentials_.ephemeral_key(),
                       ids_to_ext_prv_encs_.count(id),
                       ext_prv)) {
    return NULL;
  }
  Node* node(NodeFactory::CreateNodeFromExtended(ext_prv));
  if (!node) {
    return NULL;
  }
  if (node->child_num() != 0) {
    return NULL;
  }
  return node;
}

ChildNode* Wallet::GetChildNode(uint32_t id) {
}

void Wallet::WatchPublicAddress(const bytes_t& /*hash160*/, uint32_t /*child_num*/) {
}

void Wallet::WatchChangeAddress(const bytes_t& /*hash160*/, uint32_t /*child_num*/) {
}

// void Wallet::WatchAddress(const bytes_t& hash160,
//                           uint32_t child_num,
//                           bool is_public) {
//   if (IsAddressWatched(hash160)) {
//     return;
//   }
//   Address* address = new Address(hash160, child_num, is_public);
//   watched_addresses_[hash160] = address;
//   addresses_to_report_.insert(hash160);
// }

void Wallet::UpdateAddressBalance(const bytes_t& hash160, uint64_t balance) {
  // if (!IsAddressWatched(hash160)) {
  //   std::cerr << "oops, update address of unwatched address" << std::endl;
  //   return;
  // }
  if (watched_addresses_[hash160]->balance() != balance) {
    Address* a = watched_addresses_[hash160];
    a->set_balance(balance);
    if (a->is_public()) {
      //      CheckPublicAddressGap(a->child_num());
    } else {
      //      CheckChangeAddressGap(a->child_num());
    }
    addresses_to_report_.insert(hash160);
  }
}

Node* Wallet::GetMasterNode(uint32_t id) { return ids_to_master_nodes_[id]; }
  ChildNode* GetChildNode(uint32_t id) { return ids_to_child_nodes_[id]; }


uint32_t Wallet::AddMasterNode(const bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
    return 0;
  }
  bytes_t ext_prv;
  if (!Crypto::Decrypt(credentials_.ephemeral_key(), ext_prv_enc, ext_prv)) {
    return 0;
  }
  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromExtended(ext_prv));
  if (!node.get()) {
    return 0;
  }
  if (node->child_num() != 0) {
    return 0;
  }
  ids_to_master_nodes_[next_node_id_] = node.release();
  return next_node_id_++;
}

uint32_t Wallet::AddChildNode(const bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
    return 0;
  }
  bytes_t ext_prv;
  if (!Crypto::Decrypt(credentials_.ephemeral_key(), ext_prv_enc, ext_prv)) {
    return 0;
  }
  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromExtended(ext_prv));
  if (!node.get()) {
    return 0;
  }
  if (node->child_num() == 0) {
    return 0;
  }
  ChildNode* child_node = new ChildNode(node.release(), this, 4, 4);
  ids_to_child_nodes_[next_node_id_] = child_node;
  return next_node_id_++;
}

uint32_t Wallet::AddChildNode(const std::string& ext_pub_b58) {
  const bytes_t ext_pub = Base58::fromBase58Check(ext_pub_b58);
  if (ext_pub.empty()) {
    return false;
  }
  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromExtended(ext_pub));
  if (!node.get()) {
    return 0;
  }
  ChildNode* child_node = new ChildNode(node.release(), this, 4, 4);
  ids_to_child_nodes_[next_node_id_] = child_node;
  return next_node_id_++;
}

static bool SortAddresses(const Address* a, const Address* b) {
  if (a->is_public() != b->is_public()) {
    return a->is_public() > b->is_public();
  }
  return a->child_num() < b->child_num();
}

bool Wallet::GetAddressStatusesToReport(Address::addresses_t& statuses) {
  for (std::set<bytes_t>::const_iterator i = addresses_to_report_.begin();
       i != addresses_to_report_.end();
       ++i) {
    statuses.push_back(watched_addresses_[*i]);
  }
  std::sort(statuses.begin(), statuses.end(), SortAddresses);
  addresses_to_report_.clear();
  return true;
}

bool Wallet::GetTxRequestsToReport(tx_requests_t& requests) {
  requests = tx_requests_;
  tx_requests_.clear();
  return true;
}

void Wallet::HandleTxStatus(const bytes_t& hash, uint32_t /*height*/) {
  tx_requests_.push_back(hash);
}

bool Wallet::DoesTxExist(const bytes_t& hash) {
  return tx_hashes_to_txs_.count(hash) == 1;
}

Transaction* Wallet::GetTx(const bytes_t& hash) {
  return tx_hashes_to_txs_[hash];
}

void Wallet::AddTx(Transaction* transaction) {
  // Stick it in the map.
  tx_hashes_to_txs_[transaction->hash()] = transaction;

  // Check every input to see which output it spends, and if we know
  // about that output, mark it spent.
  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_ins_t::const_iterator j = i->second->inputs().begin();
         j != i->second->inputs().end();
         ++j) {
      if (DoesTxExist(j->prev_txo_hash())) {
        Transaction* affected_tx = GetTx(j->prev_txo_hash());
        affected_tx->MarkOutputSpent(j->prev_txo_index());
      }
    }
  }
}

void Wallet::HandleTx(const bytes_t& tx_bytes) {
  std::istringstream is;
  const char* p = reinterpret_cast<const char*>(&tx_bytes[0]);
  is.rdbuf()->pubsetbuf(const_cast<char*>(p), tx_bytes.size());
  Transaction* tx = new Transaction(is);
  AddTx(tx);

  std::map<bytes_t, uint64_t> balances;
  for (hash_to_address_t::const_iterator i = watched_addresses_.begin();
       i != watched_addresses_.end();
       ++i) {
    balances[i->first] = 0;
  }

  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_outs_t::const_iterator j = i->second->outputs().begin();
         j != i->second->outputs().end();
         ++j) {
      const bytes_t hash160 = j->GetSigningAddress();
      if (IsAddressWatched(hash160)) {
        if (!j->is_spent()) {
          balances[hash160] += j->value();
        }
      }
    }
  }

  for (std::map<bytes_t, uint64_t>::const_iterator i = balances.begin();
       i != balances.end();
       ++i) {
    UpdateAddressBalance(i->first, i->second);
  }
}

bytes_t Wallet::GetNextUnusedChangeAddress() {
  // TODO(miket): we should probably increment
  // next_change_address_index_ here, because a transaction might take
  // a while to confirm, and if the user issues several transactions
  // in a row, they'll all go to the same change address.
  std::stringstream node_path;
  node_path << "m/1/" << next_change_address_index_;  // internal path
  std::auto_ptr<Node> address_node(NodeFactory::
                                   DeriveChildNodeWithPath(*GetChildNode(),
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

void Wallet::GenerateAllSigningKeys() {
  for (uint32_t i = public_address_start_;
       i < public_address_start_ + public_address_count_;
       ++i) {
    std::stringstream node_path;
    node_path << "m/0/" << i;  // external path
    std::auto_ptr<Node> node(NodeFactory::
                             DeriveChildNodeWithPath(*GetChildNode(),
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
                             DeriveChildNodeWithPath(*GetChildNode(),
                                                     node_path.str()));
    if (node.get()) {
      bytes_t hash160(Base58::toHash160(node->public_key()));
      signing_public_keys_[hash160] = node->public_key();
      signing_keys_[hash160] = node->secret_key();
    }
  }
}

bool Wallet::CreateTx(const tx_outs_t& recipients,
                      uint64_t fee,
                      bool should_sign,
                      bytes_t& tx) {
  if (should_sign && IsWalletLocked()) {
    return false;
  }
  TxOut change_txo(0, GetNextUnusedChangeAddress());

  GenerateAllSigningKeys();

  Transaction transaction;
  int error_code = 0;
  // TODO: should_sign
  tx = transaction.Sign(this,
                        GetUnspentTxos(),
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

  // std::map<bytes_t, uint64_t> balances;
  // for (hash_to_address_t::const_iterator i = watched_addresses_.begin();
  //      i != watched_addresses_.end();
  //      ++i) {
  //   balances[i->first] = 0;
  // }

  // for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
  //      i != tx_hashes_to_txs_.end();
  //      ++i) {
  //   for (tx_outs_t::const_iterator j = i->second->outputs().begin();
  //        j != i->second->outputs().end();
  //        ++j) {
  //     const bytes_t hash160 = j->GetSigningAddress();
  //     if (IsAddressWatched(hash160)) {
  //       if (!j->is_spent()) {
  //         balances[hash160] += j->value();
  //       }
  //     }
  //   }
  // }

  // for (std::map<bytes_t, uint64_t>::const_iterator i = balances.begin();
  //      i != balances.end();
  //      ++i) {
  //   UpdateAddressBalance(i->first, i->second);
  // }
}

bool Wallet::CreateTx(ChildNode* node,
                      const tx_outs_t& recipients,
                      uint64_t fee,
                      bool should_sign,
                      bytes_t& tx) {
  tx_outs_t unspent_txos = GetUnspentTxos(node);
  return node->CreateTx(recipients, unspent_txos, fee, should_sign, tx);
}

tx_outs_t Wallet::GetUnspentTxos(const ChildNode* node) {
  tx_outs_t unspent_txos;

  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_outs_t::const_iterator j = i->second->outputs().begin();
         j != i->second->outputs().end();
         ++j) {
      const bytes_t& address = j->GetSigningAddress();
      if (node->IsAddressWatched(address)) {
        if (!j->is_spent()) {
          unspent_txos.push_back(TxOut(j->value(), j->script(),
                                       j->tx_output_n(), i->first));
        }
      }
    }
  }
  return unspent_txos;
}
