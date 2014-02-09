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
#include "blockchain.h"
#include "credentials.h"
#include "crypto.h"
#include "errors.h"
#include "node.h"
#include "node_factory.h"
#include "tx.h"
#include "types.h"
#include "wallet.h"

// echo -n "Happynine Copyright 2014 Mike Tsao." | sha256sum
const std::string PASSPHRASE_CHECK_HEX =
  "df3bc110ce022d64a20503502a9edfd8acda8a39868e5dff6601c0bb9b6f9cf9";

API::API(Blockchain* blockchain, Credentials* credentials)
  : blockchain_(blockchain), credentials_(credentials) {
}

bool API::HandleSetPassphrase(const Json::Value& args, Json::Value& result) {
  const std::string new_passphrase = args["new_passphrase"].asString();
  bytes_t salt, check, encrypted_ephemeral_key;
  if (credentials_->SetPassphrase(new_passphrase,
                                  salt,
                                  check,
                                  encrypted_ephemeral_key)) {
    result["salt"] = to_hex(salt);
    result["check"] = to_hex(check);
    result["ekey_enc"] = to_hex(encrypted_ephemeral_key);
  } else {
    SetError(result, ERROR_INVALID_PARAM, "set-passphrase failed");
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
    credentials_->Load(salt,
                       check,
                       encrypted_ephemeral_key);
    result["success"] = true;
  } else {
    SetError(result, ERROR_MISSING_PARAM,
             "missing valid salt/check/ekey_enc params");
  }
  return true;
}

bool API::HandleLock(const Json::Value& /*args*/, Json::Value& result) {
  result["success"] = credentials_->Lock();
  GenerateMasterNode();
  return true;
}

bool API::HandleUnlock(const Json::Value& args, Json::Value& result) {
  const std::string passphrase = args["passphrase"].asString();
  if (passphrase.size() != 0) {
    result["success"] = credentials_->Unlock(passphrase);
    GenerateMasterNode();
  } else {
    SetError(result, ERROR_MISSING_PARAM, "missing valid passphrase param");
  }
  return true;
}

void API::GenerateNodeResponse(Json::Value& dict, const Node* node,
                               const bytes_t& ext_prv_enc,
                               bool include_prv) {
  dict["fp"] = "0x" + to_fingerprint(node->fingerprint());
  dict["pfp"] = "0x" + to_fingerprint(node->parent_fingerprint());
  dict["child_num"] = node->child_num();
  dict["ext_pub_b58"] = Base58::toBase58Check(node->toSerializedPublic());
  if (!ext_prv_enc.empty()) {
    dict["ext_prv_enc"] = to_hex(ext_prv_enc);
  }
  if (node->is_private() && include_prv) {
    dict["ext_prv_b58"] = Base58::toBase58Check(node->toSerialized());
  }
}

bool API::HandleDeriveRootNode(const Json::Value& args, Json::Value& result) {
  const bytes_t seed(unhexlify(args["seed_hex"].asString()));

  bytes_t ext_prv_enc;
  if (Wallet::DeriveRootNode(credentials_, seed, ext_prv_enc)) {
    std::auto_ptr<Node> node(Wallet::RestoreNode(credentials_, ext_prv_enc));
    GenerateNodeResponse(result, node.get(), ext_prv_enc, true);
  } else {
    SetError(result, ERROR_INVALID_PARAM, "Root node derivation failed");
  }
  return true;
}

bool API::HandleGenerateRootNode(const Json::Value& /*args*/,
                                 Json::Value& result) {
  bytes_t ext_prv_enc;
  if (Wallet::GenerateRootNode(credentials_, ext_prv_enc)) {
    std::auto_ptr<Node> node(Wallet::RestoreNode(credentials_, ext_prv_enc));
    GenerateNodeResponse(result, node.get(), ext_prv_enc, true);
  } else {
    SetError(result, ERROR_INVALID_PARAM, "Root node generation failed");
  }
  return true;
}

bool API::HandleImportRootNode(const Json::Value& args,
                               Json::Value& result) {
  if (args.isMember("ext_prv_b58")) {
    const std::string ext_prv_b58(args["ext_prv_b58"].asString());
    bytes_t ext_prv_enc;
    if (Wallet::ImportRootNode(credentials_, ext_prv_b58, ext_prv_enc)) {
      std::auto_ptr<Node> node(Wallet::RestoreNode(credentials_, ext_prv_enc));
      GenerateNodeResponse(result, node.get(), ext_prv_enc, true);
    } else {
      SetError(result, ERROR_INVALID_PARAM, "Extended key failed validation");
    }
  } else {
    SetError(result, ERROR_MISSING_PARAM,
             "Missing required ext_prv_b58 param");
  }
  return true;
}

