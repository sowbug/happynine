#include "tx.h"

#include <iostream>  // cerr
#include <istream>
#include <iterator>
#include <set>
#include <sstream>
#include <vector>

#include "base58.h"
#include "crypto.h"
#include "node.h"
#include "node_factory.h"

static uint16_t ReadUint16(std::istream& s) {
  return s.get() | (s.get() << 8);
}

static uint32_t ReadUint32(std::istream& s) {
  return s.get() | (s.get() << 8) | (s.get() << 16) | (s.get() << 24);
}

static uint64_t ReadUint64(std::istream& s) {
  return (uint64_t)s.get() |
    ((uint64_t)s.get() << 8) |
    ((uint64_t)s.get() << 16) |
    ((uint64_t)s.get() << 24) |
    ((uint64_t)s.get() << 32) |
    ((uint64_t)s.get() << 40) |
    ((uint64_t)s.get() << 48) |
    ((uint64_t)s.get() << 56);
}

static uint64_t ReadVarInt(std::istream& s) {
  uint64_t value = s.get();
  if (value < 0xfd) {
    return value;
  }
  if (value == 0xfd) {
    return ReadUint16(s);
  }
  if (value == 0xfe) {
    return ReadUint32(s);
  }
  return ReadUint64(s);
}

static void ReadIntoBytes(std::istream& s, bytes_t& b, uint32_t len) {
  b.resize(len);
  s.read(reinterpret_cast<char *>(&b[0]), b.capacity());
}

static void PushUint16(bytes_t& out, uint16_t value) {
  out.push_back((value) & 0xff);
  out.push_back((value >>  8) & 0xff);
}

static void PushUint32(bytes_t& out, uint32_t value) {
  out.push_back((value) & 0xff);
  out.push_back((value >>  8) & 0xff);
  out.push_back((value >> 16) & 0xff);
  out.push_back((value >> 24) & 0xff);
}

static void PushUint64(bytes_t& out, uint64_t value) {
  out.push_back((value) & 0xff);
  out.push_back((value >>  8) & 0xff);
  out.push_back((value >> 16) & 0xff);
  out.push_back((value >> 24) & 0xff);
  out.push_back((value >> 32) & 0xff);
  out.push_back((value >> 40) & 0xff);
  out.push_back((value >> 48) & 0xff);
  out.push_back((value >> 56) & 0xff);
}

static void PushVarInt(bytes_t& out, uint64_t value) {
  if (value < 0xfd) {
    out.push_back(value & 0xff);
    return;
  }
  if (value <= 0xffff) {
    out.push_back(0xfd);
    PushUint16(out, value & 0xffff);
  }
  if (value <= 0xffffffff) {
    out.push_back(0xfe);
    PushUint32(out, value & 0xffffffff);
  }
  out.push_back(0xff);
  PushUint64(out, value);
}

static void PushBytesWithSize(bytes_t& out, const bytes_t& b) {
  PushVarInt(out, b.size());
  out.insert(out.end(), b.begin(), b.end());
}

TxIn::TxIn(std::istream& is) {
  ReadIntoBytes(is, prev_txo_hash_, 32);
  prev_txo_index_ = ReadUint32(is);
  uint64_t script_len = ReadVarInt(is);
  ReadIntoBytes(is, script_, script_len);
  sequence_no_ = ReadUint32(is);
  should_serialize_script_ = true;
}

TxIn::TxIn(const std::string& coinbase_message)
  : prev_txo_hash_(32, 0), prev_txo_index_(-1),
    script_(coinbase_message.begin(), coinbase_message.end()),
    sequence_no_(-1), should_serialize_script_(true) {
}

TxIn::TxIn(const Transaction& tx, uint32_t tx_n)
  : prev_txo_hash_(tx.hash()), prev_txo_index_(tx_n),
    script_(tx.outputs()[tx_n].script()),
    sequence_no_(-1), should_serialize_script_(true) {
}

TxIn::TxIn(const bytes_t& hash,
           uint32_t index,
           const bytes_t& script,
           const bytes_t& hash160)
  : prev_txo_hash_(hash), prev_txo_index_(index), script_(script),
    sequence_no_(-1), hash160_(hash160), should_serialize_script_(true) {
}

