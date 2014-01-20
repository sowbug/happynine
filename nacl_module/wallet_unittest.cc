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

#include "credentials.h"
#include "node.h"
#include "wallet.h"

const std::string PP1 = "secret";

TEST(WalletTest, HappyPath) {
  Wallet w;
  Credentials c;

  w.SetCredentials(&c);

  // Get credentials set up and unlocked.
  {
    bytes_t salt;
    bytes_t check;
    bytes_t encrypted_ephemeral_key;
    EXPECT_TRUE(c.SetPassphrase(PP1, salt, check, encrypted_ephemeral_key));
    EXPECT_FALSE(c.isLocked());
    EXPECT_TRUE(c.ephemeral_key().size() > 0);
  }

  // Generate a new node.
  bytes_t extra_seed;
  Node* node = NULL;
  bytes_t ext_prv_enc;
  bytes_t seed_bytes;
  EXPECT_TRUE(w.GenerateRootNode(extra_seed, &node, ext_prv_enc, seed_bytes));
  uint32_t fp = node->fingerprint();
  EXPECT_NE(0, fp);  // This will fail one out of every 2^32 times.
  delete node;

  // Set to the new node and confirm we got it back.
  EXPECT_TRUE(w.SetRootNode(ext_prv_enc, &node));
  EXPECT_EQ(fp, node->fingerprint());
  delete node;

  // Can we import an xprv?
  EXPECT_TRUE(w.ImportRootNode("xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4"
                               "stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNN"
                               "U3TGtRBeJgk33yuGBxrMPHi",
                               &node,
                               ext_prv_enc));
  EXPECT_EQ(0x3442193e, node->fingerprint());

  // Does importing a bad xprv fail?
  EXPECT_FALSE(w.ImportRootNode("xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4"
                                "stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNN"
                                "U3TGtRBeJgk33yuGBxrMPHi"
                                "z",
                                &node,
                                ext_prv_enc));
}
