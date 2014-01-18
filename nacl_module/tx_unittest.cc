#include <iostream>
#include <fstream>
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
  // - change address is m/0'/0/1 (we don't use the internal chain for now)
  //   - L22jhG8WTNmuRtqFvzvpnhe32F8FefJFfsLJpSr1CYsRrZCyTwKZ
  //   - 1B1TKfsCkW5LQ6R1kSXUx7hLt49m1kwz75
  //   - 6dc73af1c96ff68e9dbdecd7453bad59bf0c83a4
  // const std::string XPRV("xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ng"
  //                        "LNv1TxvUxt4cV1rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrn"
  //                        "YeSvkzY7d2bhkJ7");
  // Node* sending_node =
  //   NodeFactory::CreateNodeFromExtended(Base58::fromBase58Check(XPRV));

  // // https://blockchain.info/tx/47b95fdeff3a20cb72d3ad499f0c34b2bdec16de51a3fcf95e5db57e9d61fb18?format=json
  // UnspentTxo unspent_txo;
  // unspent_txo.hash = unhexlify("47b95fdeff3a20cb72d3ad499f0c34b2"
  //                              "bdec16de51a3fcf95e5db57e9d61fb18");
  // unspent_txo.output_n = 262;
  // unspent_txo.script = unhexlify("76a914"
  //                                "77d896b0f85f72ae0f3d0487c432b23c28b71493"
  //                                "88ac");
  // unspent_txo.value = 100000000;
  // unspent_txos_t unspent_txos;
  // unspent_txos.push_back(unspent_txo);

  // const bytes_t
  //   recipient_address(unhexlify("6b468a091d50dfb7557200c46d0c1999d060a637"));
  // const uint64_t value = 32767;
  // const uint64_t fee = 255;
  // const uint32_t change_index = 1;

  // bytes_t signed_tx;

  // int error_code = 0;
  // Transaction transaction(*sending_node,
  //       recipient_address,
  //       value,
  //       fee,
  //       change_index);

  //       unspent_txos,

  // EXPECT_TRUE(tx.CreateSignedTransaction(signed_tx, error_code));
  // EXPECT_EQ(0, error_code);

  // // Not sure how to verify this. It's different every time by design.
  // std::cerr << to_hex(signed_tx) << std::endl;

  // delete sending_node;
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

  {
    // Add one. Confirm its outputs are considered unspent.
    TransactionManager tm;
    tm.Add(tx);
    tx_outs_t unspent = tm.GetUnspentTxos();
    EXPECT_EQ(tx.outputs().size(), unspent.size());
    EXPECT_EQ(TX_1_0_VALUE, tm.GetUnspentValue());

    // Add the other, which spends the first. Confirm the prior output
    // is spent, and the new ones are unspent.
    tm.Add(tx2);

    unspent = tm.GetUnspentTxos();
    EXPECT_EQ(tx2.outputs().size(), unspent.size());
    EXPECT_EQ(TX_2_0_VALUE + TX_2_1_VALUE, tm.GetUnspentValue());
  }

  {
    // Add out-of-order. Should be same result.
    TransactionManager tm;
    tm.Add(tx2);

    tx_outs_t unspent = tm.GetUnspentTxos();
    EXPECT_EQ(tx2.outputs().size(), unspent.size());
    EXPECT_EQ(TX_2_0_VALUE + TX_2_1_VALUE, tm.GetUnspentValue());

    tm.Add(tx);

    unspent = tm.GetUnspentTxos();
    EXPECT_EQ(tx2.outputs().size(), unspent.size());
    EXPECT_EQ(TX_2_0_VALUE + TX_2_1_VALUE, tm.GetUnspentValue());
  }
}
