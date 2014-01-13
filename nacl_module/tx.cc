#include "tx.h"

#include <iostream>  // cerr
#include <iterator>
#include <set>
#include <sstream>
#include <vector>

#include "base58.h"
#include "crypto.h"
#include "node.h"
#include "node_factory.h"
#include "openssl/sha.h"

bytes_t UnspentTxo::GetSigningAddress() const {
  if (script.size() == 25 &&
      script[0] == 0x76 && script[1] == 0xa9 && script[2] == 0x14 &&
      script[23] == 0x88 && script[24] == 0xac) {
    // Standard Pay-to-PubkeyHash.
    // https://en.bitcoin.it/wiki/Transactions
    return bytes_t(script.begin() + 3, script.begin() + 3 + 20);
  }

  if (script.size() == 23 &&
      script[0] == 0xa9 && script[1] == 0x14 &&
      script[22] == 0x87) {
    // Standard Pay-to-ScriptHash.
    // https://en.bitcoin.it/wiki/Transactions
    return bytes_t(script.begin() + 2, script.begin() + 2 + 20);
  }

  return bytes_t();
}

TxOut::TxOut(const bytes_t& hash_, uint64_t value_) :
  hash(hash_), value(value_) {}

Tx::Tx(const Node& sending_node,
       const unspent_txos_t unspent_txos,
       const bytes_t recipient_hash160,
       uint64_t value,
       uint64_t fee,
       uint32_t change_index) :
  sending_node_(sending_node),
  unspent_txos_(unspent_txos),
  recipient_hash160_(recipient_hash160),
  value_(value),
  fee_(fee),
  change_index_(change_index),
  change_value_(0) {
}

Tx::~Tx() {
}

bool Tx::CreateSignedTransaction(bytes_t& signed_tx, int& error_code) {
  required_unspent_txos_.clear();
  change_value_ = 0;

  // Identify enough unspent_txos to cover transaction value.
  uint64_t required_value = fee_ + value_;
  for (unspent_txos_t::const_reverse_iterator i = unspent_txos_.rbegin();
       i != unspent_txos_.rend();
       ++i) {
    if (required_value == 0) {
      break;
    }
    required_unspent_txos_.push_back(*i);
    if (required_value >= i->value) {
      required_value -= i->value;
    } else {
      change_value_ = i->value - required_value;
      required_value = 0;
    }
  }
  if (required_value != 0) {
    // Not enough funds to cover transaction.
    std::cerr << "Not enough funds" << std::endl;
    error_code = -10;
    return false;
  }

  // We know which unspent_txos we intend to use. Create a set of
  // required addresses. Note that an address here is the hash160,
  // because that's the format embedded in the script.
  std::set<bytes_t> required_signing_addresses;
  for (unspent_txos_t::reverse_iterator i = required_unspent_txos_.rbegin();
       i != required_unspent_txos_.rend();
       ++i) {
    required_signing_addresses.insert(i->GetSigningAddress());
  }

  // Do we have all the keys for the required addresses? Generate
  // them. For now we're going to assume no account has more than 16
  // addresses, so that's the farthest we'll walk down the chain.
  uint32_t start = 0;
  uint32_t count = 16;
  signing_addresses_to_keys_.clear();
  for (uint32_t i = 0; i < count; ++i) {
    std::stringstream node_path;
    node_path << "m/0/" << (start + i);
    Node* node =
      NodeFactory::DeriveChildNodeWithPath(sending_node_, node_path.str());
    bytes_t hash160 = Base58::toHash160(node->public_key());
    if (required_signing_addresses.find(hash160) !=
        required_signing_addresses.end()) {
      signing_addresses_to_keys_[hash160] = node->secret_key();
      signing_addresses_to_public_keys_[hash160] = node->public_key();
    }
    delete node;
    if (signing_addresses_to_keys_.size() ==
        required_signing_addresses.size()) {
      break;
    }
  }
  if (signing_addresses_to_keys_.size() !=
      required_signing_addresses.size()) {
    // We don't have all the keys we need to spend these funds.
    std::cerr << "missing some keys" << std::endl;
    error_code = -11;
    return false;
  }

  recipients_.clear();
  TxOut txout(recipient_hash160_, value_);
  recipients_.push_back(txout);

  if (change_value_ != 0) {
    // Derive the change address
    std::stringstream node_path;
    node_path << "m/0/" << (change_index_);
    Node* node =
      NodeFactory::DeriveChildNodeWithPath(sending_node_, node_path.str());
    bytes_t hash160 = Base58::toHash160(node->public_key());
    recipients_.push_back(TxOut(hash160, change_value_));
    delete node;
  }

  // Now loop through and sign each txin individually.
  // https://en.bitcoin.it/w/images/en/7/70/Bitcoin_OpCheckSig_InDetail.png
  for (unspent_txos_t::iterator i = required_unspent_txos_.begin();
       i != required_unspent_txos_.end();
       ++i) {
    bytes_t script_sig;

    // TODO(miket): this is actually supposed to be the serialization
    // of the previous txo's script. We're just assuming it's a
    // standard P2PH and reconstructing what it would have been.
    script_sig.push_back(0x76);  // OP_DUP
    script_sig.push_back(0xa9);  // OP_HASH160
    script_sig.push_back(0x14);  // 20 bytes, should probably assert == hashlen
    bytes_t signing_address = i->GetSigningAddress();
    script_sig.insert(script_sig.end(),
                      signing_address.begin(),
                      signing_address.end());
    script_sig.push_back(0x88);  // OP_EQUALVERIFY
    script_sig.push_back(0xac);  // OP_CHECKSIG

    i->script_sig = script_sig;
    bytes_t tx_with_one_script_sig = SerializeTransaction();

    // SIGHASH_ALL
    tx_with_one_script_sig.push_back(1);
    tx_with_one_script_sig.push_back(0);
    tx_with_one_script_sig.push_back(0);
    tx_with_one_script_sig.push_back(0);

    bytes_t digest;
    digest.resize(SHA256_DIGEST_LENGTH);

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256,
                  &tx_with_one_script_sig[0],
                  tx_with_one_script_sig.size());
    SHA256_Final(&digest[0], &sha256);
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, &digest[0], digest.capacity());
    SHA256_Final(&digest[0], &sha256);

    bytes_t signature;
    Crypto::Sign(signing_addresses_to_keys_[signing_address],
                 digest,
                 signature);
    bytes_t script_sig_and_key;
    script_sig_and_key.push_back((signature.size() + 1) & 0xff);
    script_sig_and_key.insert(script_sig_and_key.end(),
                              signature.begin(), signature.end());
    script_sig_and_key.push_back(1);  // hash type ??
    bytes_t public_key(signing_addresses_to_public_keys_
                       [i->GetSigningAddress()]);
    script_sig_and_key.push_back(public_key.size() & 0xff);
    script_sig_and_key.insert(script_sig_and_key.end(),
                      public_key.begin(),
                      public_key.end());
    script_sigs_.push_back(script_sig_and_key);

    // And clear the sig again for the next one.
    i->script_sig.clear();
  }

  // We now have all the signatures. Stick them in.
  unspent_txos_t::iterator i = required_unspent_txos_.begin();
  std::vector<bytes_t>::const_iterator j = script_sigs_.begin();
  for (;
       i != required_unspent_txos_.end();
       ++i, ++j) {
    i->script_sig = *j;
  }

  // And finally serialize once more with all the signatures installed.
  signed_tx = SerializeTransaction();

  // https://blockchain.info/decode-tx
  //std::cerr << to_hex(signed_tx) << std::endl;

  return true;
}

