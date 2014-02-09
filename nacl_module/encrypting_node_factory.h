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

class Credentials;
class Node;

class EncryptingNodeFactory {
 public:
  // Root nodes
  static bool DeriveRootNode(Credentials* credentials,
                             const bytes_t& seed, bytes_t& ext_prv_enc);
  static bool GenerateRootNode(Credentials* credentials,
                               bytes_t& ext_prv_enc);
  static bool ImportRootNode(Credentials* credentials,
                             const std::string& ext_prv_b58,
                             bytes_t& ext_prv_enc);

  // Child nodes
  static bool DeriveChildNode(Credentials* credentials,
                              const Node* master_node,
                              const std::string& path,
                              bytes_t& ext_prv_enc);
  static bool DeriveChildNode(const Node* master_node,
                              const std::string& path,
                              std::string& ext_pub_b58);

  // All nodes
  static Node* RestoreNode(Credentials* credentials,
                           const bytes_t& ext_prv_enc);
  static Node* RestoreNode(const std::string& ext_pub_b58);
};
