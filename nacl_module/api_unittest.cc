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
#include "gtest/gtest.h"
#include "jsoncpp/json/reader.h"
#include "jsoncpp/json/writer.h"
#include "types.h"

TEST(ApiTest, HappyPath) {
  Credentials c;
  Wallet w(c);
  API api(c, w);
  Json::Value request;
  Json::Value response;

  uint64_t expected_balance = 0;

  request["new_passphrase"] = "foo";
  EXPECT_TRUE(api.HandleSetPassphrase(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));

  // Import an xprv (see test/h9-test-vectors-script.txt)
  request = Json::Value();
  response = Json::Value();
  request["seed_hex"] = "baddecaf99887766554433221100";
  EXPECT_TRUE(api.HandleDeriveRootNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));

  const std::string ext_pub_b58(response["ext_pub_b58"].asString());
  const std::string ext_prv_enc(response["ext_prv_enc"].asString());
  EXPECT_EQ("0x8bb9cbc0", response["fp"].asString());

  // Generate a root node and make sure the response changed
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api.HandleGenerateRootNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_NE(ext_pub_b58, response["ext_pub_b58"].asString());

  // Set back to the imported root node
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = ext_pub_b58;
  request["ext_prv_enc"] = ext_prv_enc;
  EXPECT_TRUE(api.HandleAddRootNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x8bb9cbc0", response["fp"].asString());

  // Derive a child
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/0'";
  request["is_watch_only"] = false;
  //  request["public_addr_n"] = 2;
  //  request["change_addr_n"] = 2;
  EXPECT_TRUE(api.HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x5adb92c0", response["fp"].asString());
  EXPECT_EQ(8 + 8, response["address_statuses"].size());

  // TODO(miket): change to address_t, including value & is_public
  EXPECT_EQ("1KK55Nf8ZZ88jQzG5pwfEzwukyDvgFxKRy",  // m/0'/0/0
            response["address_statuses"][0]["addr_b58"].asString());
  EXPECT_EQ("1CbammCCGPPU4LX64xe33QcdjsYBWv4gHG",  // m/0'/1/0
            response["address_statuses"][8 + 0]["addr_b58"].asString());

  // Pretend we sent blockchain.address.get_history for each address
  // and got back some stuff.
  request = Json::Value();
  response = Json::Value();
  const std::string HASH_TX =
    "555ae5e6d83cd05975952e2725783ddd760076de3d918f9c33ef6895e99b363a";
  request["tx_statuses"][0]["hash"] = HASH_TX;
  request["tx_statuses"][0]["height"] = 282172;
  EXPECT_TRUE(api.HandleReportTxStatuses(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ(1, response["tx_requests"].size());
  EXPECT_EQ(HASH_TX, response["tx_requests"][0].asString());

  // Pretend we did a blockchain.transaction.get for the requested
  // transaction. We should get back an update to an address balance.
  request = Json::Value();
  response = Json::Value();

  // https://blockchain.info/tx/555ae5e6d83cd05975952e2725783ddd760076de3d918f9c33ef6895e99b363a
  // 1KK55Nf8ZZ88jQzG5pwfEzwukyDvgFxKRy - 0.002 BTC
  // 1CUBwHRHD4D4ckRBu81n8cboGVUP9Ve7m4 - 0.01676469 BTC
  expected_balance += 200000;
  request["txs"][0]["tx"] =
    "01000000019970765cdbceee5b6ab67491218f74a130aa6c81932d088c9b44ece1be7fbe1"
    "b010000006b483045022100cce48367450cc2a76e4033dd342b7792e7c36011bff6e71eef"
    "314a498045f09e02205a7fdbcb0d7428f8b3ca0818902727e9babb28f8a0582f5608f3a49"
    "c842d2e51012102ecbf6d557ccbf87295769deace203ee31fd3bb57813b38d1322881c38f"
    "30674dffffffff02400d0300000000001976a914c8dd2744f160f0f24537606b82e40d5d0"
    "815810388acb5941900000000001976a9147dcdbe519137c8ccdf54da3032b16b0005d79b"
    "4488ac00000000";
  EXPECT_TRUE(api.HandleReportTxs(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ(1, response["address_statuses"].size());
  EXPECT_EQ(expected_balance,
            response["address_statuses"][0]["value"].asUInt64());

  // Spend some of the funds in the wallet.
  uint64_t amount_to_spend = 99777;
  uint64_t fee = 42;

  request = Json::Value();
  response = Json::Value();
  request["recipients"][0]["addr_b58"] = "1CUBwHRHD4D4ckRBu81n8cboGVUP9Ve7m4";
  request["recipients"][0]["value"] = (Json::Value::UInt64)amount_to_spend;
  request["fee"] = (Json::Value::UInt64)fee;
  request["change_index"] = 0;
  request["sign"] = true;

  EXPECT_TRUE(api.HandleCreateTx(request, response));

  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_TRUE(response["tx"].asString().size() > 0);
  const bytes_t tx(unhexlify(response["tx"].asString()));
  //std::cerr << response["tx"].asString() << std::endl;

  // Broadcast, then report that we got the transaction. Expect new balance.
  expected_balance -= amount_to_spend;
  expected_balance -= fee;
  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] = to_hex(tx);
  EXPECT_TRUE(api.HandleReportTxs(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ(1, response["address_statuses"].size());
  EXPECT_EQ(expected_balance,
            response["address_statuses"][0]["value"].asUInt64());
  EXPECT_EQ("1CbammCCGPPU4LX64xe33QcdjsYBWv4gHG",
            response["address_statuses"][0]["addr_b58"].asString());
}

TEST(ApiTest, BadInput) {
  Credentials c;
  Wallet w(c);
  API api(c, w);
  Json::Value request;
  Json::Value response;

  request["new_passphrase"] = "foo";
  EXPECT_TRUE(api.HandleSetPassphrase(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));

  // Import a bad xprv; should fail
  request = Json::Value();
  response = Json::Value();
  request["ext_prv_b58"] = "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stb"
    "Py6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHz";
  EXPECT_TRUE(api.HandleImportRootNode(request, response));
  EXPECT_FALSE(api.DidResponseSucceed(response));
}
