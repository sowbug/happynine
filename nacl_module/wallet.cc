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
  : credentials_(credentials), root_node_(NULL) {
}

Wallet::~Wallet() {
  delete root_node_;
}

void Wallet::set_root_ext_keys(const bytes_t& ext_pub,
                               const bytes_t& ext_prv_enc) {
  root_ext_pub_ = ext_pub;
  root_ext_prv_enc_ = ext_prv_enc;
}

bool Wallet::DeriveRootNode(const bytes_t& seed, bytes_t& ext_prv_enc) {
  if (credentials_.isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }

  Node* node = NodeFactory::CreateNodeFromSeed(seed);
  if (node) {
    if (Crypto::Encrypt(credentials_.ephemeral_key(),
                        node->toSerialized(),
                        ext_prv_enc)) {
      set_root_ext_keys(node->toSerializedPublic(), ext_prv_enc);
      return true;
    }
    delete node;
  }
  return false;
}

bool Wallet::GenerateRootNode(bytes_t& ext_prv_enc) {
  if (credentials_.isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }
  bytes_t seed(32, 0);
  if (!Crypto::GetRandomBytes(seed)) {
    return false;
  }
  return DeriveRootNode(seed, ext_prv_enc);
}

bool Wallet::SetRootNode(const std::string& ext_pub_b58,
                         const bytes_t& ext_prv_enc) {
  const bytes_t ext_pub = Base58::fromBase58Check(ext_pub_b58);
  Node* node = NodeFactory::CreateNodeFromExtended(ext_pub);
  if (node) {
    set_root_ext_keys(node->toSerializedPublic(), ext_prv_enc);
    delete node;
    return true;
  }
  return false;
}

bool Wallet::ImportRootNode(const std::string& ext_prv_b58,
                            bytes_t& ext_prv_enc) {
  if (credentials_.isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }

  const bytes_t ext_prv = Base58::fromBase58Check(ext_prv_b58);
  Node* node = NodeFactory::CreateNodeFromExtended(ext_prv);
  if (node) {
    if (Crypto::Encrypt(credentials_.ephemeral_key(),
                        ext_prv,
                        ext_prv_enc)) {
      set_root_ext_keys(node->toSerializedPublic(), ext_prv_enc);
      return true;
    }
    delete node;
  }
  return false;
}

bool Wallet::DeriveChildNode(const std::string& path,
                             bool isWatchOnly,
                             Node** node,
                             bytes_t& ext_prv_enc) {
  if (!isWatchOnly && credentials_.isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }
  if (!hasRootNode()) {
    std::cerr << "no root node" << std::endl;
    return false;
  }

  GetRootNode();  // TODO: hokey

  *node = NodeFactory::DeriveChildNodeWithPath(*root_node_, path);
  if (*node) {
    if (isWatchOnly) {
      AddChildNode(Base58::toBase58Check((*node)->toSerializedPublic()),
                   bytes_t(),
                   8,
                   8);
      return true;
    } else {
      if (Crypto::Encrypt(credentials_.ephemeral_key(),
                          (*node)->toSerialized(),
                          ext_prv_enc)) {
        AddChildNode(Base58::toBase58Check((*node)->toSerializedPublic()),
                     ext_prv_enc,
                     8,
                     8);
        return true;
      }
    }
  }
  return false;
}

