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

Wallet::Wallet(Credentials& credentials)
  : credentials_(credentials), root_node_(NULL), child_node_(NULL),
    public_address_gap_(4), change_address_gap_(4),
    public_address_start_(0), change_address_start_(0),
    next_change_address_index_(change_address_start_) {
  ResetGaps();
}

void Wallet::ResetGaps() {
  public_address_count_ = 0;
  change_address_count_ = 0;
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

void Wallet::set_root_ext_keys(const bytes_t& ext_pub,
                               const bytes_t& ext_prv_enc) {
  root_ext_pub_ = ext_pub;
  root_ext_prv_enc_ = ext_prv_enc;
}

void Wallet::set_child_ext_keys(const bytes_t& ext_pub,
                                const bytes_t& ext_prv_enc) {
  child_ext_pub_ = ext_pub;
  child_ext_prv_enc_ = ext_prv_enc;
  watched_addresses_.clear();
  ResetGaps();
  next_change_address_index_ = change_address_start_;
  addresses_to_report_.clear();
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
                             bool is_watch_only,
                             bytes_t& ext_prv_enc) {
  if (!is_watch_only && IsWalletLocked()) {
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
    if (!is_watch_only) {
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

void Wallet::WatchAddress(const bytes_t& hash160,
                          uint32_t child_num,
                          bool is_public) {
  if (IsAddressWatched(hash160)) {
    return;
  }
  Address* address = new Address(hash160, child_num, is_public);
  watched_addresses_[hash160] = address;
  addresses_to_report_.insert(hash160);
}

bool Wallet::IsAddressWatched(const bytes_t& hash160) {
  return watched_addresses_.count(hash160) == 1;
}

void Wallet::UpdateAddressBalance(const bytes_t& hash160, uint64_t balance) {
  if (!IsAddressWatched(hash160)) {
    std::cerr << "oops, update address of unwatched address" << std::endl;
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
    addresses_to_report_.insert(hash160);
  }
}

void Wallet::RestoreRootNode(const Node* /*node*/) {
  // Nothing to do!
}

void Wallet::GenerateAddressBunch(uint32_t start, uint32_t count,
                                  bool is_public) {
  const std::string& path_prefix = is_public ?
    "m/0/" : // external path
    "m/1/";  // internal path
  Node* node = GetChildNode();
  for (uint32_t i = start; i < start + count; ++i) {
    std::stringstream node_path;
    node_path << path_prefix << i;
    std::auto_ptr<Node>
      address_node(NodeFactory::DeriveChildNodeWithPath(*node,
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

void Wallet::RestoreChildNode(const Node* /*node*/) {
  CheckPublicAddressGap(0);
  CheckChangeAddressGap(0);
}

bool Wallet::RestoreNode(const std::string& ext_pub_b58,
                         const bytes_t& ext_prv_enc,
                         bool& is_root) {
  const bytes_t ext_pub = Base58::fromBase58Check(ext_pub_b58);
  std::auto_ptr<Node> node;
  node.reset(NodeFactory::CreateNodeFromExtended(ext_pub));
  if (node.get()) {
    if (node->child_num() == 0) {
      if (ext_prv_enc.empty()) {
        return false;
      }
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

  if (credentials_.isLocked() || child_ext_prv_enc_.empty()) {
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
      if (IsAddressWatched(address)) {
        if (!j->is_spent()) {
          unspent_txos.push_back(TxOut(j->value(), j->script(),
                                       j->tx_output_n(), i->first));
        }
      }
    }
  }
  return unspent_txos;
}
