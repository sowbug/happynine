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

Wallet::Wallet() : credentials_(NULL) {
}

Wallet& Wallet::GetSingleton() {
  static Wallet singleton;
  return singleton;
}

bool Wallet::GenerateRootNode(const bytes_t& extra_seed_bytes,
                              Node** node,
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

  *node = NodeFactory::CreateNodeFromSeed(seed_bytes);
  if (*node) {
    if (Crypto::Encrypt(credentials_->ephemeral_key(),
                        (*node)->toSerialized(),
                        ext_prv_enc)) {
      return true;
    }
  }
  return false;
}

bool Wallet::SetRootNode(const bytes_t& ext_prv_enc, Node** node) {
  if (credentials_->isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }

  bytes_t ext_prv;
  if (Crypto::Decrypt(credentials_->ephemeral_key(), ext_prv_enc, ext_prv)) {
    *node = NodeFactory::CreateNodeFromExtended(ext_prv);
    if (*node) {
      return true;
    }
  }
  return false;
}

bool Wallet::ImportRootNode(const std::string& ext_prv_b58,
                            Node** node,
                            bytes_t& ext_prv_enc) {
  if (credentials_->isLocked()) {
    std::cerr << "wallet is locked" << std::endl;
    return false;
  }

  const bytes_t ext_prv = Base58::fromBase58Check(ext_prv_b58);
  *node = NodeFactory::CreateNodeFromExtended(ext_prv);
  if (*node) {
    if (Crypto::Encrypt(credentials_->ephemeral_key(), ext_prv, ext_prv_enc)) {
      return true;
    }
  }
  return false;
}
