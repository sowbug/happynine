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

#ifdef BUILDING_FOR_TEST
#include "jsoncpp/json/reader.h"
#include "jsoncpp/json/writer.h"
#else
#include "json/reader.h"
#include "json/writer.h"
#endif

#include "base58.h"
#include "crypto.h"
#include "node.h"
#include "node_factory.h"
#include "tx.h"
#include "types.h"

// echo -n "Happynine Copyright 2014 Mike Tsao." | sha256sum
const std::string PASSPHRASE_CHECK_HEX =
  "df3bc110ce022d64a20503502a9edfd8acda8a39868e5dff6601c0bb9b6f9cf9";

API::API()
  : credentials_(),
    wallet_(credentials_) {
    }

bool API::HandleSetPassphrase(const Json::Value& args, Json::Value& result) {
  const std::string new_passphrase = args["new_passphrase"].asString();
  bytes_t salt, check, encrypted_ephemeral_key;
  if (credentials_.SetPassphrase(new_passphrase,
                                 salt,
                                 check,
                                 encrypted_ephemeral_key)) {
    result["salt"] = to_hex(salt);
    result["check"] = to_hex(check);
    result["ekey_enc"] = to_hex(encrypted_ephemeral_key);
  } else {
    SetError(result, -1, "set-passphrase failed");
  }
  return true;
}

bool API::HandleSetCredentials(const Json::Value& args, Json::Value& result) {
  const bytes_t salt = unhexlify(args["salt"].asString());
  const bytes_t check = unhexlify(args["check"].asString());
  const bytes_t encrypted_ephemeral_key =
    unhexlify(args["ekey_enc"].asString());
  if (salt.size() >= 32 &&
      check.size() >= 32 &&
      encrypted_ephemeral_key.size() >= 32) {
    credentials_.Load(salt,
                      check,
                      encrypted_ephemeral_key);
    result["success"] = true;
  } else {
    SetError(result, -1, "missing valid salt/check/ekey_enc params");
  }
  return true;
}

bool API::HandleLock(const Json::Value& /*args*/, Json::Value& result) {
  result["success"] = credentials_.Lock();
  return true;
}

bool API::HandleUnlock(const Json::Value& args, Json::Value& result) {
  const std::string passphrase = args["passphrase"].asString();
  if (passphrase.size() != 0) {
    result["success"] = credentials_.Unlock(passphrase);
  } else {
    SetError(result, -1, "missing valid passphrase param");
  }
  return true;
}

void API::GenerateNodeResponse(Json::Value& dict, const Node* node,
                               const bytes_t& ext_prv_enc,
                               bool include_prv) {
  dict["fp"] = "0x" + to_fingerprint(node->fingerprint());
  dict["pfp"] = "0x" + to_fingerprint(node->parent_fingerprint());
  dict["ext_pub_b58"] = Base58::toBase58Check(node->toSerializedPublic());
  dict["ext_prv_enc"] = to_hex(ext_prv_enc);
  if (node->is_private() && include_prv) {
    dict["ext_prv_b58"] = Base58::toBase58Check(node->toSerialized());
  }
}

bool API::HandleGenerateRootNode(const Json::Value& args,
                                 Json::Value& result) {
  if (credentials_.isLocked()) {
    SetError(result, -1, "locked");
    return true;
  }

  const bytes_t extra_seed_bytes(unhexlify(args.get("extra_seed_hex",
                                                    "55").asString()));

  bytes_t ext_prv_enc;
  bytes_t seed_bytes;
  if (wallet_.GenerateRootNode(extra_seed_bytes, ext_prv_enc, seed_bytes)) {
    GenerateNodeResponse(result, wallet_.GetRootNode(), ext_prv_enc, true);
    result["seed_hex"] = to_hex(seed_bytes);
  } else {
    SetError(result, -1, "Root node generation failed");
  }
  return true;
}

bool API::HandleAddRootNode(const Json::Value& args,
                            Json::Value& result) {
  const std::string ext_pub_b58(args["ext_pub_b58"].asString());
  const bytes_t ext_prv_enc(unhexlify(args["ext_prv_enc"].asString()));

  if (!ext_pub_b58.empty() && !ext_prv_enc.empty()) {
    if (wallet_.SetRootNode(ext_pub_b58, ext_prv_enc)) {
      GenerateNodeResponse(result, wallet_.GetRootNode(), ext_prv_enc, false);
    } else {
      SetError(result, -1, "Extended key failed validation");
    }
  } else {
    SetError(result, -1, "Missing required ext_pub_b58 & ext_prv_enc params");
  }
  return true;
}

bool API::HandleImportRootNode(const Json::Value& args,
                               Json::Value& result) {
  if (credentials_.isLocked()) {
    SetError(result, -1, "locked");
    return true;
  }

  if (args.isMember("ext_prv_b58")) {
    const std::string ext_prv_b58(args["ext_prv_b58"].asString());
    bytes_t ext_prv_enc;
    if (wallet_.ImportRootNode(ext_prv_b58, ext_prv_enc)) {
      GenerateNodeResponse(result, wallet_.GetRootNode(), ext_prv_enc, false);
    } else {
      SetError(result, -1, "Extended key failed validation");
    }
  } else {
    SetError(result, -1, "Missing required ext_prv_b58 param");
  }
  return true;
}

