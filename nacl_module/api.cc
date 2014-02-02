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
    credentials_->Load(salt,
                       check,
                       encrypted_ephemeral_key);
    result["success"] = true;
  } else {
    SetError(result, -1, "missing valid salt/check/ekey_enc params");
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
    SetError(result, -1, "Root node derivation failed");
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
    SetError(result, -1, "Root node generation failed");
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
      SetError(result, -1, "Extended key failed validation");
    }
  } else {
    SetError(result, -1, "Missing required ext_prv_b58 param");
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
    SetError(result, -1, "Failed to derive child node");
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

bool API::HandleRestoreNode(const Json::Value& args, Json::Value& result) {
  const std::string ext_pub_b58(args["ext_pub_b58"].asString());
  if (ext_pub_b58.empty()) {
    SetError(result, -1, "Missing ext_pub_b58 param");
    return false;
  }
  std::auto_ptr<Node> node(Wallet::RestoreNode(ext_pub_b58));
  if (!node.get()) {
    SetError(result, -1, "ext_pub_b58 validation failed");
    return false;
  }

  const bool is_master = (node->parent_fingerprint() == 0x00000000 &&
                          node->child_num() == 0);

  const bytes_t ext_prv_enc(unhexlify(args["ext_prv_enc"].asString()));
  if (is_master && ext_prv_enc.empty()) {
    SetError(result, -1, "Missing ext_prv_enc param for master node");
    return false;
  }

  if (is_master) {
    ext_pub_b58_ = ext_pub_b58;
    ext_prv_enc_ = ext_prv_enc;
    GenerateMasterNode();
  } else {
    wallet_.reset(new Wallet(blockchain_, credentials_,
                             ext_pub_b58, ext_prv_enc));
  }
  GenerateNodeResponse(result, node.get(), ext_prv_enc, false);

  return true;
}

bool API::HandleReportTxStatuses(const Json::Value& args,
                                 Json::Value& result) {
  Json::Value tx_statuses(args["tx_statuses"]);
  for (Json::Value::iterator i = tx_statuses.begin();
       i != tx_statuses.end();
       ++i) {
    blockchain_->ConfirmTransaction(unhexlify((*i)["tx_hash"].asString()),
                                    (*i)["height"].asUInt64());
  }
  return true;
}

bool API::HandleReportTxs(const Json::Value& args, Json::Value& result) {
  Json::Value txs(args["txs"]);
  for (Json::Value::iterator i = txs.begin(); i != txs.end(); ++i) {
    blockchain_->AddTransaction(unhexlify((*i)["tx"].asString()));
  }
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
    SetError(result, -1, "Transaction creation failed.");
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
