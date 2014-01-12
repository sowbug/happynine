#include <iostream>
#include <fstream>
#include <string>

#include "base58.h"
#include "gtest/gtest.h"
#include "node.h"
#include "node_factory.h"
#include "tx.h"
#include "types.h"

TEST(TxTest, BasicTransaction) {
  // Using parts of BIP 0032 Test Vector 1.
  //
  // - Root master key m is fingerprint 3442193e
  // - Sending account m/0'/0 is fingerprint d6936720
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
  const bytes_t EXPECTED_SIGNED_TX(unhexlify(
    "010000000118fb619d7eb55d5ef9fca351de16ecbdb2340c9f49add372cb203affde5"
    "fb9470601000000ffffffff02ff7f0000000000001976a9146b468a091d50dfb75572"
    "00c46d0c1999d060a63788ac0260f505000000001976a9146dc73af1c96ff68e9dbde"
    "cd7453bad59bf0c83a488ac00000000"));

  const std::string XPRV("xprv9wTYmMFdV23N21MM6dLNavSQV7Sj7meSPXx6AV5eTdqqGL"
                         "jycVjb115Ec5LgRAXscPZgy5G4jQ9csyyZLN3PZLxoM1h3BoPuE"
                         "JzsgeypdKj");
  Node* sending_node =
    NodeFactory::CreateNodeFromExtended(Base58::fromBase58Check(XPRV));

  // https://blockchain.info/tx/47b95fdeff3a20cb72d3ad499f0c34b2bdec16de51a3fcf95e5db57e9d61fb18?format=json
  UnspentTxo unspent_txo;
  unspent_txo.hash = unhexlify("47b95fdeff3a20cb72d3ad499f0c34b2"
                               "bdec16de51a3fcf95e5db57e9d61fb18");
  unspent_txo.output_n = 262;
  unspent_txo.script = unhexlify("76a914"
                                 "77d896b0f85f72ae0f3d0487c432b23c28b71493"
                                 "88ac");
  unspent_txo.value = 100000000;
  unspent_txos_t unspent_txos;
  unspent_txos.push_back(unspent_txo);

  const bytes_t
    recipient_address(unhexlify("6b468a091d50dfb7557200c46d0c1999d060a637"));
  const uint64_t value = 32767;
  const uint64_t fee = 255;
  const uint32_t change_index = 1;

  bytes_t signed_tx;

  Tx tx(*sending_node,
        unspent_txos,
        recipient_address,
        value,
        fee,
        change_index);
  EXPECT_TRUE(tx.CreateSignedTransaction(signed_tx));
  EXPECT_EQ(EXPECTED_SIGNED_TX, signed_tx);

  delete sending_node;
}
