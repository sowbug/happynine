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

#include <memory>
#include <set>
#include <string>

#include "types.h"

namespace Json {
  class Value;
}

class Blockchain;
class Credentials;
class Node;
class Wallet;

class API {
 public:
  API(Blockchain* blockchain, Credentials* credentials);

  // Credentials
  bool HandleSetPassphrase(const Json::Value& args, Json::Value& result);

  bool HandleSetCredentials(const Json::Value& args, Json::Value& result);

  bool HandleLock(const Json::Value& args, Json::Value& result);

  bool HandleUnlock(const Json::Value& args, Json::Value& result);

  // Root Nodes
  bool HandleDeriveRootNode(const Json::Value& args, Json::Value& result);

  bool HandleGenerateRootNode(const Json::Value& args, Json::Value& result);

  bool HandleImportRootNode(const Json::Value& args, Json::Value& result);

  // Child Nodes
  bool HandleDeriveChildNode(const Json::Value& args, Json::Value& result);

  bool HandleAddChildNode(const Json::Value& args, Json::Value& result);

  // All Nodes
  bool HandleRestoreNode(const Json::Value& args, Json::Value& result);

  // Transactions
  bool HandleReportTxStatuses(const Json::Value& args, Json::Value& result);

  bool HandleReportTxs(const Json::Value& args, Json::Value& result);

  bool HandleCreateTx(const Json::Value& args, Json::Value& result);

  // Utilities
  void GetError(const Json::Value& obj, int& code, std::string& message);

  int GetErrorCode(const Json::Value& obj);

  bool DidResponseSucceed(const Json::Value& obj);

  Wallet* get_wallet() const { return wallet_.get(); }

 private:
  void PopulateAddressStatuses(Json::Value& json_value);
  void PopulateTxRequests(Json::Value& json_value);
  void PopulateRecentTransactions(Json::Value& json_value);
  void PopulateResponses(Json::Value& root);

  void PopulateDictionaryFromNode(Json::Value& dict, Node* node);
  void GenerateNodeResponse(Json::Value& dict, const Node* node,
                            const bytes_t& ext_prv_enc,
                            bool include_prv);
  bool VerifyCredentials(const bytes_t& key,
                         const bytes_t& check,
                         const bytes_t& internal_key_encrypted,
                         bytes_t& internal_key,
                         int& error_code,
                         std::string& error_message);
  void SetError(Json::Value& obj, int code, const std::string& message);

  void GenerateMasterNode();

  Blockchain* blockchain_;
  Credentials* credentials_;
  std::auto_ptr<Wallet> wallet_;

  std::auto_ptr<Node> master_node_;

  // Master node
  std::string ext_pub_b58_;
  bytes_t ext_prv_enc_;
  std::string child_ext_pub_b58_;
  bytes_t child_ext_prv_enc_;

  DISALLOW_EVIL_CONSTRUCTORS(API);
};
