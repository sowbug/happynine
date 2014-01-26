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
#include <istream>
#include <sstream>

#include "base58.h"
#include "credentials.h"
#include "crypto.h"
#include "node.h"
#include "node_factory.h"
#include "wallet.h"

Wallet::Wallet(Credentials& credentials)
  : credentials_(credentials), root_node_(NULL), child_node_(NULL) {
}

Wallet::~Wallet() {
  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    delete i->second;
  }
}

void Wallet::set_root_ext_keys(const bytes_t& ext_pub,
                               const bytes_t& ext_prv_enc) {
  root_ext_pub_ = ext_pub;
  root_ext_prv_enc_ = ext_prv_enc;
}

void Wallet::set_child_ext_keys(const bytes_t& ext_pub,
                                const bytes_t& ext_prv_enc) {
  child_ext_pub_ = ext_pub;
  child_ext_prv_enc_ = ext_prv_enc;
}

bool Wallet::IsWalletLocked() const {
  if (credentials_.isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return true;
  }
  return false;
}


bool Wallet::DeriveRootNode(const bytes_t& seed, bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
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
    if (Crypto::Encrypt(credentials_.ephemeral_key(),
                        node->toSerialized(),
                        ext_prv_enc)) {
      set_root_ext_keys(node->toSerializedPublic(), ext_prv_enc);
      return true;
    }
  }
  return false;
}

bool Wallet::GenerateRootNode(bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
    return false;
  }
  bytes_t seed(32, 0);
  if (!Crypto::GetRandomBytes(seed)) {
    return false;
  }
  return DeriveRootNode(seed, ext_prv_enc);
}

bool Wallet::ImportRootNode(const std::string& ext_prv_b58,
                            bytes_t& ext_prv_enc) {
  if (IsWalletLocked()) {
    return false;
  }
  const bytes_t ext_prv = Base58::fromBase58Check(ext_prv_b58);
  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromExtended(ext_prv));
  if (node.get()) {
    if (Crypto::Encrypt(credentials_.ephemeral_key(),
                        ext_prv,
                        ext_prv_enc)) {
      // TODO: this should call RestoreNode instead. search for
      // all calls to this method & do same
      set_root_ext_keys(node->toSerializedPublic(), ext_prv_enc);
      return true;
    }
  }
  return false;
}

bool Wallet::DeriveChildNode(const std::string& path,
                             bool isWatchOnly,
                             bytes_t& ext_prv_enc) {
  if (!isWatchOnly && IsWalletLocked()) {
    return false;
  }
  if (!hasRootNode()) {
    std::cerr << "no root node" << std::endl;
    return false;
  }

  bool result = true;
  std::auto_ptr<Node> child_node(NodeFactory::
                                 DeriveChildNodeWithPath(*GetRootNode(),
                                                         path));
  if (child_node.get()) {
    const std::string ext_pub_b58 =
      Base58::toBase58Check(child_node->toSerializedPublic());
    if (!isWatchOnly) {
      if (!Crypto::Encrypt(credentials_.ephemeral_key(),
                           child_node->toSerialized(),
                           ext_prv_enc)) {
        result = false;
      }
    }
    if (result) {
      bool is_root;
      RestoreNode(ext_pub_b58, ext_prv_enc, is_root);
    }
  }
  return result;
}

void Wallet::RestoreRootNode(Node* /*node*/) {
  // Nothing to do!
}

void Wallet::RestoreChildNode(Node* node) {
  uint32_t public_address_count = 8;
  uint32_t change_address_count = 8;
  for (uint32_t i = 0; i < public_address_count; ++i) {
    std::stringstream node_path;
    node_path << "m/0/" << i;  // external path
    std::auto_ptr<Node>
      address_node(NodeFactory::DeriveChildNodeWithPath(*node,
                                                        node_path.str()));
    if (address_node.get()) {
      address_status_t as;
      as.hash160 = Base58::toHash160(address_node->public_key());
      as.value = 0;
      as.is_public = true;
      address_statuses_.push_back(as);
      public_addresses_in_wallet_.insert(as.hash160);
    }
  }
  for (uint32_t i = 0; i < change_address_count; ++i) {
    std::stringstream node_path;
    node_path << "m/1/" << i;  // internal path
    std::auto_ptr<Node>
      address_node(NodeFactory::DeriveChildNodeWithPath(*node,
                                                        node_path.str()));
    if (address_node.get()) {
      address_status_t as;
      as.hash160 = Base58::toHash160(address_node->public_key());
      as.value = 0;
      as.is_public = false;
      address_statuses_.push_back(as);
      change_addresses_in_wallet_.insert(as.hash160);
    }
  }
}