bytes_t Tx::SerializeTransaction() {
  bytes_t signed_tx;

  signed_tx.resize(0);

  // https://en.bitcoin.it/wiki/Transactions

  // Version 1
  signed_tx.push_back(1);
  signed_tx.push_back(0);
  signed_tx.push_back(0);
  signed_tx.push_back(0);

  // Number of inputs
  if (required_unspent_txos_.size() >= 0xfd) {
    // TODO: implement varint
    std::cerr << "too many inputs" << std::endl;
    return bytes_t();
  }
  signed_tx.push_back(required_unspent_txos_.size() & 0xff);

  // txin
  for (unspent_txos_t::const_iterator i = required_unspent_txos_.begin();
       i != required_unspent_txos_.end();
       ++i) {
    // For some stupid reason the hash is serialized backwards.
    // Fortunately the inputs we're getting are, too.
    for (bytes_t::const_iterator j = i->hash.begin();
         j != i->hash.end();
         ++j) {
      signed_tx.push_back(*j);
    }

    // Previous TXO index
    signed_tx.push_back((i->output_n) & 0xff);
    signed_tx.push_back((i->output_n >> 8) & 0xff);
    signed_tx.push_back((i->output_n >> 16) & 0xff);
    signed_tx.push_back((i->output_n >> 24) & 0xff);

    // TODO ScriptSig
    // https://en.bitcoin.it/wiki/OP_CHECKSIG
    if (i->script_sig.size() > 0xfd) {
      // TODO: implement varint
      std::cerr << "script_sig too big" << std::endl;
      return bytes_t();
    }
    signed_tx.push_back(i->script_sig.size() & 0xff);
    signed_tx.insert(signed_tx.end(),
                     i->script_sig.begin(),
                     i->script_sig.end());

    // sequence_no
    signed_tx.push_back(0xff);
    signed_tx.push_back(0xff);
    signed_tx.push_back(0xff);
    signed_tx.push_back(0xff);
  }

  // Number of outputs
  if (recipients_.size() >= 0xfd) {
    // TODO: implement varint
    std::cerr << "too many recipients" << std::endl;
    return bytes_t();
  }
  signed_tx.push_back(recipients_.size() & 0xff);

  for (tx_outs_t::iterator i = recipients_.begin();
       i != recipients_.end();
       ++i) {
    uint64_t recipient_value = i->value;
    signed_tx.push_back((recipient_value) & 0xff);
    signed_tx.push_back((recipient_value >>  8) & 0xff);
    signed_tx.push_back((recipient_value >> 16) & 0xff);
    signed_tx.push_back((recipient_value >> 24) & 0xff);
    signed_tx.push_back((recipient_value >> 32) & 0xff);
    signed_tx.push_back((recipient_value >> 40) & 0xff);
    signed_tx.push_back((recipient_value >> 48) & 0xff);
    signed_tx.push_back((recipient_value >> 56) & 0xff);

    bytes_t script;
    script.push_back(0x76);  // OP_DUP
    script.push_back(0xa9);  // OP_HASH160
    script.push_back(0x14);  // 20 bytes, should probably assert == hashlen
    script.insert(script.end(), i->hash.begin(), i->hash.end());
    script.push_back(0x88);  // OP_EQUALVERIFY
    script.push_back(0xac);  // OP_CHECKSIG

    signed_tx.push_back(script.size() & 0xff);
    signed_tx.insert(signed_tx.end(), script.begin(), script.end());
  }

  // Lock time
  signed_tx.push_back(0);
  signed_tx.push_back(0);
  signed_tx.push_back(0);
  signed_tx.push_back(0);

  //  std::cerr << to_hex(signed_tx) << std::endl;

  return signed_tx;
}
