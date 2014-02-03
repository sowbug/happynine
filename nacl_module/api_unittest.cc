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

#include "api.h"
#include "blockchain.h"
#include "credentials.h"
#include "gtest/gtest.h"
#include "jsoncpp/json/reader.h"
#include "jsoncpp/json/writer.h"
#include "test_constants.h"
#include "types.h"
#include "wallet.h"

static void Spend(API* api,
                  const std::string& recipient,
                  uint64_t value,
                  uint64_t fee) {
  Json::Value request;
  Json::Value response;

  // Generate the signed transaction.
  request = Json::Value();
  response = Json::Value();
  request["recipients"][0]["addr_b58"] = recipient;
  request["recipients"][0]["value"] = (Json::Value::UInt64)value;
  request["fee"] = (Json::Value::UInt64)fee;
  request["sign"] = true;

  EXPECT_TRUE(api->HandleCreateTx(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_TRUE(response["tx"].asString().size() > 0);
  const bytes_t tx(unhexlify(response["tx"].asString()));

  // Broadcast the transaction (no code, pretend).
#if defined(BE_LOUD)
  std::cerr << response["tx"].asString() << std::endl;
#endif

  // Report that we got the transaction. Expect new balance.
  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] = to_hex(tx);
  EXPECT_TRUE(api->HandleReportTxs(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
}

static bool GetAddressResponseContains(const Json::Value& response,
                                       const std::string& expected_addr_b58,
                                       uint64_t expected_value) {
  const Json::Value& r = response["addresses"];

#if defined(BE_LOUD)
  for (Json::Value::const_iterator i = r.begin(); i != r.end(); ++i) {
    std::cerr << "Rec'd: " << (*i)["addr_b58"].asString()
              << ": " << (*i)["value"].asUInt64() << std::endl;
  }
#endif

  for (Json::Value::const_iterator i = r.begin(); i != r.end(); ++i) {
    if ((*i)["addr_b58"].asString() == expected_addr_b58 &&
        (*i)["value"].asUInt64() == expected_value) {
      return true;
    }
  }
  return false;
}

TEST(ApiTest, HappyPath) {
  std::auto_ptr<Blockchain> b(new Blockchain);
  std::auto_ptr<Credentials> c(new Credentials);
  std::auto_ptr<API> api(new API(b.get(), c.get()));
  Json::Value request;
  Json::Value response;

  uint64_t expected_balance = 0;

  request["new_passphrase"] = "foo";
  EXPECT_TRUE(api->HandleSetPassphrase(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Import an xprv (see test/h9-test-vectors-script.txt)
  request = Json::Value();
  response = Json::Value();
  request["seed_hex"] = "baddecaf99887766554433221100";
  EXPECT_TRUE(api->HandleDeriveRootNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  const std::string ext_pub_b58(response["ext_pub_b58"].asString());
  const std::string ext_prv_enc(response["ext_prv_enc"].asString());
  EXPECT_EQ("0x8bb9cbc0", response["fp"].asString());

  // Generate a root node and make sure the response changed
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGenerateRootNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_NE(ext_pub_b58, response["ext_pub_b58"].asString());

  // Set back to the imported root node
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = ext_pub_b58;
  request["ext_prv_enc"] = ext_prv_enc;
  EXPECT_TRUE(api->HandleRestoreNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ("0x8bb9cbc0", response["fp"].asString());

  // Derive a child
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/0'";
  request["is_watch_only"] = false;
  EXPECT_TRUE(api->HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Save its serializable stuff
  const std::string child_ext_pub_b58(response["ext_pub_b58"].asString());
  const std::string child_ext_prv_enc(response["ext_prv_enc"].asString());
  const std::string child_fp(response["fp"].asString());

  // Switch to some other child to confirm the previous is gone
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/9999'";
  request["is_watch_only"] = true;
  EXPECT_TRUE(api->HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ("0x03c06f37", response["fp"].asString());

  // Re-add the earlier child and make sure we see the same behavior
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = child_ext_pub_b58;
  request["ext_prv_enc"] = child_ext_prv_enc;
  EXPECT_TRUE(api->HandleRestoreNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(child_fp, response["fp"].asString());

  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(4 + 4, response["addresses"].size());

  // m/0'/0/1
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_199T_B58, 0));
  // m/0'/1/1
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_1Guw_B58, 0));

  // Pretend we sent blockchain.address.get_history for each address
  // and got back some stuff.
  request = Json::Value();
  response = Json::Value();
  request["tx_statuses"][0]["tx_hash"] = TX_1BCB_HASH_HEX;
  request["tx_statuses"][0]["height"] = 282172;
  EXPECT_TRUE(api->HandleReportTxStatuses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Pretend we did a blockchain.transaction.get for the transaction
  // we got from Electrum. We should get back an update to an address
  // balance.
  request = Json::Value();
  response = Json::Value();

  expected_balance += 29000;
  request["txs"][0]["tx"] = TX_1BCB_HEX;
  EXPECT_TRUE(api->HandleReportTxs(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(4 + 4, response["addresses"].size());
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_199T_B58,
                                         expected_balance));

  // Spend some of the funds in the wallet.
  uint64_t amount_to_spend = 888;
  uint64_t fee = 42;

  Spend(api.get(), ADDR_1H97_B58, amount_to_spend, fee);
  expected_balance -= amount_to_spend;
  expected_balance -= fee;

  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(4 + 4, response["addresses"].size());
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_1Guw_B58,
                                         expected_balance));

  // Now spend the rest of the funds in the wallet. This is different
  // because it requires the wallet to report a zero balance without
  // reporting *all* zero balances.
  fee = 2;
  amount_to_spend = expected_balance - fee;
  Spend(api.get(), ADDR_1H97_B58, amount_to_spend, fee);
  expected_balance -= amount_to_spend;
  expected_balance -= fee;

  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(4 + 4, response["addresses"].size());
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_1Guw_B58,
                                         expected_balance));
}

TEST(ApiTest, BadExtPrvB58) {
  std::auto_ptr<Blockchain> b(new Blockchain);
  std::auto_ptr<Credentials> c(new Credentials);
  std::auto_ptr<API> api(new API(b.get(), c.get()));
  Json::Value request;
  Json::Value response;

  request["new_passphrase"] = "foo";
  EXPECT_TRUE(api->HandleSetPassphrase(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Import a bad xprv; should fail
  request = Json::Value();
  response = Json::Value();
  request["ext_prv_b58"] = "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stb"
    "Py6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHz";
  EXPECT_TRUE(api->HandleImportRootNode(request, response));
  EXPECT_FALSE(api->DidResponseSucceed(response));
}

TEST(ApiTest, BadSeedWhenDeriving) {
  std::auto_ptr<Blockchain> b(new Blockchain);
  std::auto_ptr<Credentials> c(new Credentials);
  std::auto_ptr<API> api(new API(b.get(), c.get()));
  Json::Value request;
  Json::Value response;

  request["new_passphrase"] = "foo";
  EXPECT_TRUE(api->HandleSetPassphrase(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Derive with no arguments
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleDeriveRootNode(request, response));
  EXPECT_FALSE(api->DidResponseSucceed(response));
}

TEST(ApiTest, RestoreWithLockedWallet) {
  std::auto_ptr<Blockchain> b(new Blockchain);
  std::auto_ptr<Credentials> c(new Credentials);
  std::auto_ptr<API> api(new API(b.get(), c.get()));
  Json::Value request;
  Json::Value response;

  // Restore wallet
  request["check"] = "c1c89ecb018a907c59926f0ff258dfe05984ee9d9778c9d64569d4b14143f97559ccf015fbc2437aceab2515af84f1ed1a914cdf8e5e42468a789de9876ee3fcd774160ad0318a0b06e47bf2ba92b993";
  request["ekey_enc"] = "dbfcae7ee9756ae82dfef5375733dd29161c0368024ad17f2a60468cf0101904826147276f14cc7e9e4d6f3551d85ff286a04608c732f1188fbad3284a82fec850ac191ba57dc41325ce7229f1826e0c";
  request["salt"] = "4ff991e1b756b4c34a11a435e0ca6a8da738001ad34c7df2327d237bffcb705e";
  EXPECT_TRUE(api->HandleSetCredentials(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Restore root node with just public. This should always fail.
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = EXT_3442193E_PUB_B58;
  EXPECT_FALSE(api->HandleRestoreNode(request, response));
  EXPECT_FALSE(api->DidResponseSucceed(response));

  // Restore root node with just extended private. This should fail
  // with a locked wallet.
  request = Json::Value();
  response = Json::Value();
  request["ext_prv_b58"] = EXT_3442193E_PRV_B58;
  EXPECT_FALSE(api->HandleRestoreNode(request, response));
  EXPECT_FALSE(api->DidResponseSucceed(response));

  // Restore root node with both public/private. This should succeed,
  // though it will be impossible to derive children while the wallet
  // is locked.
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = EXT_3442193E_PUB_B58;
  request["ext_prv_enc"] = "728f52c0144d0d48ae888b01ba9392b84e79c617291263e2c39c9369f7b4f9b0c3d856d6934efaba17b76582570f0719d642532c3cc668b511dff38cd089f098c4a87e3ea8de334c5348322f18844cfc4d81650f7c3b3f2301d81ac06f87b9ef9e99b3f5e9d60ea59dffb16fb8fcd3af";
  EXPECT_TRUE(api->HandleRestoreNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ("0x3442193e", response["fp"].asString());
  EXPECT_EQ("0x00000000", response["pfp"].asString());

  // Restore child node
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw";
  request["ext_prv_enc"] = "4615e7885c69e2ccb4e8c2617de746487e576651e02c4f5dbc780d1d485647a197092738710acb7b50468978c1ded4605f508a7677a82a0690c415529acaa0eea778041aa8405433c0244179e1a271ba745c54e92eaa2b98fa7f222b9e85545f2a2ddbbd215f332c3e7fc4d1e71b778b";
  EXPECT_TRUE(api->HandleRestoreNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ("0x5c1bd648", response["fp"].asString());
  EXPECT_EQ("0x3442193e", response["pfp"].asString());

  // Now unlock wallet so we can derive children.
  request = Json::Value();
  response = Json::Value();
  request["passphrase"] = "foo";
  EXPECT_TRUE(api->HandleUnlock(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Confirm that the wallet correctly distinguished between restored
  // root and child by deriving a different child.
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/0'/1";
  request["is_watch_only"] = true;
  EXPECT_TRUE(api->HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ("0xbef5a2f9", response["fp"].asString());
}

TEST(ApiTest, ReportActualTransactions) {
  std::auto_ptr<Blockchain> b(new Blockchain);
  std::auto_ptr<Credentials> c(new Credentials);
  std::auto_ptr<API> api(new API(b.get(), c.get()));
  Json::Value request;
  Json::Value response;
  Address::addresses_t public_addresses;
  Address::addresses_t change_addresses;

  request["new_passphrase"] = "foo";
  EXPECT_TRUE(api->HandleSetPassphrase(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Import an xprv (see test/h9-test-vectors-script.txt)
  request = Json::Value();
  response = Json::Value();
  request["seed_hex"] = "baddecaf99887766554433221100";
  EXPECT_TRUE(api->HandleDeriveRootNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  const std::string ext_pub_b58(response["ext_pub_b58"].asString());
  const std::string ext_prv_enc(response["ext_prv_enc"].asString());
  const std::string fp(response["fp"].asString());
  EXPECT_EQ("0x8bb9cbc0", fp);

  // Load what we imported
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = ext_pub_b58;
  request["ext_prv_enc"] = ext_prv_enc;
  EXPECT_TRUE(api->HandleRestoreNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(fp, response["fp"].asString());

  // Derive a child
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/0'";
  request["is_watch_only"] = false;
  EXPECT_TRUE(api->HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ("0x5adb92c0", response["fp"].asString());

  const std::string child_ext_pub_b58(response["ext_pub_b58"].asString());
  const std::string child_ext_prv_enc(response["ext_prv_enc"].asString());
  const std::string child_fp(response["fp"].asString());

  // Load the derived child
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = child_ext_pub_b58;
  request["ext_prv_enc"] = child_ext_prv_enc;
  EXPECT_TRUE(api->HandleRestoreNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(child_fp, response["fp"].asString());

  // The wallet should now be watching addresses.
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(4 + 4, response["addresses"].size());

  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] = TX_1BCB_HEX;
  EXPECT_TRUE(api->HandleReportTxs(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // + 4 because using addresses generates another block
  //  EXPECT_EQ(4 + 4, public_addresses.size());
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(4 + 4, response["addresses"].size());
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_199T_B58,
                                         29000));

  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] = TX_100D_HEX;
  EXPECT_TRUE(api->HandleReportTxs(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  // + 4 because using addresses generates another block
  //  EXPECT_EQ(2 + 4, response["address_statuses"].size());

  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_199T_B58, 0));
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_1Guw_B58, 14000));

  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] = TX_BFB1_HEX;
  EXPECT_TRUE(api->HandleReportTxs(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_1Guw_B58, 0));
}
