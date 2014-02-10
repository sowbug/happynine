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
#include "encrypting_node_factory.h"
#include "node.h"
#include "wallet.h"

class TestWallet: public Wallet {
public:
  TestWallet(Blockchain* blockchain,
             Credentials* credentials,
             const std::string& ext_pub_b58,
             const bytes_t& ext_prv_enc)
    : Wallet(blockchain, credentials, ext_pub_b58, ext_prv_enc) {
  }

  void FakeUpdateAddressBalance(const bytes_t& hash160,
                                uint64_t balance) {
    UpdateAddressBalance(hash160, balance);
  }

  void FakeUpdateAddressTxCount(const bytes_t& hash160,
                                uint64_t tx_count) {
    UpdateAddressTxCount(hash160, tx_count);
  }
};

TEST(WalletTest, HappyPath) {
  std::auto_ptr<Blockchain> b(new Blockchain);

  // Load credentials
  std::auto_ptr<Credentials> c(new Credentials);
  c->Load(unhexlify(CREDENTIALS_SALT),
          unhexlify(CREDENTIALS_CHECK),
          unhexlify(CREDENTIALS_EKEY_ENC));

  // Set up wallet with just child
  std::auto_ptr<TestWallet>
    w(new TestWallet(b.get(),
                     c.get(),
                     EXT_3442193E_PUB_B58,
                     unhexlify(EXT_3442193E_PRV_ENC)));

  // Does the wallet have the expect number of addresses?
  EXPECT_EQ(4, w->public_address_count());
  EXPECT_EQ(4, w->change_address_count());

  // ... and the expected addresses?
  const Blockchain::address_t
    ADDR(Base58::fromAddress("12CL4K2eVqj7hQTix7dM7CVHCkpP17Pry3"));
  Address::addresses_t addresses;
  w->GetAddresses(addresses);
  EXPECT_EQ(ADDR, addresses[0]->hash160());

  // Send to that address. Does the number of addresses in the wallet
  // increase?
  w->FakeUpdateAddressTxCount(ADDR, 12);
  EXPECT_EQ(4 + 4, w->public_address_count());
  EXPECT_EQ(4, w->change_address_count());

  // ... and to one of the change addresses. Number should increase as well.
  const Blockchain::address_t
    CHANGE_ADDR(Base58::fromAddress("1GtD6J3DK1SrZu7bqMt1VKGNjhFap7t5Ku"));
  w->FakeUpdateAddressTxCount(CHANGE_ADDR, 12);
  EXPECT_EQ(4 + 4, w->public_address_count());
  EXPECT_EQ(4 + 4, w->change_address_count());

  // More twiddling addresses in the initial block, which shouldn't
  // cause additional new blocks to be allocated.
  std::vector<Blockchain::address_t> ADDRS;
  ADDRS.push_back(Base58::fromAddress("12CL4K2eVqj7hQTix7dM7CVHCkpP17Pry3"));
  ADDRS.push_back(Base58::fromAddress("13Q3u97PKtyERBpXg31MLoJbQsECgJiMMw"));
  ADDRS.push_back(Base58::fromAddress("18FcseQ86zCaXzLbgDsH86292xb2EuKtFW"));
  ADDRS.push_back(Base58::fromAddress("1EBPs7ApVkRNy9Y8Z8xLAueeH4wuD1Aixb"));
  ADDRS.push_back(Base58::fromAddress("1GtD6J3DK1SrZu7bqMt1VKGNjhFap7t5Ku"));
  ADDRS.push_back(Base58::fromAddress("1J4LVanjHMu3JkXbVrahNuQCTGCRRgfWWx"));
  ADDRS.push_back(Base58::fromAddress("1NZ97rKhSPy6NLud5Dp89E4yH5a2fUGeyC"));
  ADDRS.push_back(Base58::fromAddress("1NwEtFZ6Td7cpKaJtYoeryS6avP2TUkSMh"));
  for (std::vector<Blockchain::address_t>::const_iterator i = ADDRS.begin();
       i != ADDRS.end();
       ++i) {
    w->FakeUpdateAddressBalance(*i, 12);
    w->FakeUpdateAddressTxCount(*i, 12);
  }
  EXPECT_EQ(4 + 4, w->public_address_count());
  EXPECT_EQ(4 + 4, w->change_address_count());
}

TEST(WalletTest, NodeCreation) {
  const std::string PP1 = "secret";

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
  EXPECT_TRUE(EncryptingNodeFactory::GenerateMasterNode(c.get(), ext_prv_enc));
  std::auto_ptr<Node>
    master_node(EncryptingNodeFactory::RestoreNode(c.get(), ext_prv_enc));
  uint32_t fp = master_node->fingerprint();
  EXPECT_NE(0, fp);  // This will fail one out of every 2^32 times.

  // Does importing a bad xprv fail?
  EXPECT_FALSE(EncryptingNodeFactory::ImportMasterNode(c.get(),
                                                       EXT_3442193E_PRV_B58
                                                       + "z",
                                                       ext_prv_enc));

  // Can we import an xprv?
  EXPECT_TRUE(EncryptingNodeFactory::ImportMasterNode(c.get(),
                                                      EXT_3442193E_PRV_B58,
                                                      ext_prv_enc));
  master_node.reset(EncryptingNodeFactory::RestoreNode(c.get(), ext_prv_enc));
  EXPECT_EQ(0x3442193e, master_node->fingerprint());

  EXPECT_TRUE(EncryptingNodeFactory::DeriveChildNode(c.get(),
                                                     master_node.get(),
                                                     "m/0'",
                                                     ext_prv_enc));
  std::auto_ptr<Node> node(EncryptingNodeFactory::RestoreNode(c.get(),
                                                              ext_prv_enc));
  EXPECT_EQ(0x5c1bd648, node->fingerprint());
}
