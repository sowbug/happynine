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

#include "api.h"

#include <iomanip>
#include <sstream>
#include <stdint.h>

#include "base58.h"
#include "crypto.h"
#ifdef BUILDING_FOR_TEST
#include "jsoncpp/json/reader.h"
#include "jsoncpp/json/writer.h"
#else
#include "json/reader.h"
#include "json/writer.h"
#endif
#include "node.h"
#include "node_factory.h"
#include "tx.h"
#include "types.h"

// echo -n "Happynine Copyright 2014 Mike Tsao." | sha256sum
const std::string PASSPHRASE_CHECK_HEX =
  "df3bc110ce022d64a20503502a9edfd8acda8a39868e5dff6601c0bb9b6f9cf9";

void API::PopulateDictionaryFromNode(Json::Value& dict, Node* node) {
  dict["hex_id"] = to_hex(node->hex_id());
  dict["fingerprint"] = "0x" + to_fingerprint(node->fingerprint());
  dict["address"] = Base58::toAddress(node->public_key());
  dict["public_key"] = to_hex(node->public_key());
  dict["chain_code"] = to_hex(node->chain_code());
  dict["ext_pub_hex"] = to_hex(node->toSerializedPublic());
  dict["ext_pub_b58"] = Base58::toBase58Check(node->toSerializedPublic());
  if (node->is_private()) {
    dict["secret_key"] = to_hex(node->secret_key());
    dict["secret_wif"] = Base58::toPrivateKey(node->secret_key());
    dict["ext_prv_hex"] = to_hex(node->toSerialized());
    dict["ext_prv_b58"] = Base58::toBase58Check(node->toSerialized());
  }
}

bool API::HandleCreateNode(const Json::Value& args,
                           Json::Value& result) {
  bytes_t seed_bytes(32, 0);
  const bytes_t
    supplied_seed_bytes(unhexlify(args.get("seed", "00").asString()));

  if (!Crypto::GetRandomBytes(seed_bytes)) {
    result["error_code"] = -1;
    result["error_message"] =
      std::string("The PRNG has not been seeded with enough "
                  "randomness to ensure an unpredictable byte sequence.");
    return true;
  }
  seed_bytes.insert(seed_bytes.end(),
                    supplied_seed_bytes.begin(),
                    supplied_seed_bytes.end());

  Node *node = NodeFactory::CreateNodeFromSeed(seed_bytes);
  PopulateDictionaryFromNode(result, node);
  delete node;
  return true;
}

bool API::HandleGetNode(const Json::Value& args, Json::Value& result) {
  const std::string seed = args.get("seed", "").asString();
  const bytes_t seed_bytes(unhexlify(seed));

  Node *parent_node = NULL;
  if (seed_bytes.size() == 78) {
    parent_node = NodeFactory::CreateNodeFromExtended(seed_bytes);
  } else if (seed[0] == 'x') {
    parent_node =
      NodeFactory::CreateNodeFromExtended(Base58::fromBase58Check(seed));
  } else {
    parent_node = NodeFactory::CreateNodeFromSeed(seed_bytes);
  }

  const std::string node_path = args.get("path", "m").asString();
  Node* node =
    NodeFactory::DeriveChildNodeWithPath(*parent_node, node_path);
  delete parent_node;

  PopulateDictionaryFromNode(result, node);
  delete node;

  return true;
}

