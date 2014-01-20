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

class Wallet {
 public:
  Wallet();

  static Wallet& GetSingleton();

  void SetCredentials(Credentials* credentials) { credentials_ = credentials; }

  bool GenerateRootNode(const bytes_t& extra_seed_bytes,
                        Node** node,
                        bytes_t& ext_prv_enc,
                        bytes_t& seed_bytes);
  bool SetRootNode(const bytes_t& ext_prv_enc, Node** node);
  bool ImportRootNode(const std::string& ext_prv_b58,
                      Node** node,
                      bytes_t& ext_prv_enc);

 private:
  Credentials* credentials_;
};
