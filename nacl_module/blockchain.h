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

#if !defined(__BLOCKCHAIN_H__)
#define __BLOCKCHAIN_H__

#include <map>
#include <set>

#include "tx.h"
#include "types.h"

// An in-memory, small-scale blockchain.
class Blockchain {
 public:
  Blockchain();
  virtual ~Blockchain();

  typedef bytes_t tx_t;
  typedef bytes_t tx_hash_t;
  typedef bytes_t address_t;
  typedef std::set<address_t> address_set_t;

  // Blocks
  uint64_t max_block_height() const { return max_block_height_; }
  void ConfirmBlock(uint64_t height, uint64_t timestamp);
  uint64_t GetBlockTimestamp(uint64_t height);

  // Transactions
  void AddTransaction(const tx_t& transaction);
  void ConfirmTransaction(const tx_hash_t& tx_hash, uint64_t height);
  void GetUnspentTxos(const address_set_t& addresses, tx_outs_t& unspent_txos);
  uint64_t GetTransactionHeight(const tx_hash_t& tx_hash);
  // TODO: GetTransactions(const address_set_t& addresses, _______);

  // Addresses
  uint64_t GetAddressBalance(const address_t& address);
  uint64_t GetAddressTxCount(const address_t& address);

 private:
  Transaction* GetTransaction(const tx_hash_t& tx_hash);
  void MarkSpentTxos();
  void CalculateUnspentTxos();
  void CalculateBalances();
  void CalculateTransactionCounts();

  typedef std::map<tx_hash_t, Transaction*> transaction_map_t;

  uint64_t max_block_height_;
  std::map<uint64_t, uint64_t> block_timestamps_;
  std::map<tx_hash_t, uint64_t> tx_heights_;
  transaction_map_t transactions_;

  std::map<address_t, uint64_t> balances_;
  std::map<address_t, uint64_t> tx_counts_;

  tx_outs_t unspent_txos_;

  DISALLOW_EVIL_CONSTRUCTORS(Blockchain);
};

#endif  // #if !defined(__BLOCKCHAIN_H__)
