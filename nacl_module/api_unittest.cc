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

const bool BE_LOUD = false;

static void Spend(API &api,
                  const std::string& recipient,
                  uint64_t value,
                  uint64_t fee,
                  std::string& actual_sender,
                  uint64_t& actual_new_balance) {
  Json::Value request;
  Json::Value response;

  // Generate the signed transaction.
  request = Json::Value();
  response = Json::Value();
  request["recipients"][0]["addr_b58"] = recipient;
  request["recipients"][0]["value"] = (Json::Value::UInt64)value;
  request["fee"] = (Json::Value::UInt64)fee;
  request["sign"] = true;

  EXPECT_TRUE(api.HandleCreateTx(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_TRUE(response["tx"].asString().size() > 0);
  const bytes_t tx(unhexlify(response["tx"].asString()));

  // Broadcast the transaction (no code, pretend).
  if (BE_LOUD) {
    std::cerr << response["tx"].asString() << std::endl;
  }

  // Report that we got the transaction. Expect new balance.
  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] = to_hex(tx);
  EXPECT_TRUE(api.HandleReportTxs(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_TRUE(response["address_statuses"].size() > 0);
  actual_sender = response["address_statuses"][0]["addr_b58"].asString();

  actual_new_balance = 0;
  for (unsigned int i = 0; i < response["address_statuses"].size(); ++i) {
    actual_new_balance += response["address_statuses"][i]["value"].asUInt64();
  }
}

static bool StatusContains(Json::Value& statuses,
                           const std::string& expected_addr_b58,
                           uint64_t expected_value) {
  for (int i = 0; i < statuses.size(); ++i) {
    if (BE_LOUD) {
      std::cerr << "Rec'd: " << statuses[i]["addr_b58"].asString()
                << ": " << statuses[i]["value"].asUInt64() << std::endl;
    }
    if (statuses[i]["addr_b58"].asString() == expected_addr_b58 &&
        statuses[i]["value"].asUInt64() == expected_value) {
      return true;
    }
  }
  return false;
}

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
  request["is_watch_only"] = true;
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
  EXPECT_EQ("199TSaKH54KeWDm5cs7r43oe1ccaxVrBgC",  // m/0'/0/1
            response["address_statuses"][0]["addr_b58"].asString());
  EXPECT_EQ("1GuwtbNdTBeXL8ZdjHSV69MeERtwQsgLZd",  // m/0'/1/1
            response["address_statuses"][8 + 0]["addr_b58"].asString());

  // Pretend we sent blockchain.address.get_history for each address
  // and got back some stuff.
  request = Json::Value();
  response = Json::Value();
  const std::string HASH_TX =
    "1bcbf3b8244b25e4430d2abf706f5f53a16ad8ff2a42129fa9ca79477b905fbd";
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

  // https://blockchain.info/tx/1bcbf3b8244b25e4430d2abf706f5f53a16ad8ff2a42129fa9ca79477b905fbd
  // 199TSaKH54KeWDm5cs7r43oe1ccaxVrBgC - 0.00029
  expected_balance += 29000;
  request["txs"][0]["tx"] =
    "01000000018498a6567575912c5b891afa51d028b250465c2423fafa121b7dfe8c9382de"
    "d3000000008b48304502207a9e02fba54f78c220ef1d3c9c2e40f49b042a3e00c6073133"
    "97d02109d9907d022100f87cbf506772763cf6a5b8cd63ec2d9c574bc956af892f0d87a9"
    "3b339f115b03014104c3ff3d7202a81877b8537ed836529269b79ce245d69aaf52907514"
    "cb412bbb93bf61e66a72dba22064757236063cd9ddd2094e9356bc62e955ea7752e7aa5b"
    "7bffffffff0148710000000000001976a914595a67df1963dc16c5567abdd4a6443c8278"
    "0d1688ac00000000";
  EXPECT_TRUE(api.HandleReportTxs(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ(1, response["address_statuses"].size());
  EXPECT_EQ(expected_balance,
            response["address_statuses"][0]["value"].asUInt64());

  // Spend some of the funds in the wallet.
  uint64_t amount_to_spend = 888;
  uint64_t fee = 42;

  std::string actual_sender;
  uint64_t actual_new_balance;
  Spend(api,
        "1CUBwHRHD4D4ckRBu81n8cboGVUP9Ve7m4",
        amount_to_spend,
        fee,
        actual_sender,
        actual_new_balance);
  expected_balance -= amount_to_spend;
  expected_balance -= fee;
  EXPECT_EQ(expected_balance, actual_new_balance);
  EXPECT_EQ("199TSaKH54KeWDm5cs7r43oe1ccaxVrBgC", actual_sender);

  // Now spend the rest of the funds in the wallet. This is different
  // because it requires the wallet to report a zero balance without
  // reporting *all* zero balances.
  fee = 2;
  amount_to_spend = expected_balance - fee;
  Spend(api,
        "1CUBwHRHD4D4ckRBu81n8cboGVUP9Ve7m4",
        amount_to_spend,
        fee,
        actual_sender,
        actual_new_balance);
  expected_balance -= amount_to_spend;
  expected_balance -= fee;
  EXPECT_EQ("1GuwtbNdTBeXL8ZdjHSV69MeERtwQsgLZd", actual_sender);
  EXPECT_EQ(expected_balance, actual_new_balance);
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
  request["is_watch_only"] = true;
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

TEST(ApiTest, ReportActualTransactions) {
  Credentials c;
  Wallet w(c);
  API api(c, w);
  Json::Value request;
  Json::Value response;

  request["new_passphrase"] = "foo";
  EXPECT_TRUE(api.HandleSetPassphrase(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));

  // Import an xprv (see test/h9-test-vectors-script.txt)
  request = Json::Value();
  response = Json::Value();
  request["seed_hex"] = "baddecaf99887766554433221100";
  EXPECT_TRUE(api.HandleDeriveRootNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));

  // Derive a child
  request = Json::Value();
  response = Json::Value();
  request["path"] = "m/0'";
  request["is_watch_only"] = false;
  EXPECT_TRUE(api.HandleDeriveChildNode(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ("0x5adb92c0", response["fp"].asString());
  EXPECT_EQ(8 + 8, response["address_statuses"].size());

  // https://blockchain.info/tx/1bcbf3b8244b25e4430d2abf706f5f53a16ad8ff2a42129fa9ca79477b905fbd
  // 199TSaKH54KeWDm5cs7r43oe1ccaxVrBgC - 0.00029
  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] =
    "01000000018498a6567575912c5b891afa51d028b250465c2423fafa121b7dfe8c9382de"
    "d3000000008b48304502207a9e02fba54f78c220ef1d3c9c2e40f49b042a3e00c6073133"
    "97d02109d9907d022100f87cbf506772763cf6a5b8cd63ec2d9c574bc956af892f0d87a9"
    "3b339f115b03014104c3ff3d7202a81877b8537ed836529269b79ce245d69aaf52907514"
    "cb412bbb93bf61e66a72dba22064757236063cd9ddd2094e9356bc62e955ea7752e7aa5b"
    "7bffffffff0148710000000000001976a914595a67df1963dc16c5567abdd4a6443c8278"
    "0d1688ac00000000";
  EXPECT_TRUE(api.HandleReportTxs(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ(1, response["address_statuses"].size());
  EXPECT_TRUE(StatusContains(response["address_statuses"],
                             "199TSaKH54KeWDm5cs7r43oe1ccaxVrBgC", 29000));

  // https://blockchain.info/tx/100dc580584e9b709fd42be707b6eaf8205efebb9b5356affbc311799be1beaf
  // 1PB8bTcRXz1u84Yxn5JpRXDUhXwc7DxUt1 - 0.00014 BTC (not in wallet)
  // 1GuwtbNdTBeXL8ZdjHSV69MeERtwQsgLZd - 0.00014 BTC (in wallet)
  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] =
    "0100000001bd5f907b4779caa99f12422affd86aa1535f6f70bf2a0d43e4254b24b8f3cb"
    "1b000000006a47304402201ede3d04b7a6c22aec5421fc089e464ce3bafc012f40d24010"
    "7bf1d19be1a410022027b157c524c3211528ed32f1ec3a971a0cffe173b0b91c2c801469"
    "87a37ddbfe012103a434f5b4f9d99a4c786a44dd50d5b7832ec417ae7150f049904e3a0f"
    "544621a2ffffffff02b0360000000000001976a914f33d441fd850487267ed7681b19550"
    "761bf1e4cd88acb0360000000000001976a914ae8d5613d9e7e7281451c0abf5424a3e42"
    "95fc5088ac00000000";
  EXPECT_TRUE(api.HandleReportTxs(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ(2, response["address_statuses"].size());
  EXPECT_TRUE(StatusContains(response["address_statuses"],
                             "199TSaKH54KeWDm5cs7r43oe1ccaxVrBgC", 0));
  EXPECT_TRUE(StatusContains(response["address_statuses"],
                             "1GuwtbNdTBeXL8ZdjHSV69MeERtwQsgLZd", 14000));

  // https://blockchain.info/tx/TODO-REPLACE-WITH-ACTUAL
  // 1PB8bTcRXz1u84Yxn5JpRXDUhXwc7DxUt1 - 0.00013 BTC (not in wallet)
  request = Json::Value();
  response = Json::Value();
  request["txs"][0]["tx"] =
    "0100000001afbee19b7911c3fbaf56539bbbfe5e20f8eab607e72bd49f709b4e5880c50d10010000006b48304502201743eb3b1385618a1c1d67dda780498a6bd4dbe586489049def22f0795491c5d022100b822a98051574c4a8d9670b548251c1c7a2d27f47eeb732f456775d84e2e04db012102c372ba6e50d79c1fa02a32a22d0350b176935a78fd75c134e246c9ac25c98a31ffffffff01c8320000000000001976a914f33d441fd850487267ed7681b19550761bf1e4cd88ac00000000";
  EXPECT_TRUE(api.HandleReportTxs(request, response));
  EXPECT_TRUE(api.DidResponseSucceed(response));
  EXPECT_EQ(1, response["address_statuses"].size());
  EXPECT_TRUE(StatusContains(response["address_statuses"],
                             "1GuwtbNdTBeXL8ZdjHSV69MeERtwQsgLZd", 0));
}