bool Wallet::RestoreNode(const std::string& ext_pub_b58,
                         const bytes_t& ext_prv_enc,
                         bool& is_root) {
  const bytes_t ext_pub = Base58::fromBase58Check(ext_pub_b58);
  std::auto_ptr<Node> node;
  node.reset(NodeFactory::CreateNodeFromExtended(ext_pub));
  if (node.get()) {
    if (node->parent_fingerprint() == 0 && node->child_num() == 0) {
      is_root = true;
      set_root_ext_keys(ext_pub, ext_prv_enc);
      RestoreRootNode(node.get());
      GetRootNode();  // groan -- this regenerates the cached node
    } else {
      is_root = false;
      set_child_ext_keys(ext_pub, ext_prv_enc);
      RestoreChildNode(node.get());
      GetChildNode();  // groan -- this regenerates the cached node
    }
    return true;
  }
  return false;
}

Node* Wallet::GetRootNode() {
  if (!hasRootNode()) {
    return NULL;
  }

  if (credentials_.isLocked()) {
    root_node_.reset(NodeFactory::CreateNodeFromExtended(root_ext_pub_));
  } else {
    bytes_t ext_prv;
    if (Crypto::Decrypt(credentials_.ephemeral_key(),
                        root_ext_prv_enc_,
                        ext_prv)) {
      root_node_.reset(NodeFactory::CreateNodeFromExtended(ext_prv));
    } else {
      root_node_.reset();
    }
  }
  return root_node_.get();
}

Node* Wallet::GetChildNode() {
  if (!hasChildNode()) {
    return NULL;
  }

  if (credentials_.isLocked()) {
    child_node_.reset(NodeFactory::CreateNodeFromExtended(child_ext_pub_));
  } else {
    bytes_t ext_prv;
    if (Crypto::Decrypt(credentials_.ephemeral_key(),
                        child_ext_prv_enc_,
                        ext_prv)) {
      child_node_.reset(NodeFactory::CreateNodeFromExtended(ext_prv));
    } else {
      child_node_.reset();
    }
  }
  return child_node_.get();
}

bool Wallet::GetAddressStatusesToReport(address_statuses_t& statuses) {
  statuses = address_statuses_;
  address_statuses_.clear();
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

bool Wallet::IsPublicAddressInWallet(const bytes_t& hash160) {
  return public_addresses_in_wallet_.count(hash160) == 1;
}

bool Wallet::IsChangeAddressInWallet(const bytes_t& hash160) {
  return change_addresses_in_wallet_.count(hash160) == 1;
}

bool Wallet::IsAddressInWallet(const bytes_t& hash160) {
  return IsPublicAddressInWallet(hash160) ||
    IsChangeAddressInWallet(hash160);
}

void Wallet::HandleTx(const bytes_t& tx_bytes) {
  std::istringstream is;
  const char* p = reinterpret_cast<const char*>(&tx_bytes[0]);
  is.rdbuf()->pubsetbuf(const_cast<char*>(p), tx_bytes.size());
  Transaction* tx = new Transaction(is);
  AddTx(tx);

  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_outs_t::const_iterator j = i->second->outputs().begin();
         j != i->second->outputs().end();
         ++j) {
      const bytes_t hash160 = j->GetSigningAddress();
      if (!j->is_spent() && IsAddressInWallet(hash160)) {
        address_status_t address_status;
        address_status.hash160 = hash160;
        address_status.value = j->value();
        address_status.is_public = IsPublicAddressInWallet(hash160);
        address_statuses_.push_back(address_status);
      }
    }
  }
}

bool Wallet::IsAddressUsed(const bytes_t& /*hash160*/) {
  return false;  // TODO(miket): implement
}

bytes_t Wallet::GetNextUnusedChangeAddress() {
  uint32_t i = 0;
  while (true) {
    std::stringstream node_path;
    node_path << "m/1/" << i++;  // internal path
    std::auto_ptr<Node> address_node(NodeFactory::
                                     DeriveChildNodeWithPath(*GetChildNode(),
                                                             node_path.str()));
    if (address_node.get()) {
      const bytes_t hash160 = Base58::toHash160(address_node->public_key());
      if (!IsAddressUsed(hash160)) {
        return hash160;
      }
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

  Transaction transaction;
  int error_code = 0;
  tx = transaction.Sign(*GetChildNode(),  // TODO: should_sign
                        GetUnspentTxos(),
                        recipients,
                        change_txo,
                        fee,
                        error_code);
  return true;
}

tx_outs_t Wallet::GetUnspentTxos() {
  tx_outs_t unspent_txos;

  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_outs_t::const_iterator j = i->second->outputs().begin();
         j != i->second->outputs().end();
         ++j) {
      const bytes_t& address = j->GetSigningAddress();
      if (public_addresses_in_wallet_.count(address) == 1 ||
          change_addresses_in_wallet_.count(address) == 1) {
        if (!j->is_spent()) {
          unspent_txos.push_back(TxOut(j->value(), j->script(),
                                       j->tx_output_n(), i->first));
        }
      }
    }
  }
  return unspent_txos;
}
