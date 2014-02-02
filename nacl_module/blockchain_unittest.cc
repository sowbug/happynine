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
#include "test_constants.h"

TEST(BlockchainTest, HappyPath) {
  Blockchain blockchain;

  EXPECT_EQ(0, blockchain.max_block_height());
  EXPECT_EQ(0, blockchain.GetBlockTimestamp(0));

  blockchain.ConfirmBlock(0, 1231006505);
  EXPECT_EQ(1231006505, blockchain.GetBlockTimestamp(0));

  blockchain.ConfirmBlock(1, 1231469665);
  EXPECT_EQ(1, blockchain.max_block_height());

  blockchain.AddTransaction(TX_0E3E);
  EXPECT_EQ(0, blockchain.GetTransactionHeight(TX_0E3E_HASH));
  blockchain.ConfirmTransaction(TX_0E3E_HASH, 1);
  EXPECT_EQ(1, blockchain.GetTransactionHeight(TX_0E3E_HASH));

  EXPECT_EQ(50 * SATOSHIS_IN_BTC, blockchain.GetAddressBalance(ADDR_12c6));
  EXPECT_EQ(1, blockchain.GetAddressTxCount(ADDR_12c6));

  // Get all unspent txos.
  Blockchain::address_set_t address_filter;
  tx_outs_t unspent_txos;
  blockchain.GetUnspentTxos(address_filter, unspent_txos);
  EXPECT_EQ(1, unspent_txos.size());

  // Get a filtered unspent txo list that should be the null set.
  address_filter.insert(ADDR_1A1z);
  blockchain.GetUnspentTxos(address_filter, unspent_txos);
  EXPECT_EQ(0, unspent_txos.size());

  // Get a filtered unspent txo list that should be just one item.
  address_filter.insert(ADDR_12c6);
  blockchain.GetUnspentTxos(address_filter, unspent_txos);
  EXPECT_EQ(1, unspent_txos.size());

  address_filter.clear();
  blockchain.AddTransaction(TX_1BCB);
  blockchain.GetUnspentTxos(address_filter, unspent_txos);
  EXPECT_EQ(2, unspent_txos.size());

  blockchain.AddTransaction(TX_100D);
  blockchain.GetUnspentTxos(address_filter, unspent_txos);
  EXPECT_EQ(3, unspent_txos.size());
  EXPECT_EQ(14000, blockchain.GetAddressBalance(ADDR_1Guw));
  EXPECT_EQ(1, blockchain.GetAddressTxCount(ADDR_1Guw));

  blockchain.AddTransaction(TX_BFB1);
  blockchain.GetUnspentTxos(address_filter, unspent_txos);
  EXPECT_EQ(3, unspent_txos.size());

  EXPECT_EQ(50 * SATOSHIS_IN_BTC, blockchain.GetAddressBalance(ADDR_12c6));
  EXPECT_EQ(1, blockchain.GetAddressTxCount(ADDR_12c6));
  EXPECT_EQ(27000, blockchain.GetAddressBalance(ADDR_1PB8));
  EXPECT_EQ(2, blockchain.GetAddressTxCount(ADDR_1PB8));
  EXPECT_EQ(0, blockchain.GetAddressBalance(ADDR_1Guw));
  EXPECT_EQ(2, blockchain.GetAddressTxCount(ADDR_1Guw));
}

TEST(BlockchainTest, OutOfOrder) {
  Blockchain blockchain;

  blockchain.AddTransaction(TX_BFB1);
  blockchain.AddTransaction(TX_100D);
  blockchain.AddTransaction(TX_1BCB);

  EXPECT_EQ(27000, blockchain.GetAddressBalance(ADDR_1PB8));
  EXPECT_EQ(2, blockchain.GetAddressTxCount(ADDR_1PB8));
  EXPECT_EQ(0, blockchain.GetAddressBalance(ADDR_1Guw));
  EXPECT_EQ(2, blockchain.GetAddressTxCount(ADDR_1Guw));
}