bool API::HandleDeriveChildNode(const Json::Value& args,
                                Json::Value& result) {
  const std::string path(args["path"].asString());
  const bool is_watch_only(args["is_watch_only"].asBool());

  std::auto_ptr<Node> node;
  bytes_t ext_prv_enc;
  if (is_watch_only) {
    std::string ext_pub_b58;
    if (Wallet::DeriveChildNode(master_node_.get(), path, ext_pub_b58)) {
      node.reset(Wallet::RestoreNode(ext_pub_b58));
    }
  } else {
    if (Wallet::DeriveChildNode(credentials_, master_node_.get(), path,
                                ext_prv_enc)) {
      node.reset(Wallet::RestoreNode(credentials_, ext_prv_enc));
    }
  }
  if (node.get()) {
    GenerateNodeResponse(result, node.get(), ext_prv_enc, is_watch_only);
    result["path"] = path;
  } else {
    SetError(result, ERROR_DERIVATION_FAILED, "Failed to derive child node");
  }
  return true;
}

void API::GenerateMasterNode() {
  if (ext_prv_enc_.empty()) {
    return;
  }
  if (credentials_->isLocked()) {
    master_node_.reset(Wallet::RestoreNode(ext_pub_b58_));
  } else {
    master_node_.reset(Wallet::RestoreNode(credentials_, ext_prv_enc_));
  }
}

bool API::HandleDescribeNode(const Json::Value& args, Json::Value& result) {
  const std::string ext_pub_b58(args["ext_pub_b58"].asString());
  if (ext_pub_b58.empty()) {
    SetError(result, ERROR_MISSING_PARAM, "Missing ext_pub_b58 param");
    return true;
  }
  std::auto_ptr<Node> node(Wallet::RestoreNode(ext_pub_b58));
  if (!node.get()) {
    SetError(result, ERROR_INVALID_PARAM, "ext_pub_b58 validation failed");
    return true;
  }

  GenerateNodeResponse(result, node.get(), bytes_t(), false);

  return true;
}

bool API::HandleRestoreNode(const Json::Value& args, Json::Value& result) {
  const std::string ext_pub_b58(args["ext_pub_b58"].asString());
  if (ext_pub_b58.empty()) {
    SetError(result, ERROR_MISSING_PARAM, "Missing ext_pub_b58 param");
    return true;
  }
  std::auto_ptr<Node> node(Wallet::RestoreNode(ext_pub_b58));
  if (!node.get()) {
    SetError(result, ERROR_INVALID_PARAM, "ext_pub_b58 validation failed");
    return true;
  }

  const bool is_master = (node->parent_fingerprint() == 0x00000000 &&
                          node->child_num() == 0);

  const bytes_t ext_prv_enc(unhexlify(args["ext_prv_enc"].asString()));
  if (is_master && ext_prv_enc.empty()) {
    SetError(result, ERROR_MISSING_PARAM,
             "Missing ext_prv_enc param for master node");
    return true;
  }

  if (is_master) {
    ext_pub_b58_ = ext_pub_b58;
    ext_prv_enc_ = ext_prv_enc;
    GenerateMasterNode();
  } else {
    wallet_.reset(new Wallet(blockchain_, credentials_,
                             ext_pub_b58, ext_prv_enc));
    result["wallet"] = wallet_.get();
  }
  GenerateNodeResponse(result, node.get(), ext_prv_enc, false);

  return true;
}

void API::PopulateAddress(const Address* address, Json::Value& root) {
  root["addr_b58"] = Base58::hash160toAddress(address->hash160());
  root["child_num"] = address->child_num();
  root["is_public"] = address->is_public();
  root["value"] = (Json::Value::UInt64)address->balance();
  root["tx_count"] = (Json::Value::UInt64)address->tx_count();
}

void API::PopulateHistoryItem(const HistoryItem* item, Json::Value& root) {
  root["tx_hash"] = to_hex(item->tx_hash());
  root["addr_b58"] = Base58::hash160toAddress(item->hash160());
  root["timestamp"] = (Json::Value::UInt64)item->timestamp();
  root["value"] = (Json::Value::Int64)item->value();
  root["fee"] = (Json::Value::UInt64)item->fee();
}

