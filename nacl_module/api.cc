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

API::API(Credentials& credentials, Wallet& wallet)
  : credentials_(credentials), wallet_(wallet) {
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

bool API::HandleDeriveRootNode(const Json::Value& args, Json::Value& result) {
  const bytes_t seed(unhexlify(args["seed_hex"].asString()));

  bytes_t ext_prv_enc;
  if (wallet_.DeriveRootNode(seed, ext_prv_enc)) {
    GenerateNodeResponse(result, wallet_.GetRootNode(), ext_prv_enc, true);
  } else {
    SetError(result, -1, "Root node generation failed");
  }
  return true;
}

bool API::HandleGenerateRootNode(const Json::Value& /*args*/,
                                 Json::Value& result) {
  bytes_t ext_prv_enc;
  if (wallet_.GenerateRootNode(ext_prv_enc)) {
    GenerateNodeResponse(result, wallet_.GetRootNode(), ext_prv_enc, true);
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
  Wallet::address_statuses_t items;
  wallet_.GetAddressStatusesToReport(items);
  for (Wallet::address_statuses_t::const_iterator i = items.begin();
       i != items.end();
       ++i) {
    Json::Value as;
    as["addr_b58"] = Base58::hash160toAddress(i->hash160);
    as["value"] = (Json::Value::UInt64)i->value;
    as["is_public"] = i->is_public;
    json_value.append(as);
  }
}

void API::PopulateTxRequests(Json::Value& json_value) {
  Wallet::tx_requests_t items;
  wallet_.GetTxRequestsToReport(items);
  for (Wallet::tx_requests_t::const_iterator i = items.begin();
       i != items.end();
       ++i) {
    json_value.append(to_hex(*i));
  }
}

void API::PopulateResponses(Json::Value& root) {
  PopulateAddressStatuses(root["address_statuses"]);
  PopulateTxRequests(root["tx_requests"]);
}

bool API::HandleDeriveChildNode(const Json::Value& args,
                                Json::Value& result) {
  const std::string path(args["path"].asString());
  const bool isWatchOnly(args["isWatchOnly"].asBool());

  bytes_t ext_prv_enc;
  if (wallet_.DeriveChildNode(path, isWatchOnly, ext_prv_enc)) {
    GenerateNodeResponse(result, wallet_.GetChildNode(), ext_prv_enc, false);
    PopulateResponses(result);
    result["path"] = path;
  } else {
    std::stringstream s;
    s << "Failed to derive child node @ "<< path << " from " <<
      std::hex << wallet_.GetRootNode()->fingerprint();
    SetError(result, -1, s.str());
  }

  return true;
}

bool API::HandleAddChildNode(const Json::Value& /*args*/,
                             Json::Value& result) {
  PopulateResponses(result);
  return false;
}

bool API::HandleReportTxStatuses(const Json::Value& args,
                                 Json::Value& result) {
  Json::Value tx_statuses(args["tx_statuses"]);
  for (Json::Value::iterator i = tx_statuses.begin();
       i != tx_statuses.end();
       ++i) {
    wallet_.HandleTxStatus(unhexlify((*i)["hash"].asString()),
                           (*i)["height"].asUInt());
  }
  PopulateResponses(result);
  return true;
}

bool API::HandleReportTxs(const Json::Value& args, Json::Value& result) {
  Json::Value txs(args["txs"]);
  for (Json::Value::iterator i = txs.begin(); i != txs.end(); ++i) {
    wallet_.HandleTx(unhexlify((*i)["tx"].asString()));
  }
  PopulateResponses(result);
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
  if (wallet_.CreateTx(recipient_txos, fee, should_sign, tx)) {
    result["tx"] = to_hex(tx);
    PopulateResponses(result);
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
