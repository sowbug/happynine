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

#include "types.h"

class Node;

class NodeFactory {
 public:
  // From a seed. Typically for master key generation.
  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#master-key-generation
  static Node* CreateNodeFromSeed(const bytes_t& seed);

  // From a decoded-from-Base58 xpub/xprv string. Used for both master
  // and children.
  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#serialization-format
  static Node* CreateNodeFromExtended(const bytes_t& bytes);

  // Given a parent node and using a path string like "m/0'/1/2",
  // returns the appropriate child node (which, in the case of m, is a copy
  // of the parent). Caller obtains ownership of the result.
  static Node* DeriveChildNodeWithPath(const Node& parent_node,
                                       const std::string& path);

  // TODO
  static Node* DeriveChildNode(const Node& parent_node,
                               uint32_t i);
};