bytes_t TxIn::Serialize() const {
  bytes_t s(prev_txo_hash_.begin(), prev_txo_hash_.end());
  PushUint32(s, prev_txo_index_);
  if (should_serialize_script_) {
    PushBytesWithSize(s, script_);
  } else {
    PushVarInt(s, 0);
  }
  PushUint32(s, sequence_no_);
  return s;
}

Transaction::Transaction() {
  UpdateHash();
}

Transaction::Transaction(std::istream& is) {
  version_ = ReadUint32(is);
  uint64_t tx_in_count = ReadVarInt(is);
  for (uint64_t i = 0; i < tx_in_count; ++i) {
    inputs_.push_back(TxIn(is));
  }
  uint64_t tx_out_count = ReadVarInt(is);
  for (uint64_t i = 0; i < tx_out_count; ++i) {
    TxOut tx_out(is);
    tx_out.set_tx_output_n(outputs_.size());
    outputs_.push_back(tx_out);
  }
  lock_time_ = ReadUint32(is);
  UpdateHash();
}

void Transaction::Add(const TxIn& tx_in) {
  inputs_.push_back(tx_in);
  UpdateHash();
}

void Transaction::Add(const TxOut& tx_out) {
  TxOut out(tx_out);
  out.set_tx_output_n(outputs_.size());
  outputs_.push_back(out);
  UpdateHash();
}

void Transaction::UpdateHash() {
  bytes_t serialized = Serialize();
  hash_ = Crypto::DoubleSHA256(serialized);
  // TODO(miket): crazy byte order
}

// https://en.bitcoin.it/wiki/Transactions
bytes_t Transaction::Serialize() const {
  bytes_t s;

  // Version 1
  PushUint32(s, 1);

  // Number of inputs
  PushVarInt(s, inputs_.size());
  for (tx_ins_t::const_iterator i = inputs_.begin();
       i != inputs_.end();
       ++i) {
    bytes_t tx_in_bytes = i->Serialize();
    s.insert(s.end(), tx_in_bytes.begin(), tx_in_bytes.end());
  }

  // Number of outputs
  PushVarInt(s, outputs_.size());

  for (tx_outs_t::const_iterator i = outputs_.begin();
       i != outputs_.end();
       ++i) {
    bytes_t tx_out_bytes = i->Serialize();
    s.insert(s.end(), tx_out_bytes.begin(), tx_out_bytes.end());
  }

  // Lock time
  PushUint32(s, 0);

  return s;
}

bool Transaction::IdentifyUnspentTxos(const tx_outs_t& unspent_txos,
                                      uint64_t value,
                                      uint64_t fee,
                                      tx_outs_t& required_txos,
                                      uint64_t& change_value,
                                      int& error_code) {
  uint64_t required_value = value + fee;
  for (tx_outs_t::const_reverse_iterator i = unspent_txos.rbegin();
       i != unspent_txos.rend();
       ++i) {
    if (required_value == 0) {
      break;
    }
    required_txos.push_back(*i);
    if (required_value >= i->value()) {
      required_value -= i->value();
    } else {
      change_value = i->value() - required_value;
      required_value = 0;
    }
  }
  if (required_value != 0) {
    // Not enough funds to cover transaction.
    error_code = -10;
    return false;
  }
  return true;
}

uint64_t Transaction::AddRecipientValues(const tx_outs_t& txos) {
  uint64_t value = 0;
  for (tx_outs_t::const_iterator i = txos.begin(); i != txos.end(); ++i) {
    value += i->value();
  }
  return value;
}

bool Transaction::
GenerateKeysForUnspentTxos(const Node& signing_node,
                           const tx_outs_t& txos,
                           std::map<bytes_t, bytes_t>& signing_keys,
                           std::map<bytes_t, bytes_t>& signing_public_keys,
                           int& error_code) {
  // Create a set of addresses needed to sign the required_txos. Note
  // that an address here is the hash160, because that's the format
  // embedded in the script.
  std::set<bytes_t> signing_addresses;
  for (tx_outs_t::const_reverse_iterator i = txos.rbegin();
       i != txos.rend();
       ++i) {
    signing_addresses.insert(i->GetSigningAddress());
  }

  // Do we have all the keys for the required addresses? Generate
  // them. For now we're going to assume no account has more than 16
  // addresses, so that's the farthest we'll walk down the chain.
  uint32_t start = 0;
  uint32_t count = 16;
  for (uint32_t i = 0; i < count; ++i) {
    std::stringstream node_path;
    node_path << "m/0/" << (start + i);  // external path
    Node* node =
      NodeFactory::DeriveChildNodeWithPath(signing_node, node_path.str());
    bytes_t hash160 = Base58::toHash160(node->public_key());
    if (signing_addresses.find(hash160) !=
        signing_addresses.end()) {
      signing_keys[hash160] = node->secret_key();
      signing_public_keys[hash160] = node->public_key();
    }
    delete node;
    if (signing_keys.size() == signing_addresses.size()) {
      break;
    }
  }
  if (signing_keys.size() != signing_addresses.size()) {
    // We don't have all the keys we need to spend these funds.
    error_code = -11;
    return false;
  }
  return true;
}

