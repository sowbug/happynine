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

#include "blockchain.h"

#include "gtest/gtest.h"

#include "base58.h"

TEST(BlockchainTest, HappyPath) {
  Blockchain blockchain;

  EXPECT_EQ(0, blockchain.max_block_height());
  EXPECT_EQ(0, blockchain.GetBlockTimestamp(0));

  blockchain.AddBlock(0, 1231006505);
  EXPECT_EQ(1231006505, blockchain.GetBlockTimestamp(0));

  blockchain.AddBlock(1, 1231469665);
  EXPECT_EQ(1, blockchain.max_block_height());

  const bytes_t TX_0E3E(unhexlify("01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac00000000"));
  const bytes_t TX_0E3E_HASH(unhexlify("0e3e2357e806b6cdb1f70b54c3a3a17b6714ee1f0e68bebb44a74b1efd512098"));
  blockchain.AddTransaction(TX_0E3E);
  EXPECT_EQ(0, blockchain.GetTransactionHeight(TX_0E3E_HASH));
  blockchain.ConfirmTransaction(TX_0E3E_HASH, 1);
  EXPECT_EQ(1, blockchain.GetTransactionHeight(TX_0E3E_HASH));

  const Blockchain::address_t
    ADDR_12c6(Base58::fromAddress("12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX"));
  EXPECT_EQ(50 * SATOSHIS_IN_BTC, blockchain.GetAddressBalance(ADDR_12c6));
  EXPECT_EQ(1, blockchain.GetAddressTxCount(ADDR_12c6));

  // Get all unspent txos.
  Blockchain::address_set_t addresses;
  tx_outs_t unspent_txos;
  blockchain.GetUnspentTxos(addresses, unspent_txos);
  EXPECT_EQ(1, unspent_txos.size());

  // Get a filtered unspent txo list that should be the null set.
  addresses.insert(Base58::fromAddress("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"));
  blockchain.GetUnspentTxos(addresses, unspent_txos);
  EXPECT_EQ(0, unspent_txos.size());

  // Get a filtered unspent txo list that should be just one item.
  addresses.insert(ADDR_12c6);
  blockchain.GetUnspentTxos(addresses, unspent_txos);
  EXPECT_EQ(1, unspent_txos.size());
}
