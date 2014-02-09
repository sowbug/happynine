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

#include <cstdlib>
#include <istream>
#include <memory>
#include <sstream>

#include "debug.h"

HistoryItem::HistoryItem(const bytes_t& tx_hash,
                         const bytes_t& hash160,
                         uint64_t timestamp,
                         int64_t value,
                         uint64_t fee,
                         bool inputs_are_known)
  : tx_hash_(tx_hash), hash160_(hash160), timestamp_(timestamp),
    value_(value), fee_(fee), inputs_are_known_(inputs_are_known) {
  }

Blockchain::Blockchain()
  : max_block_height_(0) {
}

Blockchain::~Blockchain() {
  for (transaction_map_t::const_iterator i = transactions_.begin();
       i != transactions_.end();
       ++i) {
    delete i->second;
  }
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

void Blockchain::CalculateUnspentTxos() {
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

void Blockchain::CalculateBalances() {
  balances_.clear();

  for (tx_outs_t::const_iterator i = unspent_txos_.begin();
       i != unspent_txos_.end();
       ++i) {
    balances_[i->GetSigningAddress()] += i->value();
  }
}

void Blockchain::CalculateTransactionCounts() {
  tx_counts_.clear();

  for (transaction_map_t::const_iterator txh = transactions_.begin();
       txh != transactions_.end();
       ++txh) {
    const Transaction* tx = txh->second;

    // Check the inputs.
    for (tx_ins_t::const_iterator i = tx->inputs().begin();
         i != tx->inputs().end();
         ++i) {
      if (GetTransaction(i->prev_txo_hash())) {
        Transaction* affected_tx = GetTransaction(i->prev_txo_hash());
        const TxOut *txo = &affected_tx->outputs()[i->prev_txo_index()];
        ++tx_counts_[txo->GetSigningAddress()];
      }
    }

    // Check the outputs.
    for (tx_outs_t::const_iterator i = tx->outputs().begin();
         i != tx->outputs().end();
         ++i) {
      ++tx_counts_[i->GetSigningAddress()];
    }
  }
}

void Blockchain::UpdateDerivedInformation() {
  MarkSpentTxos();
  CalculateUnspentTxos();
  CalculateBalances();
  CalculateTransactionCounts();
}

void Blockchain::AddTransaction(const tx_t& transaction) {
  std::istringstream is;
  const char* p = reinterpret_cast<const char*>(&transaction[0]);
  is.rdbuf()->pubsetbuf(const_cast<char*>(p), transaction.size());
  Transaction* tx = new Transaction(is);
  transactions_[tx->hash()] = tx;

  UpdateDerivedInformation();
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

uint64_t Blockchain::GetTransactionTimestamp(const tx_hash_t& tx_hash) {
  if (tx_heights_.count(tx_hash) == 0) {
    return 0;
  }
  return GetBlockTimestamp(tx_heights_[tx_hash]);
}

uint64_t Blockchain::GetBlockTimestamp(uint64_t height) {
  if (block_timestamps_.count(height) != 0) {
    return block_timestamps_[height];
  }
  return 0;
}

uint64_t Blockchain::GetAddressBalance(const Blockchain::address_t& address) {
  if (balances_.count(address) != 0) {
    return balances_[address];
  }
  return 0;
}

uint64_t Blockchain::GetAddressTxCount(const Blockchain::address_t& address) {
  if (tx_counts_.count(address) != 0) {
    return tx_counts_[address];
  }
  return 0;
}

void Blockchain::
GetTransactionsForAddresses(const address_set_t& addresses,
                            std::vector<const Transaction*>& transactions) {
  transactions.clear();
  for (transaction_map_t::const_iterator txh = transactions_.begin();
       txh != transactions_.end();
       ++txh) {
    const Transaction* tx = txh->second;
    bool is_in_address_set = false;

    for (tx_ins_t::const_iterator i = tx->inputs().begin();
         i != tx->inputs().end();
         ++i) {
      if (GetTransaction(i->prev_txo_hash())) {
        Transaction* source_tx = GetTransaction(i->prev_txo_hash());
        const TxOut *txo = &source_tx->outputs()[i->prev_txo_index()];
        if (addresses.count(txo->GetSigningAddress()) != 0) {
          is_in_address_set = true;
          break;
        }
      }
    }

    if (!is_in_address_set) {
      for (tx_outs_t::const_iterator i = tx->outputs().begin();
           i != tx->outputs().end();
           ++i) {
        if (addresses.count(i->GetSigningAddress()) != 0) {
          is_in_address_set = true;
          break;
        }
      }
    }

    if (is_in_address_set) {
      transactions.push_back(tx);
    }
  }
}

HistoryItem Blockchain::
TransactionToHistoryItem(const address_set_t& addresses,
                         const Transaction* transaction) {
  std::map<address_t, int64_t> balances;
  int64_t all_txin = 0;
  int64_t all_txo = 0;
  bool inputs_are_known = true;

  // Tally up all inputs, mapped to addresses we care about.
  for (tx_ins_t::const_iterator i = transaction->inputs().begin();
       i != transaction->inputs().end();
       ++i) {
    if (GetTransaction(i->prev_txo_hash())) {
      Transaction* source_tx = GetTransaction(i->prev_txo_hash());
      const TxOut *txo = &source_tx->outputs()[i->prev_txo_index()];
      all_txin -= txo->value();
      if (addresses.count(txo->GetSigningAddress()) != 0) {
        balances[txo->GetSigningAddress()] -= txo->value();
      }
    } else {
      inputs_are_known = false;
    }
  }

  // Same with outputs.
  for (tx_outs_t::const_iterator i = transaction->outputs().begin();
       i != transaction->outputs().end();
       ++i) {
    all_txo += i->value();
    if (addresses.count(i->GetSigningAddress()) != 0) {
      balances[i->GetSigningAddress()] += i->value();
    }
  }

  // Now build the transaction description.
  uint64_t fee = 0;
  if (all_txin != 0 && inputs_are_known) {
    fee = -(all_txin + all_txo);
  }
  int64_t net_to_wallet = 0;
  address_t most_affected_address;
  int64_t biggest_effect = 0;
  for (std::map<address_t, int64_t>::const_iterator i = balances.begin();
       i != balances.end();
       ++i) {
    if (abs(i->second) > biggest_effect) {
      biggest_effect = abs(i->second);
      most_affected_address = i->first;
    }
    net_to_wallet += i->second;
  }
  return HistoryItem(transaction->hash(),
                     most_affected_address,
                     GetTransactionTimestamp(transaction->hash()),
                     net_to_wallet,
                     fee,
                     inputs_are_known);
}