bool Transaction::CopyUnspentTxosToTxins(const tx_outs_t& required_txos,
                                         int& error_code) {
  inputs_.clear();
  for (tx_outs_t::const_iterator i = required_txos.begin();
       i != required_txos.end();
       ++i) {
    inputs_.push_back(TxIn(i->tx_hash(), i->tx_output_n(), i->script(),
                           i->GetSigningAddress()));
  }
  error_code = 0;
  return true;
}

bool Transaction::
GenerateScriptSigs(std::map<bytes_t, bytes_t>& signing_keys,
                   std::map<bytes_t, bytes_t>& signing_public_keys,
                   int& error_code) {
  // Loop through each txin and sign individually.
  // https://en.bitcoin.it/w/images/en/7/70/Bitcoin_OpCheckSig_InDetail.png

  // Mark all inputs so they pretend they have no script.
  for (tx_ins_t::iterator i = inputs_.begin();
       i != inputs_.end();
       ++i) {
    i->should_serialize_script(false);
  }

  for (tx_ins_t::iterator i = inputs_.begin();
       i != inputs_.end();
       ++i) {

    // Output just this one tx's script for the transaction.
    i->should_serialize_script(true);

    // Take a snapshot of what we want to sign.
    bytes_t tx_with_one_script_sig = Serialize();

    // Put the script back into hiding.
    i->should_serialize_script(false);

    // SIGHASH_ALL
    PushUint32(tx_with_one_script_sig, 1);

    // Generate the signature.
    const bytes_t& signing_address = i->hash160();
    bytes_t signature;
    Crypto::Sign(signing_keys[signing_address],
                 Crypto::DoubleSHA256(tx_with_one_script_sig),
                 signature);

    // Serialize the signature + public key, then insert it in place
    // of the txo script in the txin.
    bytes_t script_sig_and_key;
    PushVarInt(script_sig_and_key, signature.size() + 1);
    script_sig_and_key.insert(script_sig_and_key.end(),
                              signature.begin(), signature.end());
    script_sig_and_key.push_back(1);  // hash type ??
    PushBytesWithSize(script_sig_and_key,
                      signing_public_keys[signing_address]);
    i->set_script(script_sig_and_key);
  }
  error_code = 0;
  return true;
}

bytes_t Transaction::Sign(const Node& node,
                          const tx_outs_t& unspent_txos,
                          const tx_outs_t& desired_txos,
                          const TxOut& change_address,
                          uint64_t fee,
                          int& error_code) {
  // Determine which unspent_txos we need.
  uint64_t required_value = AddRecipientValues(desired_txos);
  tx_outs_t required_txos;
  uint64_t change_value = 0;
  if (!IdentifyUnspentTxos(unspent_txos,
                           required_value,
                           fee,
                           required_txos,
                           change_value,
                           error_code)) {
    return bytes_t();
  }

  // Generate outputs, adding change address if needed.
  outputs_ = desired_txos;
  if (change_value != 0) {
    TxOut change_address_with_value(change_address);
    change_address_with_value.set_value(change_value);
    outputs_.push_back(change_address_with_value);
  }

  std::map<bytes_t, bytes_t> signing_keys;
  std::map<bytes_t, bytes_t> signing_public_keys;
  if (!GenerateKeysForUnspentTxos(node,
                                  required_txos,
                                  signing_keys,
                                  signing_public_keys,
                                  error_code)) {
    return bytes_t();
  }

  // Convert the unspent txos into unsigned txins.
  if (!CopyUnspentTxosToTxins(required_txos, error_code)) {
    return bytes_t();
  }

  // Replace the txo scripts in the txins with script sigs.
  if (!GenerateScriptSigs(signing_keys, signing_public_keys, error_code)) {
    return bytes_t();
  }

  // Make all the script sigs visible.
  for (tx_ins_t::iterator i = inputs_.begin(); i != inputs_.end(); ++i) {
    i->should_serialize_script(true);
  }

  // And finally serialize once more with all the signatures available.
  return Serialize();
}


