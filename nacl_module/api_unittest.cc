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

#if defined(BE_LOUDER)
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

static bool
GetHistoryResponseContains(const Json::Value& response,
                           const std::string& expected_addr_b58,
                           int64_t expected_value) {
  const Json::Value& r = response["history"];

#if defined(BE_LOUD)
  std::cerr << "START HIST" << std::endl;
  for (Json::Value::const_iterator i = r.begin(); i != r.end(); ++i) {
    std::cerr << "Hist: "
              << (*i)["tx_hash"].asString()
              << ": " << (*i)["addr_b58"].asString()
              << ": " << (*i)["value"].asInt64() << std::endl;
  }
  std::cerr << "END HIST" << std::endl;
#endif

  for (Json::Value::const_iterator i = r.begin(); i != r.end(); ++i) {
    if ((*i)["addr_b58"].asString() == expected_addr_b58 &&
        (*i)["value"].asInt64() == expected_value) {
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
  EXPECT_TRUE(api->HandleDeriveMasterNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  const std::string ext_pub_b58(response["ext_pub_b58"].asString());
  const std::string ext_prv_enc(response["ext_prv_enc"].asString());
  EXPECT_EQ("0x8bb9cbc0", response["fp"].asString());

  // Generate a master node and make sure the response changed
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGenerateMasterNode(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_NE(ext_pub_b58, response["ext_pub_b58"].asString());

  // Set back to the imported master node
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
  // we got from Electrum.
  request = Json::Value();
  response = Json::Value();
  expected_balance += 29000;
  request["txs"][0]["tx"] = TX_1BCB_HEX;
  EXPECT_TRUE(api->HandleReportTxs(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Now when we ask for a snapshot of addresses, the balance should
  // be correct.
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ((4 * 2) + 4, response["addresses"].size());
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_199T_B58,
                                         expected_balance));

  // We should see the transaction in the history.
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetHistory(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(1, response["history"].size());
  EXPECT_TRUE(GetHistoryResponseContains(response, ADDR_199T_B58, 29000));

  // Spend some of the funds in the wallet.
  uint64_t amount_to_spend = 888;
  uint64_t fee = 42;

  Spend(api.get(), ADDR_1H97_B58, amount_to_spend, fee);
  expected_balance -= amount_to_spend;
  expected_balance -= fee;

  // Balance should reflect the spending.
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ((2 * 4) + (2 * 4), response["addresses"].size());
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_1Guw_B58,
                                         expected_balance));

  // We should see the transaction in the history.
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetHistory(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(2, response["history"].size());
  EXPECT_TRUE(GetHistoryResponseContains(response, ADDR_199T_B58, 29000));
  EXPECT_TRUE(GetHistoryResponseContains(response, ADDR_199T_B58, -930));

  // Now spend the rest of the funds in the wallet.
  fee = 2;
  amount_to_spend = expected_balance - fee;
  Spend(api.get(), ADDR_1H97_B58, amount_to_spend, fee);
  expected_balance -= amount_to_spend;
  expected_balance -= fee;

  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ((2 * 4) + (2 * 4), response["addresses"].size());
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_1Guw_B58,
                                         expected_balance));

  // We should see all our transactions in the history.
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetHistory(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ(3, response["history"].size());
  EXPECT_TRUE(GetHistoryResponseContains(response, ADDR_199T_B58, 29000));
  EXPECT_TRUE(GetHistoryResponseContains(response, ADDR_1Guw_B58, -28070));
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
  request["ext_prv_b58"] = EXT_3442193E_PRV_B58 + "z";
  EXPECT_TRUE(api->HandleImportMasterNode(request, response));
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
  EXPECT_TRUE(api->HandleDeriveMasterNode(request, response));
  EXPECT_FALSE(api->DidResponseSucceed(response));
}

TEST(ApiTest, RestoreWithLockedWallet) {
  std::auto_ptr<Blockchain> b(new Blockchain);
  std::auto_ptr<Credentials> c(new Credentials);
  std::auto_ptr<API> api(new API(b.get(), c.get()));
  Json::Value request;
  Json::Value response;

  // Restore wallet
  request["check"] = CREDENTIALS_CHECK;
  request["ekey_enc"] = CREDENTIALS_EKEY_ENC;
  request["salt"] = CREDENTIALS_SALT;
  EXPECT_TRUE(api->HandleSetCredentials(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

  // Restore master node with just public. This should always fail.
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = EXT_3442193E_PUB_B58;
  EXPECT_TRUE(api->HandleRestoreNode(request, response));
  EXPECT_FALSE(api->DidResponseSucceed(response));

  // Restore master node with just extended private. This should
  // always fail.
  request = Json::Value();
  response = Json::Value();
  request["ext_prv_b58"] = EXT_3442193E_PRV_B58;
  EXPECT_TRUE(api->HandleRestoreNode(request, response));
  EXPECT_FALSE(api->DidResponseSucceed(response));

  // Restore master node with both public/private. This should succeed,
  // though it will be impossible to derive children while the wallet
  // is locked.
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = EXT_3442193E_PUB_B58;
  request["ext_prv_enc"] = EXT_3442193E_PRV_ENC;
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
  // master and child by deriving a different child.
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
  EXPECT_TRUE(api->HandleDeriveMasterNode(request, response));
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
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api->HandleGetAddresses(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));
  EXPECT_EQ((2 * 4) + 4, response["addresses"].size());
  EXPECT_TRUE(GetAddressResponseContains(response, ADDR_199T_B58,
                                         29000));

  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] = TX_100D_HEX;
  EXPECT_TRUE(api->HandleReportTxs(request, response));
  EXPECT_TRUE(api->DidResponseSucceed(response));

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
