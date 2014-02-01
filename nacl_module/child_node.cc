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

#include "child_node.h"

#include <iostream>  // cerr
#include <istream>
#include <sstream>

#include "address_watcher.h"
#include "base58.h"
#include "errors.h"
#include "node.h"
#include "node_factory.h"

ChildNode::ChildNode(Node* node, AddressWatcher* address_watcher,
                     uint32_t public_address_gap, uint32_t change_address_gap)
  : node_(node), address_watcher_(address_watcher),
    public_address_gap_(public_address_gap),
    change_address_gap_(change_address_gap),
    public_address_start_(0), change_address_start_(0),
    next_change_address_index_(change_address_start_) {
  ResetGaps();
  CheckPublicAddressGap(0);
  CheckChangeAddressGap(0);
    }

ChildNode::~ChildNode() {
  delete node_;
}

void ChildNode::GenerateAddressBunch(uint32_t start, uint32_t count,
                                     bool is_public) {
  const std::string& path_prefix = is_public ?
    "m/0/" : // external path
    "m/1/";  // internal path
  for (uint32_t i = start; i < start + count; ++i) {
    std::stringstream node_path;
    node_path << path_prefix << i;
    std::auto_ptr<Node>
      address_node(NodeFactory::DeriveChildNodeWithPath(node_,
                                                        node_path.str()));
    if (address_node.get()) {
      const bytes_t addr = Base58::toHash160(address_node->public_key());
      if (is_public) {
        address_watcher_->WatchPublicAddress(addr, i);
      } else {
        address_watcher_->WatchChangeAddress(addr, i);
      }
    }
  }
}

void ChildNode::NotifyPublicAddressUsed(uint32_t index) {
  CheckPublicAddressGap(index);
}

void ChildNode::NotifyChangeAddressUsed(uint32_t index) {
  if (index >= next_change_address_index_) {
    next_change_address_index_ = index + 1;
    if (next_change_address_index_ < change_address_start_) {
      next_change_address_index_ = change_address_start_;
    }
  }
  CheckChangeAddressGap(index);
}

void ChildNode::CheckPublicAddressGap(uint32_t index) {
  // Given the highest address we've used, should we allocate a new
  // bunch of addresses?
  uint32_t desired_count = index + public_address_gap_ -
    public_address_start_ + 1;
  if (desired_count > public_address_count_) {
    // Yes, it's time to allocate.
    GenerateAddressBunch(public_address_start_ + public_address_count_,
                         public_address_gap_, true);
    public_address_count_ += public_address_gap_;
  }
}

void ChildNode::CheckChangeAddressGap(uint32_t index) {
  // Given the highest address we've used, should we allocate a new
  // bunch of addresses?
  uint32_t desired_count = index + change_address_gap_ -
    change_address_start_ + 1;
  if (desired_count > change_address_count_) {
    // Yes, it's time to allocate.
    GenerateAddressBunch(change_address_start_ + change_address_count_,
                         change_address_gap_, false);
    change_address_count_ += change_address_gap_;
  }
}

void ChildNode::ResetGaps() {
  public_address_count_ = 0;
  change_address_count_ = 0;
}

bytes_t ChildNode::GetNextUnusedChangeAddress() {
  // TODO(miket): we should probably increment
  // next_change_address_index_ here, because a transaction might take
  // a while to confirm, and if the user issues several transactions
  // in a row, they'll all go to the same change address.
  std::stringstream node_path;
  node_path << "m/1/" << next_change_address_index_;  // internal path
  std::auto_ptr<Node> address_node(NodeFactory::
                                   DeriveChildNodeWithPath(node_,
                                                           node_path.str()));
  if (address_node.get()) {
    return Base58::toHash160(address_node->public_key());
  }
  return bytes_t();
}

bool ChildNode::GetKeysForAddress(const bytes_t& hash160,
                                  bytes_t& public_key,
                                  bytes_t& key) {
  if (signing_keys_.count(hash160) == 0) {
    return false;
  }
  public_key = signing_public_keys_[hash160];
  key = signing_keys_[hash160];
  return true;
}

void ChildNode::GenerateAllSigningKeys() {
  for (int i = public_address_start_;
       i < public_address_start_ + public_address_count_;
       ++i) {
    std::stringstream node_path;
    node_path << "m/0/" << i;  // external path
    std::auto_ptr<Node> node(NodeFactory::
                             DeriveChildNodeWithPath(node_,
                                                     node_path.str()));
    if (node.get()) {
      bytes_t hash160(Base58::toHash160(node->public_key()));
      signing_public_keys_[hash160] = node->public_key(); 
      signing_keys_[hash160] = node->secret_key();
    }
  }
  for (int i = change_address_start_;
       i < change_address_start_ + change_address_count_;
       ++i) {
    std::stringstream node_path;
    node_path << "m/1/" << i;  // internal path
    std::auto_ptr<Node> node(NodeFactory::
                             DeriveChildNodeWithPath(node_,
                                                     node_path.str()));
    if (node.get()) {
      bytes_t hash160(Base58::toHash160(node->public_key()));
      signing_public_keys_[hash160] = node->public_key();
      signing_keys_[hash160] = node->secret_key();
    }
  }
}

bool ChildNode::CreateTx(const tx_outs_t& recipients,
                         const tx_outs_t& unspent_txos,
                         uint64_t fee,
                         bool should_sign,
                         bytes_t& tx) {
  if (should_sign && !node_->is_private()) {
    return false;
  }
  TxOut change_txo(0, GetNextUnusedChangeAddress());

  GenerateAllSigningKeys();

  Transaction transaction;
  int error_code = 0;
  // TODO should_sign
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

bool ChildNode::IsAddressWatched(const bytes_t& hash160) const {
  return watched_addresses_.count(hash160) == 1;
}

