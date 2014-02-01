#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include "base58.h"
#include "gtest/gtest.h"
#include "node.h"
#include "node_factory.h"
#include "tx.h"
#include "types.h"

#include "test_transactions.hex"

TEST(TxTest, BasicTransaction) {
  // Using parts of BIP 0032 Test Vector 1.
  //
  // - Root master key m is fingerprint 3442193e
  // - Sending account m/0' is fingerprint 5c1bd648
  // - unspent txo was sent to m/0'/0/0
  //   - L3dzheSvHWc2scJdiikdZmYdFzPcvZMAnT5g62ikVWZdBewoWpL1
  //   - 1BvgsfsZQVtkLS69NvGF8rw6NZW2ShJQHr
  //   - 77d896b0f85f72ae0f3d0487c432b23c28b71493
  // - recipient is m/1'/0/0
  //   - L51Rt2TvamJzvbBJz1UG27RMtfvPLwCKqFsXRYgL4EXRTjvMiYqM
  //   - 1AnDogBPp4VL48Nrh7h8LquV68ZzXNtwcq
  //   - 6b468a091d50dfb7557200c46d0c1999d060a637
  // - change address is m/0'/0/1 (external chain)
  //   - L22jhG8WTNmuRtqFvzvpnhe32F8FefJFfsLJpSr1CYsRrZCyTwKZ
  //   - 1B1TKfsCkW5LQ6R1kSXUx7hLt49m1kwz75
  //   - 6dc73af1c96ff68e9dbdecd7453bad59bf0c83a4
  const std::string XPRV("xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ng"
                         "LNv1TxvUxt4cV1rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrn"
                         "YeSvkzY7d2bhkJ7");
  std::auto_ptr<Node>
    sending_node(NodeFactory::
                 CreateNodeFromExtended(Base58::fromBase58Check(XPRV)));

  // https://blockchain.info/tx/47b95fdeff3a20cb72d3ad499f0c34b2bdec16de51a3fcf95e5db57e9d61fb18?format=json
  TxOut unspent_txo(100000000,
                    unhexlify("76a914"
                              "77d896b0f85f72ae0f3d0487c432b23c28b71493"
                              "88ac"),
                    262,
                    unhexlify("47b95fdeff3a20cb72d3ad499f0c34b2"
                              "bdec16de51a3fcf95e5db57e9d61fb18")
                    );
  tx_outs_t unspent_txos;
  unspent_txos.push_back(unspent_txo);

  TxOut recipient_txo(32767,
                      unhexlify("6b468a091d50dfb7557200c46d0c1999d060a637"));
  tx_outs_t recipient_txos;
  recipient_txos.push_back(recipient_txo);

  std::auto_ptr<Node>
    change_node(NodeFactory::
                DeriveChildNodeWithPath(sending_node.get(),
                                        std::string("m/0/1")));
  TxOut change_txo(0, Base58::toHash160(change_node->public_key()));

  class TestKeyProvider : public KeyProvider {
  public:
    virtual bool GetKeysForAddress(const bytes_t& hash160,
                                   bytes_t& public_key,
                                   bytes_t& key) {
      //      std::cerr << to_hex(hash160) << std::endl;
      return false;
    }
  };
  TestKeyProvider tkp;

  Transaction transaction;
  int error_code = 0;
  bytes_t signed_tx = transaction.Sign(&tkp,
                                       unspent_txos,
                                       recipient_txos,
                                       change_txo,
                                       255,
                                       error_code);
  EXPECT_EQ(0, error_code);

  // // Not sure how to verify this. It's different every time by design.
  // std::cerr << to_hex(signed_tx) << std::endl;
}

TEST(TxTest, ParseRawTransaction) {
  std::istringstream is;
  const char* p = reinterpret_cast<const char*>(&TX_1[0]);
  is.rdbuf()->pubsetbuf(const_cast<char*>(p),
                        TX_1.size());

  Transaction tx(is);
  EXPECT_EQ(1, tx.version());
  EXPECT_EQ(230, tx.inputs().size());
  EXPECT_EQ(1, tx.outputs().size());
  EXPECT_EQ(0, tx.lock_time());

  // And for style points, serialize it again.
  const bytes_t tx_serialized = tx.Serialize();
  ASSERT_EQ(TX_1, tx_serialized);
}

TEST(TxTest, CoinbaseToFritterAway) {
  const std::string ADDR_B58("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa");
  const bytes_t ADDR(Base58::toHash160(Base58::fromBase58Check(ADDR_B58)));
  const std::string ADDR2_B58("16hhvFgjMxUBd1qXAKnFHNx5YbCTwG2EbJ");
  const bytes_t ADDR2(Base58::toHash160(Base58::fromBase58Check(ADDR2_B58)));

  uint64_t TX_1_0_VALUE = 100000000;
  uint64_t TX_2_0_VALUE = 39573729;
  uint64_t TX_2_1_VALUE = TX_1_0_VALUE - TX_2_0_VALUE;

  Transaction tx;
  {
    TxIn tx_in(std::string("BitCoin mined by Paul Krugman"));
    tx.Add(tx_in);
    TxOut tx_out(TX_1_0_VALUE, ADDR);
    tx.Add(tx_out);
  }
  EXPECT_FALSE(tx.outputs()[0].is_spent());

  Transaction tx2;
  {
    TxIn tx_in(tx, 0);  // zeroth input of first tx
    tx2.Add(tx_in);
    TxOut  tx_out(TX_2_0_VALUE, ADDR);
    tx2.Add(tx_out);
    TxOut tx2_out(TX_2_1_VALUE, ADDR2);
    tx2.Add(tx2_out);
  }
}