bool API::HandleGetAddresses(const Json::Value& args,
                             Json::Value& result) {
  const std::string seed = args.get("seed", "").asString();
  const bytes_t seed_bytes(unhexlify(seed));

  Node *parent_node = NULL;
  if (seed_bytes.size() == 78) {
    parent_node = NodeFactory::CreateNodeFromExtended(seed_bytes);
  } else if (seed[0] == 'x') {
    parent_node =
      NodeFactory::CreateNodeFromExtended(Base58::fromBase58Check(seed));
  } else {
    parent_node = NodeFactory::CreateNodeFromSeed(seed_bytes);
  }

  uint32_t start = args.get("start", 0).asUInt();
  uint32_t count = args.get("count", 20).asUInt();
  const std::string base_node_path = args.get("path", "m").asString();
  for (uint32_t i = 0; i < count; ++i) {
    std::stringstream node_path;
    node_path << base_node_path << "/" << (start + i);
    Node* node =
      NodeFactory::DeriveChildNodeWithPath(*parent_node, node_path.str());
    result["addresses"][i]["index"] = i + start;
    result["addresses"][i]["path"] = node_path.str();
    result["addresses"][i]["address"] =
      Base58::toAddress(node->public_key());
    if (node->secret_key().size() != 0) {
      result["addresses"][i]["key"] =
        Base58::toPrivateKey(node->secret_key());
    }
    delete node;
  }
  delete parent_node;

  return true;
}

bool API::VerifyCredentials(const bytes_t& key,
                            const bytes_t& check,
                            const bytes_t& internal_key_encrypted,
                            bytes_t& internal_key,
                            int& error_code,
                            std::string& error_message) {
  bytes_t check_decrypted;
  if (!Crypto::Decrypt(key, check, check_decrypted)) {
    error_code = -2;
    error_message = "Check decryption failed";
    return false;
  }
  if (check_decrypted != unhexlify(PASSPHRASE_CHECK_HEX)) {
    error_code = -3;
    error_message = "Check verification failed";
    return false;
  }
  if (!Crypto::Decrypt(key, internal_key_encrypted, internal_key)) {
    error_code = -4;
    error_message = "internal_key decryption failed";
    return false;
  }
  return true;
}

bool API::HandleSetPassphrase(const Json::Value& args,
                              Json::Value& result) {
  bytes_t key(unhexlify(args.get("key", "").asString()));
  bytes_t check(unhexlify(args.get("check", "").asString()));
  bytes_t
    internal_key_encrypted(unhexlify(args.get("internal_key_encrypted", "")
                                     .asString()));
  const std::string new_passphrase = args["new_passphrase"].asString();

  bytes_t internal_key;
  if (key.size() > 0 &&
      check.size() > 0 &&
      internal_key_encrypted.size() > 0) {
    int error_code;
    std::string error_message;
    if (!VerifyCredentials(key,
                           check,
                           internal_key_encrypted,
                           internal_key,
                           error_code,
                           error_message)) {
      result["error_code"] = error_code;
      result["error_message"] = error_message;
      return true;
    }
  } else {
    internal_key.resize(32);
    Crypto::GetRandomBytes(internal_key);
  }
  key = bytes_t(32, 0);
  check = bytes_t();

  bytes_t salt(32);
  Crypto::GetRandomBytes(salt);

  if (!Crypto::DeriveKey(new_passphrase, salt, key)) {
    result["error_code"] = -1;
    result["error_message"] = "Key derivation failed";
    return true;
  }
  if (!Crypto::Encrypt(key, unhexlify(PASSPHRASE_CHECK_HEX), check)) {
    result["error_code"] = -5;
    result["error_message"] = "Check generation failed";
    return true;
  }
  if (!Crypto::Encrypt(key, internal_key, internal_key_encrypted)) {
    result["error_code"] = -5;
    result["error_message"] = "Check generation failed";
    return true;
  }
  result["salt"] = to_hex(salt);
  result["key"] = to_hex(key);
  result["check"] = to_hex(check);
  result["internal_key"] = to_hex(internal_key);
  result["internal_key_encrypted"] = to_hex(internal_key_encrypted);
  return true;
}