void API::PopulateAddressStatuses(Json::Value& json_value) {
  Wallet::address_statuses_t addresses;
  wallet_.GetAddressStatusesToReport(addresses);
  for (Wallet::address_statuses_t::const_iterator i = addresses.begin();
       i != addresses.end();
       ++i) {
    json_value.append(*i);
  }
}

bool API::HandleDeriveChildNode(const Json::Value& args,
                                Json::Value& result) {
  if (credentials_.isLocked()) {
    SetError(result, -1, "locked");
    return true;
  }

  if (!wallet_.hasRootNode()) {
    SetError(result, -1, "Wallet is missing root node");
    return true;
  }

  const std::string path(args["path"].asString());
  const bool isWatchOnly(args["isWatchOnly"].asBool());

  Node *node = NULL;
  bytes_t ext_prv_enc;
  if (wallet_.DeriveChildNode(path, isWatchOnly, &node, ext_prv_enc)) {
    GenerateNodeResponse(result, node, ext_prv_enc, false);
    PopulateAddressStatuses(result["addresses"]);
    result["path"] = path;
    delete node;  // TODO(miket): implement AddChildNode & move addr gen code
  } else {
    std::stringstream s;
    s << "Failed to derive child node @ "<< path << " from " <<
      std::hex << wallet_.GetRootNode()->fingerprint();
    SetError(result, -1, s.str());
  }

  return true;
}

bool API::HandleAddChildNode(const Json::Value& /*args*/,
                             Json::Value& /*result*/) {
  return false;
}

