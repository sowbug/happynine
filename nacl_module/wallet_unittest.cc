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

#include "gtest/gtest.h"
#include "test_constants.h"

#include "base58.h"
#include "credentials.h"
#include "node.h"
#include "wallet.h"

const std::string PP1 = "secret";

TEST(WalletTest, HappyPath) {
  std::auto_ptr<Credentials> c(new Credentials);

  // Get credentials set up and unlocked.
  {
    bytes_t salt;
    bytes_t check;
    bytes_t encrypted_ephemeral_key;
    EXPECT_TRUE(c->SetPassphrase(PP1, salt, check, encrypted_ephemeral_key));
    EXPECT_FALSE(c->isLocked());
    EXPECT_TRUE(c->ephemeral_key().size() > 0);
  }

  // Generate a new node.
  bytes_t ext_prv_enc;
  EXPECT_TRUE(Wallet::GenerateRootNode(c.get(), ext_prv_enc));
  std::auto_ptr<Node> master_node(Wallet::RestoreNode(c.get(), ext_prv_enc));
  uint32_t fp = master_node->fingerprint();
  EXPECT_NE(0, fp);  // This will fail one out of every 2^32 times.

  // Does importing a bad xprv fail?
  EXPECT_FALSE(Wallet::ImportRootNode(c.get(), EXT_3442193E_PRV_B58 + "z",
                                      ext_prv_enc));

  // Can we import an xprv?
  EXPECT_TRUE(Wallet::ImportRootNode(c.get(), EXT_3442193E_PRV_B58,
                                     ext_prv_enc));
  master_node.reset(Wallet::RestoreNode(c.get(), ext_prv_enc));
  EXPECT_EQ(0x3442193e, master_node->fingerprint());

  EXPECT_TRUE(Wallet::DeriveChildNode(c.get(), master_node.get(), "m/0'",
                                      ext_prv_enc));
  std::auto_ptr<Node> node(Wallet::RestoreNode(c.get(), ext_prv_enc));
  EXPECT_EQ(0x5c1bd648, node->fingerprint());
}