bool API::HandleUnlockWallet(const Json::Value& args,
                             Json::Value& result) {
  const bytes_t salt(unhexlify(args["salt"].asString()));
  const bytes_t check(unhexlify(args["check"].asString()));
  const std::string passphrase = args["passphrase"].asString();
  const bytes_t
    internal_key_encrypted(unhexlify(args["internal_key_encrypted"]
                                     .asString()));

  bytes_t key(32, 0);
  if (!Crypto::DeriveKey(passphrase, salt, key)) {
    result["error_code"] = -1;
    result["error_message"] = "Key derivation failed";
    return true;
  }
  bytes_t internal_key;
  int error_code;
  std::string error_message;
  if (!VerifyCredentials(key,
                         check,
                         internal_key_encrypted,
                         internal_key,
                         error_code,
                         error_message)) {
    result["error_code"] = error_code;
    result["error_message"] = error_message;
    return true;
  }
  result["key"] = to_hex(key);
  result["internal_key"] = to_hex(internal_key);
  return true;
}

bool API::HandleEncryptItem(const Json::Value& args,
                            Json::Value& result) {
  bytes_t internal_key(unhexlify(args["internal_key"].asString()));
  const std::string item = args["item"].asString();
  bytes_t item_encrypted;
  bytes_t item_bytes(&item[0], &item[item.size()]);
  if (Crypto::Encrypt(internal_key, item_bytes, item_encrypted)) {
    result["item_encrypted"] = to_hex(item_encrypted);
  } else {
    result["error_code"] = -1;
  }
  return true;
}

bool API::HandleDecryptItem(const Json::Value& args,
                            Json::Value& result) {
  bytes_t internal_key(unhexlify(args["internal_key"].asString()));
  bytes_t item_encrypted(unhexlify(args["item_encrypted"].asString()));
  bytes_t item_bytes;
  if (Crypto::Decrypt(internal_key, item_encrypted, item_bytes)) {
    result["item"] =
      std::string(reinterpret_cast<char const*>(&item_bytes[0]),
                  item_bytes.size());
  } else {
    result["error_code"] = -1;
  }
  return true;
}

bool API::HandleGetSignedTransaction(const Json::Value& args,
                                     Json::Value& result) {
  const std::string ext_prv_b58 = args.get("ext_prv_b58", "").asString();

  Node *sending_node = NULL;
  if (ext_prv_b58[0] == 'x') {
    sending_node =
      NodeFactory::CreateNodeFromExtended(Base58::
                                          fromBase58Check(ext_prv_b58));
  }
  if (sending_node == NULL) {
    result["error_code"] = -2;
    return true;
  }

  tx_outs_t unspent_txos;
  for (unsigned int i = 0; i < args["unspent_txos"].size(); ++i) {
    Json::Value jtxo = args["unspent_txos"][i];
    TxOut utxo(jtxo["value"].asUInt64(),
               unhexlify(jtxo["script"].asString()),
               jtxo["tx_output_n"].asUInt(),
               unhexlify(jtxo["tx_hash"].asString()));
    unspent_txos.push_back(utxo);
  }

  tx_outs_t recipient_txos;
  for (unsigned int i = 0; i < args["recipients"].size(); ++i) {
    Json::Value recipient = args["recipients"][i];
    const std::string address(recipient["address"].asString());
    const bytes_t address_bytes_with_version(Base58::fromBase58Check(address));
    const bytes_t recipient_address(address_bytes_with_version.begin() + 1,
                                    address_bytes_with_version.end());
    uint64_t value = recipient["value"].asUInt64();
    TxOut recipient_txo(value, recipient_address);
    recipient_txos.push_back(recipient_txo);
  }

  uint64_t fee = args["fee"].asUInt64();
  uint32_t change_index = args["change_index"].asUInt();

  std::stringstream node_path;
  node_path << "m/0/" << change_index;
  Node* change_node =
    NodeFactory::DeriveChildNodeWithPath(*sending_node, node_path.str());
  TxOut change_txo(0, Base58::toHash160(change_node->public_key()));
  delete change_node;

  Transaction transaction;
  int error_code = 0;
  bytes_t signed_tx = transaction.Sign(*sending_node,
                                       unspent_txos,
                                       recipient_txos,
                                       change_txo,
                                       fee,
                                       error_code);
  if (error_code != 0) {
    result["error_code"] = error_code;
  } else {
    result["signed_tx"] = to_hex(signed_tx);
  }
  delete sending_node;
  return true;
}
