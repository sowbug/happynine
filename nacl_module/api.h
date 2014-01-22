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

#include <set>
#include <string>

#include "credentials.h"
#include "types.h"
#include "wallet.h"

namespace Json {
  struct Value;
}
class Node;

class API {
 public:
  API();

  // Credentials
  bool HandleSetPassphrase(const Json::Value& args, Json::Value& result);

  bool HandleSetCredentials(const Json::Value& args, Json::Value& result);

  bool HandleLock(const Json::Value& args, Json::Value& result);

  bool HandleUnlock(const Json::Value& args, Json::Value& result);

  // Root Nodes
  bool HandleGenerateRootNode(const Json::Value& args, Json::Value& result);

  bool HandleAddRootNode(const Json::Value& args, Json::Value& result);

  bool HandleImportRootNode(const Json::Value& args, Json::Value& result);

  // Child Nodes
  bool HandleDeriveChildNode(const Json::Value& args, Json::Value& result);

  bool HandleAddChildNode(const Json::Value& args, Json::Value& result);

  // The rest
  bool HandleGetNode(const Json::Value& args, Json::Value& result);

  bool HandleGetAddresses(const Json::Value& args,
                          Json::Value& result);

  bool HandleEncryptItem(const Json::Value& args,
                         Json::Value& result);

  bool HandleDecryptItem(const Json::Value& args,
                         Json::Value& result);

  bool HandleGetSignedTransaction(const Json::Value& args,
                                  Json::Value& result);

  bool HandleGetUnspentTxos(const Json::Value& args,
                            Json::Value& result);

  bool HandleReportAddressHistory(const Json::Value& args,
                                  Json::Value& result);

  bool HandleReportTransactions(const Json::Value& args,
                                Json::Value& result);

  // Utilities
  void GetError(const Json::Value& obj, int& code, std::string& message);

  int GetErrorCode(const Json::Value& obj);

  bool DidResponseSucceed(const Json::Value& obj);

 private:
  void PopulateAddressStatuses(Json::Value& json_value);
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
  Node* CreateNodeFromArgs(const Json::Value& args);
  void GetAddresses(const Node& node,
                    uint32_t start,
                    uint32_t count,
                    const std::string& base_node_path,
                    Json::Value& result);
  void GetHash160s(const Node& parent_node,
                   uint32_t start,
                   uint32_t count,
                   const std::string& base_node_path,
                   std::set<bytes_t>& result);

  Credentials credentials_;
  Wallet wallet_;
};
