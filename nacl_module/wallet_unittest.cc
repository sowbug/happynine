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
#include <string>
#include <set>

#include "gtest/gtest.h"

#include "address.h"
#include "base58.h"
#include "child_node.h"
#include "credentials.h"
#include "node.h"
#include "wallet.h"

const std::string PP1 = "secret";

TEST(WalletTest, HappyPath) {
  Credentials c;
  Wallet w(c);

  // Get credentials set up and unlocked.
  {
    bytes_t salt;
    bytes_t check;
    bytes_t encrypted_ephemeral_key;
    EXPECT_TRUE(c.SetPassphrase(PP1, salt, check, encrypted_ephemeral_key));
    EXPECT_FALSE(c.isLocked());
    EXPECT_TRUE(c.ephemeral_key().size() > 0);
  }

  // Test each way of obtaining a node.

  bytes_t generated_ext_prv_enc;
  EXPECT_TRUE(w.GenerateMasterNode(generated_ext_prv_enc));

  bytes_t imported_ext_prv_enc;
  const std::string GOOD_XPRV =
    "xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUt"
    "rGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm5"
    "5Qn3LqFtT2emdEXVYsCzC2U";
  EXPECT_TRUE(w.ImportMasterNode(GOOD_XPRV, imported_ext_prv_enc));

  bytes_t derived_ext_prv_enc;
  const bytes_t SEED(unhexlify("000102030405060708090a0b0c0d0e0f"));
  EXPECT_TRUE(w.DeriveMasterNode(SEED, derived_ext_prv_enc));

  // Now lock the wallet. We should still be able to add the keys.
  EXPECT_TRUE(c.Lock());

  {
    const uint32_t id = w.AddMasterNode(generated_ext_prv_enc);
    EXPECT_NE(0, id);
    Node* node = w.GetMasterNode(id);
    uint32_t fp = node->fingerprint();
    EXPECT_NE(0, fp);  // This will fail one out of every 2^32 times.
    EXPECT_EQ(0, node->child_num());
  }
  {
    const uint32_t id = w.AddMasterNode(imported_ext_prv_enc);
    EXPECT_NE(0, id);
    Node* node = w.GetMasterNode(id);
    EXPECT_NE(0xbd16bee5, node->fingerprint());
    EXPECT_EQ(0, node->child_num());
  }
  {
    const uint32_t id = w.AddMasterNode(derived_ext_prv_enc);
    EXPECT_NE(0, id);
    Node* node = w.GetMasterNode(id);
    EXPECT_NE(0x3442193e, node->fingerprint());
    EXPECT_EQ(0, node->child_num());
  }

  bytes_t ext_prv_enc;
  EXPECT_FALSE(w.GenerateMasterNode(ext_prv_enc));
  EXPECT_FALSE(w.ImportMasterNode(GOOD_XPRV, ext_prv_enc));
  //  EXPECT_FALSE(w.DeriveChildNode(node, 1, ext_prv_enc));

  // TODO(miket): test that we don't allow adding duplicates
}

TEST(WalletTest, MultipleNodes) {
  Credentials c;
  Wallet w(c);

  // Get credentials set up and unlocked.
  {
    bytes_t salt;
    bytes_t check;
    bytes_t encrypted_ephemeral_key;
    EXPECT_TRUE(c.SetPassphrase(PP1, salt, check, encrypted_ephemeral_key));
    EXPECT_FALSE(c.isLocked());
    EXPECT_TRUE(c.ephemeral_key().size() > 0);
  }

  bytes_t ext_prv_enc;

  const int MASTER_COUNT = 4;
  const int CHILD_COUNT = 8;

  std::set<int> master_node_ids;
  for (int i = 0; i < MASTER_COUNT; ++i) {
    // Generate a new node.
    EXPECT_TRUE(w.GenerateMasterNode(ext_prv_enc));
    const uint32_t id = w.AddMasterNode(ext_prv_enc);
    EXPECT_NE(0, id);
    master_node_ids.insert(id);
  }
  EXPECT_EQ(MASTER_COUNT, master_node_ids.size());

  std::set<int> child_node_ids;
  for (std::set<int>::iterator i = master_node_ids.begin();
       i != master_node_ids.end();
       ++i) {
    const Node* master_node = w.GetMasterNode(*i);
    EXPECT_TRUE(master_node != NULL);
    for (int j = 0; j < CHILD_COUNT; ++j) {
      EXPECT_TRUE(w.DeriveChildNode(master_node, j, ext_prv_enc));
      const uint32_t child_id = w.AddChildNode(ext_prv_enc);
      EXPECT_NE(0, child_id);
      child_node_ids.insert(child_id);
    }
  }
  EXPECT_EQ(CHILD_COUNT * MASTER_COUNT, child_node_ids.size());
}
