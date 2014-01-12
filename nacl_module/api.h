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

#include <string>

#include "types.h"

namespace Json {
  struct Value;
}
class Node;

class API {
 public:
  bool HandleCreateNode(const Json::Value& args, Json::Value& result);

  bool HandleGetNode(const Json::Value& args, Json::Value& result);

  bool HandleGetAddresses(const Json::Value& args,
                          Json::Value& result);

  bool HandleSetPassphrase(const Json::Value& args,
                           Json::Value& result);

  bool HandleUnlockWallet(const Json::Value& args,
                          Json::Value& result);

  bool HandleEncryptItem(const Json::Value& args,
                         Json::Value& result);

  bool HandleDecryptItem(const Json::Value& args,
                         Json::Value& result);

  bool HandleGetSignedTransaction(const Json::Value& args,
                                  Json::Value& result);

 private:
  void PopulateDictionaryFromNode(Json::Value& dict, Node* node);
  bool VerifyCredentials(const bytes_t& key,
                         const bytes_t& check,
                         const bytes_t& internal_key_encrypted,
                         bytes_t& internal_key,
                         int& error_code,
                         std::string& error_message);
};