TxOut::TxOut(std::istream& is) : tx_output_n_(-1), is_spent_(false) {
  value_ = ReadUint64(is);
  uint64_t script_len = ReadVarInt(is);
  ReadIntoBytes(is, script_, script_len);
}

TxOut::TxOut(uint64_t value, const bytes_t& recipient_hash160)
  : value_(value), tx_output_n_(-1), is_spent_(false) {
  script_.push_back(0x76);  // OP_DUP
  script_.push_back(0xa9);  // OP_HASH160
  script_.push_back(0x14);  // 20 bytes, should probably assert == hashlen
  script_.insert(script_.end(),
                 recipient_hash160.begin(),
                 recipient_hash160.end());
  script_.push_back(0x88);  // OP_EQUALVERIFY
  script_.push_back(0xac);  // OP_CHECKSIG
}

TxOut::TxOut(uint64_t value, const bytes_t& script,
             uint32_t tx_output_n, const bytes_t& tx_hash)
  : value_(value), script_(script), tx_output_n_(tx_output_n),
    is_spent_(false), tx_hash_(tx_hash) {
}

bytes_t TxOut::GetSigningAddress() const {
  if (script_.size() == 25 &&
      script_[0] == 0x76 && script_[1] == 0xa9 && script_[2] == 0x14 &&
      script_[23] == 0x88 && script_[24] == 0xac) {
    // Standard Pay-to-PubkeyHash.
    // https://en.bitcoin.it/wiki/Transactions
    return bytes_t(script_.begin() + 3, script_.begin() + 3 + 20);
  }

  if (script_.size() == 23 &&
      script_[0] == 0xa9 && script_[1] == 0x14 &&
      script_[22] == 0x87) {
    // Standard Pay-to-ScriptHash.
    // https://en.bitcoin.it/wiki/Transactions
    return bytes_t(script_.begin() + 2, script_.begin() + 2 + 20);
  }

  return bytes_t();
}

bytes_t TxOut::Serialize() const {
  bytes_t s;
  PushUint64(s, value_);
  PushBytesWithSize(s, script_);
  return s;
}

void TransactionManager::Add(const Transaction& transaction) {
  // Stick it in the map.
  tx_hashes_to_txs_[transaction.hash()] = transaction;

  // Check every input to see which output it spends, and if we know
  // about that output, mark it spent.
  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_ins_t::const_iterator j = i->second.inputs().begin();
         j != i->second.inputs().end();
         ++j) {
      if (Exists(j->prev_txo_hash())) {
        Transaction& affected_tx = Get(j->prev_txo_hash());
        affected_tx.MarkOutputSpent(j->prev_txo_index());
      }
    }
  }
}

bool TransactionManager::Exists(const bytes_t& hash) {
  return tx_hashes_to_txs_.count(hash) == 1;
}

Transaction& TransactionManager::Get(const bytes_t& hash) {
  return tx_hashes_to_txs_[hash];
}

tx_outs_t TransactionManager::GetUnspentTxos() {
  tx_outs_t unspent_txos;

  for (tx_hashes_to_txs_t::const_iterator i = tx_hashes_to_txs_.begin();
       i != tx_hashes_to_txs_.end();
       ++i) {
    for (tx_outs_t::const_iterator j = i->second.outputs().begin();
         j != i->second.outputs().end();
         ++j) {
      if (!j->is_spent()) {
        unspent_txos.push_back(TxOut(j->value(), j->script(),
                                     j->tx_output_n(), i->first));
      }
    }
  }
  return unspent_txos;
}

uint64_t TransactionManager::GetUnspentValue() {
  tx_outs_t unspent_txos = GetUnspentTxos();
  uint64_t value = 0;
  for (tx_outs_t::const_iterator i = unspent_txos.begin();
       i != unspent_txos.end();
       ++i) {
    value += i->value();
  }
  return value;
}

// http://stackoverflow.com/questions/1661529/is-meyers-implementation-of-singleton-pattern-thread-safe
TransactionManager& TransactionManager::GetSingleton() {
  static TransactionManager tm;
  return tm;
}
