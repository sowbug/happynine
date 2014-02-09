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

#include "encrypting_node_factory.h"

#include <memory>

#include "base58.h"
#include "credentials.h"
#include "crypto.h"
#include "node.h"
#include "node_factory.h"

bool EncryptingNodeFactory::DeriveRootNode(Credentials* credentials,
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

bool EncryptingNodeFactory::GenerateRootNode(Credentials* credentials,
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

bool EncryptingNodeFactory::ImportRootNode(Credentials* credentials,
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

bool EncryptingNodeFactory::DeriveChildNode(Credentials* credentials,
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

bool EncryptingNodeFactory::DeriveChildNode(const Node* master_node,
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

Node* EncryptingNodeFactory::RestoreNode(Credentials* credentials,
                                         const bytes_t& ext_prv_enc) {
  bytes_t ext_prv;
  if (!Crypto::Decrypt(credentials->ephemeral_key(),
                       ext_prv_enc,
                       ext_prv)) {
    return NULL;
  }
  return NodeFactory::CreateNodeFromExtended(ext_prv);
}

Node* EncryptingNodeFactory::RestoreNode(const std::string& ext_pub_b58) {
  const bytes_t ext_pub = Base58::fromBase58Check(ext_pub_b58);
  return NodeFactory::CreateNodeFromExtended(ext_pub);
}