void API::PopulateDictionaryFromNode(Json::Value& dict, Node* node) {
  dict["hex_id"] = to_hex(node->hex_id());
  dict["fingerprint"] = "0x" + to_fingerprint(node->fingerprint());
  dict["parent_fingerprint"] =
    "0x" + to_fingerprint(node->parent_fingerprint());
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

Node* API::CreateNodeFromArgs(const Json::Value& args) {
  // TODO(miket): https://github.com/sowbug/happynine/issues/29
  const std::string seed_hex = args.get("seed_hex", "").asString();
  if (seed_hex.size() != 0) {
    const bytes_t seed_bytes(unhexlify(seed_hex));
    return NodeFactory::CreateNodeFromSeed(seed_bytes);
  }
  bytes_t ext_bytes;
  const std::string ext_hex = args.get("ext_hex", "").asString();
  if (ext_hex.size() != 0) {
    ext_bytes = unhexlify(ext_hex);
  } else {
    const std::string ext_b58 = args.get("ext_b58", "").asString();
    if (ext_b58.size() != 0) {
      ext_bytes = Base58::fromBase58Check(ext_b58);
    }
  }
  if (ext_bytes.size() != 0) {
    return NodeFactory::CreateNodeFromExtended(ext_bytes);
  }
  return NULL;
}

bool API::HandleGetNode(const Json::Value& args, Json::Value& result) {
  Node* parent_node = CreateNodeFromArgs(args);
  if (!parent_node) {
    SetError(result,
             -1,
             "Missing valid 'seed', 'ext_hex', 'ext_b58' params.");
    return true;
  }

  const std::string node_path = args.get("path", "m").asString();
  Node* node =
    NodeFactory::DeriveChildNodeWithPath(*parent_node, node_path);
  delete parent_node;

  PopulateDictionaryFromNode(result, node);
  result["path"] = node_path;
  delete node;

  return true;
}

void API::GetAddresses(const Node& parent_node,
                       uint32_t start,
                       uint32_t count,
                       const std::string& base_node_path,
                       Json::Value& result) {
  for (uint32_t i = 0; i < count; ++i) {
    std::stringstream node_path;
    node_path << base_node_path << "/" << (start + i);
    Node* node =
      NodeFactory::DeriveChildNodeWithPath(parent_node, node_path.str());
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
}

void API::GetHash160s(const Node& parent_node,
                      uint32_t start,
                      uint32_t count,
                      const std::string& base_node_path,
                      std::set<bytes_t>& result) {
  for (uint32_t i = 0; i < count; ++i) {
    std::stringstream node_path;
    node_path << base_node_path << "/" << (start + i);
    Node* node =
      NodeFactory::DeriveChildNodeWithPath(parent_node, node_path.str());
    result.insert(Base58::toHash160(node->public_key()));
    delete node;
  }
}

bool API::HandleGetAddresses(const Json::Value& args,
                             Json::Value& result) {
  Node* parent_node = CreateNodeFromArgs(args);
  if (!parent_node) {
    SetError(result,
             -1,
             "Missing valid 'seed', 'ext_hex', 'ext_b58' params.");
    return true;
  }
  GetAddresses(*parent_node,
               args.get("start", 0).asUInt(),
               args.get("count", 20).asUInt(),
               args.get("path", "m").asString(),
               result);
  delete parent_node;

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
    SetError(result, -1, "Encryption failed");
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
    SetError(result, -1, "Decryption failed");
  }
  return true;
}

bool API::HandleGetSignedTransaction(const Json::Value& args,
                                     Json::Value& result) {
  const std::string ext_prv_b58 = args.get("ext_prv_b58", "").asString();

  Node *sending_node =
    NodeFactory::CreateNodeFromExtended(Base58::
                                        fromBase58Check(ext_prv_b58));
  if (sending_node == NULL) {
    SetError(result, -2, "No node derived from extended key");
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
    SetError(result, error_code, "Signing failed");
  } else {
    result["signed_tx"] = to_hex(signed_tx);
  }
  delete sending_node;
  return true;
}

bool API::HandleGetUnspentTxos(const Json::Value& args,
                               Json::Value& result) {
  int error_code = 0;
  TransactionManager& tm = TransactionManager::GetSingleton();

  tx_outs_t unspent_txos = tm.GetUnspentTxos();
  if (error_code != 0) {
    SetError(result, error_code, "Getting unspent txos failed");
    return true;
  }

  const std::string ext_b58 = args.get("ext_b58", "").asString();
  Node *sending_node =
    NodeFactory::CreateNodeFromExtended(Base58::fromBase58Check(ext_b58));

  std::set<bytes_t> addresses;
  if (sending_node != NULL) {
    GetHash160s(*sending_node,
                args.get("start", 0).asUInt(),
                args.get("pub_n", 8).asUInt(),
                args.get("path", "m/0").asString(),
                addresses);
    GetHash160s(*sending_node,
                args.get("start", 0).asUInt(),
                args.get("change_n", 8).asUInt(),
                args.get("path", "m/1").asString(),
                addresses);
    delete sending_node;
  }

  result["unspent_txos"].resize(0);
  for (tx_outs_t::const_iterator i = unspent_txos.begin();
       i != unspent_txos.end();
       ++i) {
    if (addresses.size() == 0 ||
        addresses.count(i->GetSigningAddress()) != 0) {
      Json::Value utxo;
      utxo["script"] = to_hex(i->script());
      utxo["tx_hash"] = to_hex_reversed(i->tx_hash());
      utxo["tx_output_n"] = i->tx_output_n();
      // TODO(miket): today it's just one address/utxo.
      utxo["addr_b58"] = Base58::hash160toAddress(i->GetSigningAddress());
      utxo["value"] = (Json::UInt64)i->value();
      result["unspent_txos"].append(utxo);
    }
  }

  return true;
}

bool API::HandleReportAddressHistory(const Json::Value& args,
                                     Json::Value& result) {
  int error_code = 0;
  std::string error_message;
  TransactionManager& tm = TransactionManager::GetSingleton();

  result["unknown_txs"].resize(0);
  if (args.isMember("history")) {
    for (Json::Value::const_iterator i = args["history"].begin();
         i != args["history"].end();
         ++i) {
      const std::string hash((*i)["tx_hash"].asString());
      if (!tm.Exists(unhexlify(hash))) {
        result["unknown_txs"].append(hash);
      }
    }
  } else {
    error_code = -1;
    error_message = "Missing 'history' param";
  }
  if (error_code != 0) {
    SetError(result, error_code, error_message);
  }
  return true;
}

bool API::HandleReportTransactions(const Json::Value& args,
                                   Json::Value& result) {
  int error_code = 0;
  std::string error_message;

  TransactionManager& tm = TransactionManager::GetSingleton();

  if (args.isMember("txs")) {
    for (Json::Value::const_iterator i = args["txs"].begin();
         i != args["txs"].end();
         ++i) {
      const bytes_t tx_bytes(unhexlify((*i).asString()));
      std::istringstream is;
      const char* p = reinterpret_cast<const char*>(&tx_bytes[0]);
      is.rdbuf()->pubsetbuf(const_cast<char*>(p),
                            tx_bytes.size());
      Transaction tx(is);
      tm.Add(tx);
    }
  } else {
    error_code = -1;
    error_message = "Missing 'txs' param";
  }

  if (error_code != 0) {
    SetError(result, error_code, error_message);
  }
  return true;
}

void API::GetError(const Json::Value& obj, int& code, std::string& message) {
  if (!obj.isMember("error")) {
    code = 0;
    message = "No error";
  } else {
    // -99999 = poorly constructed error
    code = obj["error"].get("code", -99999).asInt();
    message = obj["error"].get("message", "Missing error message").asString();
  }
}

int API::GetErrorCode(const Json::Value& obj) {
  int code;
  std::string message;
  GetError(obj, code, message);
  return code;
}

bool API::DidResponseSucceed(const Json::Value& obj) {
  return GetErrorCode(obj) == 0;
}

void API::SetError(Json::Value& obj, int code, const std::string& message) {
  obj["error"]["code"] = code;
  if (message.size() > 0) {
    obj["error"]["message"] = message;
  } else {
    obj["error"]["message"] = "Unspecified error";
  }
}
