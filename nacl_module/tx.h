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

#include <map>
#include <string>
#include <vector>

#include "types.h"

class Node;
class Transaction;

// https://en.bitcoin.it/wiki/Transactions
class TxIn {
 public:
  TxIn(std::istream& is);
  TxIn(const std::string& coinbase_message);
  TxIn(const Transaction& tx, uint32_t tx_n);
  TxIn(const bytes_t& hash,
       uint32_t index,
       const bytes_t& script,
       const bytes_t& hash160);

  const bytes_t& prev_txo_hash() const { return prev_txo_hash_; }
  uint32_t prev_txo_index() const { return prev_txo_index_; }

  const bytes_t& script() const { return script_; }
  void set_script(const bytes_t& script) { script_ = script; }
  void ClearScriptSig() { script_.clear(); }

  const bytes_t& hash160() const { return hash160_; }
  void set_hash160(const bytes_t& hash160) { hash160_ = hash160; }

  void should_serialize_script(bool should_serialize_script) {
    should_serialize_script_ = should_serialize_script;
  }

  bytes_t Serialize() const;

 private:
  bytes_t prev_txo_hash_;
  uint32_t prev_txo_index_;
  bytes_t script_;
  uint32_t sequence_no_;

  // Begin unserialized parts
  bytes_t hash160_;  // The address that needs to sign this input
  bool should_serialize_script_;
};
typedef std::vector<TxIn> tx_ins_t;

class TxOut {
 public:
  // Creating a recipient.
  TxOut(uint64_t value, const bytes_t& recipient_hash160);
  // Restoring from octets.
  TxOut(std::istream& is);
  // Generating an unspent txo list.
  TxOut(uint64_t value, const bytes_t& script,
        uint32_t tx_output_n, const bytes_t& tx_hash);

  // The address whose private key is needed to spend this output.
  // Empty if we don't know how to parse the output script.
  bytes_t GetSigningAddress() const;

  uint64_t value() const { return value_; }
  void set_value(uint64_t new_value) { value_ = new_value; }
  const bytes_t& script() const { return script_; }

  uint32_t tx_output_n() const { return tx_output_n_; }
  void set_tx_output_n(uint32_t n) { tx_output_n_ = n; }

  bytes_t Serialize() const;

  void MarkSpent() { is_spent_ = true; }
  bool is_spent() const { return is_spent_; }

  const bytes_t& tx_hash() const { return tx_hash_; }

 private:
  uint64_t value_;
  bytes_t script_;
  uint32_t tx_output_n_;
  bool is_spent_;

  // set only for unspent_txos
  bytes_t tx_hash_;
};
typedef std::vector<TxOut> tx_outs_t;

class Transaction {
 public:
  Transaction();
  Transaction(std::istream& is);

  bytes_t Serialize() const;
  bytes_t Sign(const Node& node,
               const tx_outs_t& unspent_txos,
               const tx_outs_t& desired_txos,
               const TxOut& change_address,
               uint64_t fee,
               int& error_code);

  uint32_t version() const { return version_; }
  const tx_ins_t& inputs() const { return inputs_; }
  const tx_outs_t& outputs() const { return outputs_; }
  uint32_t lock_time() const { return lock_time_; }
  const bytes_t& hash() const { return hash_; }

  void MarkOutputSpent(uint32_t index) { outputs_[index].MarkSpent(); }

  void Add(const TxIn& tx_in);
  void Add(const TxOut& tx_out);

 private:
  void UpdateHash();
  uint64_t AddRecipientValues(const tx_outs_t& txos);
  bool IdentifyUnspentTxos(const tx_outs_t& unspent_txos,
                           uint64_t value,
                           uint64_t fee,
                           tx_outs_t& required_txos,
                           uint64_t& change_value,
                           int& error_code);
  bool GenerateKeysForUnspentTxos(const Node& node,
                                  const tx_outs_t& txos,
                                  std::map<bytes_t, bytes_t>& signing_keys,
                                  std::map<bytes_t, bytes_t>&
                                  signing_public_keys,
                                  int& error_code);
  bool CopyUnspentTxosToTxins(const tx_outs_t& required_txos,
                              int& error_code);
  bool GenerateScriptSigs(std::map<bytes_t, bytes_t>& signing_keys,
                          std::map<bytes_t, bytes_t>& signing_public_keys,
                          int& error_code);

  uint32_t version_;
  tx_ins_t inputs_;
  tx_outs_t outputs_;
  uint32_t lock_time_;
  bytes_t hash_;
};
typedef std::vector<Transaction> transactions_t;

class TransactionManager {
 public:
  void Add(const Transaction& transaction);
  bool Exists(const bytes_t& hash);
  Transaction& Get(const bytes_t& hash);
  tx_outs_t GetUnspentTxos();
  uint64_t GetUnspentValue();

 private:
  typedef std::map<bytes_t, Transaction> tx_hashes_to_txs_t;
  tx_hashes_to_txs_t tx_hashes_to_txs_;
};
