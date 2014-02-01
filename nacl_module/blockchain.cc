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

#include <istream>
#include <sstream>

#include "debug.h"

Blockchain::Blockchain()
  : max_block_height_(0) {
}

Blockchain::~Blockchain() {
  // DELETE transactions_
}

void Blockchain::ConfirmBlock(uint64_t height, uint64_t timestamp) {
  block_timestamps_[height] = timestamp;
  if (height > max_block_height_) {
    max_block_height_ = height;
  }
}

Transaction* Blockchain::GetTransaction(const tx_hash_t& tx_hash) {
  if (transactions_.count(tx_hash) == 0) {
    return NULL;
  }
  return transactions_[tx_hash];
}

void Blockchain::MarkSpentTxos() {
  // Check every input to see which output it spends, and if we know
  // about that output, mark it spent.
  for (transaction_map_t::const_iterator i = transactions_.begin();
       i != transactions_.end();
       ++i) {
    for (tx_ins_t::const_iterator j = i->second->inputs().begin();
         j != i->second->inputs().end();
         ++j) {
      if (GetTransaction(j->prev_txo_hash()) != NULL) {
        Transaction* affected_tx = GetTransaction(j->prev_txo_hash());
        affected_tx->MarkOutputSpent(j->prev_txo_index());
      }
    }
  }
}

void Blockchain::RebuildUnspentTxos() {
  unspent_txos_.clear();

  for (transaction_map_t::const_iterator i = transactions_.begin();
       i != transactions_.end();
       ++i) {
    for (tx_outs_t::const_iterator j = i->second->outputs().begin();
         j != i->second->outputs().end();
         ++j) {
      if (!j->is_spent()) {
        unspent_txos_.push_back(TxOut(j->value(), j->script(),
                                      j->tx_output_n(), i->first));
      }
    }
  }
}

void Blockchain::UpdateBalancesFromUnspentTxos() {
  balances_.clear();
  for (tx_outs_t::const_iterator i = unspent_txos_.begin();
       i != unspent_txos_.end();
       ++i) {
    balances_[i->GetSigningAddress()] += i->value();
  }
}

void Blockchain::UpdateTransactionCounts(const Transaction* tx) {
  for (tx_outs_t::const_iterator i = tx->outputs().begin();
       i != tx->outputs().end();
       ++i) {
    ++tx_counts_[i->GetSigningAddress()];
  }

  // TODO(miket): because this method builds on existing state rather
  // than recalculating all tx counts, it might miss some counts if
  // the prior transaction wasn't here yet but arrives later.
  for (tx_ins_t::const_iterator i = tx->inputs().begin();
       i != tx->inputs().end();
       ++i) {
    if (GetTransaction(i->prev_txo_hash())) {
      Transaction* affected_tx = GetTransaction(i->prev_txo_hash());
      const TxOut *txo = &affected_tx->outputs()[i->prev_txo_index()];
      ++tx_counts_[txo->GetSigningAddress()];
    }
  }
}

void Blockchain::AddTransaction(const tx_t& transaction) {
  std::istringstream is;
  const char* p = reinterpret_cast<const char*>(&transaction[0]);
  is.rdbuf()->pubsetbuf(const_cast<char*>(p), transaction.size());
  Transaction* tx = new Transaction(is);
  transactions_[tx->hash()] = tx;

  MarkSpentTxos();
  RebuildUnspentTxos();
  UpdateBalancesFromUnspentTxos();
  UpdateTransactionCounts(tx);
}

void Blockchain::ConfirmTransaction(const tx_hash_t& tx_hash,
                                    uint64_t height) {
  tx_heights_[tx_hash] = height;
}

void Blockchain::GetUnspentTxos(const address_set_t& addresses,
                                tx_outs_t& unspent_txos) {
  unspent_txos.clear();
  for (tx_outs_t::const_iterator i = unspent_txos_.begin();
       i != unspent_txos_.end();
       ++i) {
    if (addresses.empty() ||
        addresses.count(i->GetSigningAddress()) != 0) {
      unspent_txos.push_back(*i);
    }
  }
}

uint64_t Blockchain::GetTransactionHeight(const tx_hash_t& tx_hash) {
  return tx_heights_[tx_hash];
}

uint64_t Blockchain::GetBlockTimestamp(uint64_t height) {
  return block_timestamps_[height];
}

uint64_t Blockchain::GetAddressBalance(const Blockchain::address_t& address) {
  return balances_[address];
}

uint64_t Blockchain::GetAddressTxCount(const Blockchain::address_t& address) {
  return tx_counts_[address];
}
