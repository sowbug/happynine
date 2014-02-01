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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "address.h"
#include "address_watcher.h"
#include "base58.h"
#include "child_node.h"
#include "node.h"
#include "node_factory.h"
#include "types.h"

class FakeAddressWatcher : public AddressWatcher {
public:
  virtual void WatchPublicAddress(const bytes_t& hash160,
                                  uint32_t child_num) {
    public_addresses.push_back(Address(hash160, child_num, true));
  }

  virtual void WatchChangeAddress(const bytes_t& hash160,
                                  uint32_t child_num) {
    change_addresses.push_back(Address(hash160, child_num, false));
  }

  std::vector<Address> public_addresses;
  std::vector<Address> change_addresses;
};

TEST(ChildNodeTest, HappyPath) {
  const uint32_t PUBLIC_GAP = 4;
  const uint32_t CHANGE_GAP = 7;
  const std::string XPRV_TV1_0P("xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7");
  const bytes_t xprv(Base58::fromBase58Check(XPRV_TV1_0P));
  std::auto_ptr<Node> node(NodeFactory::CreateNodeFromExtended(xprv));

  FakeAddressWatcher fake_address_watcher;
  ChildNode child_node(node.release(), &fake_address_watcher,
                       PUBLIC_GAP, CHANGE_GAP);

  // On first invocation, there should be a set of each kind of address
  // watched.
  EXPECT_EQ(PUBLIC_GAP, child_node.public_address_count());
  EXPECT_EQ(CHANGE_GAP, child_node.change_address_count());
  EXPECT_EQ(PUBLIC_GAP, fake_address_watcher.public_addresses.size());
  EXPECT_EQ(CHANGE_GAP, fake_address_watcher.change_addresses.size());

  // From TV1: m/0'/1/0
  EXPECT_EQ("1J5rebbkQaunJTUoNVREDbeB49DqMNFFXk",
            Base58::hash160toAddress(child_node.GetNextUnusedChangeAddress()));
}
