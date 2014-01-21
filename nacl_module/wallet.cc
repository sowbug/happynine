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

#include "base58.h"
#include "credentials.h"
#include "crypto.h"
#include "node.h"
#include "node_factory.h"
#include "wallet.h"

Wallet::Wallet()
  : credentials_(NULL), root_node_(NULL) {
}

Wallet::~Wallet() {
  delete root_node_;
}

Wallet& Wallet::GetSingleton() {
  static Wallet singleton;
  return singleton;
}

void Wallet::set_root_ext_keys(const bytes_t& ext_pub,
                               const bytes_t& ext_prv_enc) {
  root_ext_pub_ = ext_pub;
  root_ext_prv_enc_ = ext_prv_enc;
}

bool Wallet::GenerateRootNode(const bytes_t& extra_seed_bytes,
                              bytes_t& ext_prv_enc,
                              bytes_t& seed_bytes) {
  if (credentials_->isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }
  seed_bytes = bytes_t(32, 0);
  if (!Crypto::GetRandomBytes(seed_bytes)) {
    return false;
  }
  seed_bytes.insert(seed_bytes.end(),
                    extra_seed_bytes.begin(),
                    extra_seed_bytes.end());

  Node* node = NodeFactory::CreateNodeFromSeed(seed_bytes);
  if (node) {
    if (Crypto::Encrypt(credentials_->ephemeral_key(),
                        node->toSerialized(),
                        ext_prv_enc)) {
      set_root_ext_keys(node->toSerializedPublic(), ext_prv_enc);
      return true;
    }
    delete node;
  }
  return false;
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
  if (credentials_->isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }

  const bytes_t ext_prv = Base58::fromBase58Check(ext_prv_b58);
  Node* node = NodeFactory::CreateNodeFromExtended(ext_prv);
  if (node) {
    if (Crypto::Encrypt(credentials_->ephemeral_key(),
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
  if (!isWatchOnly && credentials_->isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }
  if (!hasRootNode()) {
    std::cerr << "no root node" << std::endl;
    return false;
  }

  *node = NodeFactory::DeriveChildNodeWithPath(*root_node_, path);
  if (*node) {
    if (isWatchOnly) {
      return true;
    } else {
      if (Crypto::Encrypt(credentials_->ephemeral_key(),
                          (*node)->toSerialized(),
                          ext_prv_enc)) {
        return true;
      }
    }
  }
  return false;
}

Node* Wallet::GetRootNode() {
  if (!hasRootNode()) {
    return NULL;
  }

  delete root_node_;
  if (credentials_->isLocked()) {
    root_node_ = NodeFactory::CreateNodeFromExtended(root_ext_pub_);
  } else {
    bytes_t ext_prv;
    if (Crypto::Decrypt(credentials_->ephemeral_key(),
                        root_ext_prv_enc_,
                        ext_prv)) {
      root_node_ = NodeFactory::CreateNodeFromExtended(ext_prv);
    } else {
      root_node_ = NULL;
    }
  }
  return root_node_;
}
