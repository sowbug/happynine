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
  EXPECT_TRUE(api.HandleRestoreNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x8bb9cbc0", response["fp"].asString());

  // Derive a child
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/0'";
  request["is_watch_only"] = false;
  //  request["public_addr_n"] = 2;  TODO: wallet should update these
  //  request["change_addr_n"] = 2;
  EXPECT_TRUE(api.HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x5adb92c0", response["fp"].asString());
  EXPECT_EQ(8 + 8, response["address_statuses"].size());

  // Save its serializable stuff
  const std::string child_ext_pub_b58(response["ext_pub_b58"].asString());
  const std::string child_ext_prv_enc(response["ext_prv_enc"].asString());
  const std::string child_fp(response["fp"].asString());

  // Switch to some other child to confirm the previous is gone
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/9999'";
  request["is_watch_only"] = false;  // TODO(miket): true fails!
  EXPECT_TRUE(api.HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x03c06f37", response["fp"].asString());

  // Re-add the earlier child and make sure we see the same behavior
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = child_ext_pub_b58;
  request["ext_prv_enc"] = child_ext_prv_enc;
  EXPECT_TRUE(api.HandleRestoreNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ(child_fp, response["fp"].asString());
  EXPECT_EQ(8 + 8, response["address_statuses"].size());

  // TODO(miket): change to address_t, including value & is_public
  // TODO(miket): neither of these is valid; see
  // https://github.com/sowbug/happynine/issues/35
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
  request["tx_statuses"][0]["tx_hash"] = HASH_TX;
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

TEST(ApiTest, BadExtPrvB58) {
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

TEST(ApiTest, BadSeedWhenDeriving) {
  Credentials c;
  Wallet w(c);
  API api(c, w);
  Json::Value request;
  Json::Value response;

  request["new_passphrase"] = "foo";
  EXPECT_TRUE(api.HandleSetPassphrase(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));

  // Derive with no arguments
  request = Json::Value();
  response = Json::Value();
  EXPECT_TRUE(api.HandleDeriveRootNode(request, response));
  EXPECT_FALSE(api.DidResponseSucceed(response));
}

TEST(ApiTest, RestoreWithLockedWallet) {
  Credentials c;
  Wallet w(c);
  API api(c, w);
  Json::Value request;
  Json::Value response;

  // Restore wallet
  request["check"] = "c1c89ecb018a907c59926f0ff258dfe05984ee9d9778c9d64569d4b14143f97559ccf015fbc2437aceab2515af84f1ed1a914cdf8e5e42468a789de9876ee3fcd774160ad0318a0b06e47bf2ba92b993";
  request["ekey_enc"] = "dbfcae7ee9756ae82dfef5375733dd29161c0368024ad17f2a60468cf0101904826147276f14cc7e9e4d6f3551d85ff286a04608c732f1188fbad3284a82fec850ac191ba57dc41325ce7229f1826e0c";
  request["salt"] = "4ff991e1b756b4c34a11a435e0ca6a8da738001ad34c7df2327d237bffcb705e";
  EXPECT_TRUE(api.HandleSetCredentials(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));

  // Import root node with just extended private. This should fail
  // with a locked wallet.
  request = Json::Value();
  response = Json::Value();
  request["ext_prv_b58"] = "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi";
  EXPECT_TRUE(api.HandleRestoreNode(request, response));
  EXPECT_FALSE(api.DidResponseSucceed(response));

  // Restore root node with both
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8";
  request["ext_prv_enc"] = "728f52c0144d0d48ae888b01ba9392b84e79c617291263e2c39c9369f7b4f9b0c3d856d6934efaba17b76582570f0719d642532c3cc668b511dff38cd089f098c4a87e3ea8de334c5348322f18844cfc4d81650f7c3b3f2301d81ac06f87b9ef9e99b3f5e9d60ea59dffb16fb8fcd3af";
  EXPECT_TRUE(api.HandleRestoreNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x3442193e", response["fp"].asString());
  EXPECT_EQ("0x00000000", response["pfp"].asString());

  // Restore child node
  request = Json::Value();
  response = Json::Value();
  request["ext_pub_b58"] = "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw";
  request["ext_prv_enc"] = "4615e7885c69e2ccb4e8c2617de746487e576651e02c4f5dbc780d1d485647a197092738710acb7b50468978c1ded4605f508a7677a82a0690c415529acaa0eea778041aa8405433c0244179e1a271ba745c54e92eaa2b98fa7f222b9e85545f2a2ddbbd215f332c3e7fc4d1e71b778b";
  EXPECT_TRUE(api.HandleRestoreNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x5c1bd648", response["fp"].asString());
  EXPECT_EQ("0x3442193e", response["pfp"].asString());

  // Now unlock wallet so we can derive children.
  request = Json::Value();
  response = Json::Value();
  request["passphrase"] = "foo";
  EXPECT_TRUE(api.HandleUnlock(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));

  // Confirm that the wallet correctly distinguished between restored
  // root and child by deriving a different child.
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/0'/1";
  request["is_watch_only"] = false;  // TODO(miket): true fails!
  EXPECT_TRUE(api.HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0xbef5a2f9", response["fp"].asString());

  // Import root node with just extended private. This is possible
  // with an unlocked wallet.
  request = Json::Value();
  response = Json::Value();
  request["ext_prv_b58"] = "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi";
  EXPECT_TRUE(api.HandleRestoreNode(request, response));
  EXPECT_FALSE(api.DidResponseSucceed(response));

  // Import root node with just extended private. This should succeed
  // with an unlocked wallet.
  request = Json::Value();
  response = Json::Value();
  request["ext_prv_b58"] = "xprv9s21ZrQH143K4EVCsxGjJncM1vLdwwi3CZ4ecGrV8X1ieEJpiDjYdE2PxTu4zvKVkR9e8RW9JRHGmNvbQMusmgFDayakzs68YXYvyJw3rSU";
  EXPECT_TRUE(api.HandleImportRootNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x8bb9cbc0", response["fp"].asString());
}