bool API::HandleGetAddresses(const Json::Value& /*args*/,
                             Json::Value& result) {
  if (!wallet_.get()) {
    SetError(result, ERROR_MISSING_CHILD_NODE, "No child node set");
    return true;
  }

  Address::addresses_t addresses;

  wallet_->GetAddresses(addresses);

  result["addresses"] = Json::Value();
  for (Address::addresses_t::const_iterator i = addresses.begin();
       i != addresses.end();
       ++i) {
    Json::Value root;
    PopulateAddress(*i, root);
    result["addresses"].append(root);
  }
  return true;
}

bool API::HandleGetHistory(const Json::Value& /*args*/,
                           Json::Value& result) {
  if (!wallet_.get()) {
    SetError(result, ERROR_MISSING_CHILD_NODE, "No child node set");
    return true;
  }

  Address::addresses_t addresses;
  wallet_->GetAddresses(addresses);

  Blockchain::address_set_t address_set;
  for (Address::addresses_t::const_iterator i = addresses.begin();
       i != addresses.end();
       ++i) {
    address_set.insert((*i)->hash160());
  }

  std::vector<const Transaction*> transactions;
  blockchain_->GetTransactionsForAddresses(address_set, transactions);

  history_t history;
  for (std::vector<const Transaction*>::const_iterator i =
         transactions.begin();
       i != transactions.end();
       ++i) {
    HistoryItem item = blockchain_->TransactionToHistoryItem(address_set, *i);
    history.push_back(item);
  }

  result["history"] = Json::Value();
  for (history_t::const_iterator i = history.begin();
       i != history.end();
       ++i) {
    Json::Value root;
    PopulateHistoryItem(&(*i), root);
    result["history"].append(root);
  }
  return true;
}

bool API::HandleReportTxStatuses(const Json::Value& args,
                                 Json::Value& /*result*/) {
  Json::Value tx_statuses(args["tx_statuses"]);
  for (Json::Value::iterator i = tx_statuses.begin();
       i != tx_statuses.end();
       ++i) {
    blockchain_->ConfirmTransaction(unhexlify((*i)["tx_hash"].asString()),
                                    (*i)["height"].asUInt64());
  }
  return true;
}

bool API::HandleReportTxs(const Json::Value& args, Json::Value& /*result*/) {
  Json::Value txs(args["txs"]);
  for (Json::Value::iterator i = txs.begin(); i != txs.end(); ++i) {
    blockchain_->AddTransaction(unhexlify((*i)["tx"].asString()));
  }
  wallet_->UpdateAddressBalancesAndTxCounts();
  return true;
}

bool API::HandleCreateTx(const Json::Value& args, Json::Value& result) {
  const bool should_sign = args["sign"].asBool();
  const uint64_t fee = args["fee"].asUInt64();

  tx_outs_t recipient_txos;
  for (unsigned int i = 0; i < args["recipients"].size(); ++i) {
    Json::Value recipient = args["recipients"][i];
    const std::string address(recipient["addr_b58"].asString());
    const bytes_t recipient_addr_b58(Base58::fromAddress(address));
    uint64_t value = recipient["value"].asUInt64();
    TxOut recipient_txo(value, recipient_addr_b58);
    recipient_txos.push_back(recipient_txo);
  }

  bytes_t tx;
  if (wallet_->CreateTx(recipient_txos, fee, should_sign, tx)) {
    result["tx"] = to_hex(tx);
  } else {
    SetError(result, ERROR_TRANSACTION_FAILED,
             "Transaction creation failed.");
  }
  return true;
}

bool API::HandleConfirmBlock(const Json::Value& args,
                             Json::Value& /*result*/) {
  const uint64_t block_height = args["block_height"].asUInt64();
  const uint64_t timestamp = args["timestamp"].asUInt64();
  blockchain_->ConfirmBlock(block_height, timestamp);
  return true;
}

void API::GetError(const Json::Value& obj, Error& code, std::string& message) {
  if (!obj.isMember("error")) {
    code = ERROR_NONE;
    message = "No error";
  } else {
    code = (Error)obj["error"].get("code", ERROR_YOU_WIN).asInt();
    message = obj["error"].get("message", "Missing error message").asString();
  }
}

Error API::GetErrorCode(const Json::Value& obj) {
  Error code;
  std::string message;
  GetError(obj, code, message);
  return code;
}

bool API::DidResponseSucceed(const Json::Value& obj) {
  return GetErrorCode(obj) == ERROR_NONE;
}

void API::SetError(Json::Value& obj, Error code, const std::string& message) {
  obj["error"]["code"] = code;
  if (message.size() > 0) {
    obj["error"]["message"] = message;
  } else {
    obj["error"]["message"] = "Unspecified error";
  }
}