bool Wallet::AddChildNode(const std::string& ext_pub_b58,
                          const bytes_t& /*ext_prv_enc*/,
                          uint32_t public_address_count,
                          uint32_t change_address_count) {
  const bytes_t ext_pub = Base58::fromBase58Check(ext_pub_b58);
  Node* child_node = NodeFactory::CreateNodeFromExtended(ext_pub);
  if (child_node) {
    for (uint32_t i = 0; i < public_address_count; ++i) {
      std::string node_path("m/0/");
      node_path += i;
      Node* address_node =
        NodeFactory::DeriveChildNodeWithPath(*child_node, node_path);
      if (address_node) {
        address_status_t as;
        as.hash160 = Base58::toHash160(address_node->public_key());
        as.value = 0;
        as.is_public = true;
        address_statuses_.push_back(as);
        public_addresses_in_wallet_.insert(as.hash160);
        delete address_node;
      }
    }
    for (uint32_t i = 0; i < change_address_count; ++i) {
      std::string node_path("m/1/");
      node_path += i;
      Node* address_node =
        NodeFactory::DeriveChildNodeWithPath(*child_node, node_path);
      if (address_node) {
        address_status_t as;
        as.hash160 = Base58::toHash160(address_node->public_key());
        as.value = 0;
        as.is_public = false;
        address_statuses_.push_back(as);
        change_addresses_in_wallet_.insert(as.hash160);
        delete address_node;
      }
    }
    delete child_node;
    // TODO(miket): save ext_prv_enc somewhere
    return true;
  }
  return false;
}

Node* Wallet::GetRootNode() {
  if (!hasRootNode()) {
    return NULL;
  }

  delete root_node_;
  if (credentials_.isLocked()) {
    root_node_ = NodeFactory::CreateNodeFromExtended(root_ext_pub_);
  } else {
    bytes_t ext_prv;
    if (Crypto::Decrypt(credentials_.ephemeral_key(),
                        root_ext_prv_enc_,
                        ext_prv)) {
      root_node_ = NodeFactory::CreateNodeFromExtended(ext_prv);
    } else {
      root_node_ = NULL;
    }
  }
  return root_node_;
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

Transaction& Wallet::GetTx(const bytes_t& hash) {
  return tx_hashes_to_txs_[hash];
}

void Wallet::AddTx(const Transaction& transaction) {
  // Stick it in the map.
  tx_hashes_to_txs_[transaction.hash()] = transaction;

  // Check every input to see which output it spends, and if we know
  // about that output, mark it spent.
  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_ins_t::const_iterator j = i->second.inputs().begin();
         j != i->second.inputs().end();
         ++j) {
      if (DoesTxExist(j->prev_txo_hash())) {
        Transaction& affected_tx = GetTx(j->prev_txo_hash());
        affected_tx.MarkOutputSpent(j->prev_txo_index());
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
  Transaction tx(is);
  AddTx(tx);

  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_outs_t::const_iterator j = i->second.outputs().begin();
         j != i->second.outputs().end();
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

bool Wallet::CreateTx(const tx_outs_t& /*recipients*/,
                      uint64_t /*fee*/,
                      uint32_t change_index,
                      bool /*should_sign*/,
                      bytes_t& tx) {
  std::string node_path("m/1/");
  node_path += change_index;
  Node* change_node =
    NodeFactory::DeriveChildNodeWithPath(*GetRootNode(), node_path);
  TxOut change_txo(0, Base58::toHash160(change_node->public_key()));
  delete change_node;

  // TODO: this is https://blockchain.info/tx/0eb2848657a8b0804041f6168f12e69b0297c2fa0fe85f39b8969a294846a6df
  tx = unhexlify("01000000013a369be99568ef339c8f913dde760076dd3d7825272e957559d03cd8e6e55a55000000006b483045022100cddd9d990ef28591f75f02faf58c1627303954db1927503a3010174c910fa470022059540c028a2606613cd645bb40fd0957a3fc6ed930533882ab5079f48995974e012103ad1f0703fe90a3ae314b2a0e92717a2151331d7e8aeb1f5aedc0f242ffd1b122ffffffff02a0860100000000001976a9147dcdbe519137c8ccdf54da3032b16b0005d79b4488aca0860100000000001976a9147f33b7b268769df922c817dbd8d1cca48249c66288ac00000000");

  return true;
}

tx_outs_t Wallet::GetUnspentTxos() {
  tx_outs_t unspent_txos;

  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_outs_t::const_iterator j = i->second.outputs().begin();
         j != i->second.outputs().end();
         ++j) {
      if (!j->is_spent()) {
        unspent_txos.push_back(TxOut(j->value(), j->script(),
                                     j->tx_output_n(), i->first));
      }
    }
  }
  return unspent_txos;
}
