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

#include <json/reader.h>
#include <json/writer.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include "api.h"
#include "blockchain.h"
#include "credentials.h"
#include "node.h"

class HDWalletDispatcherInstance : public pp::Instance {
public:
  explicit HDWalletDispatcherInstance(PP_Instance instance)
  : pp::Instance(instance), credentials_() {
  }

  virtual ~HDWalletDispatcherInstance() {}

  /// Handler for messages coming in from the browser via
  /// postMessage().  The @a var_message can contain be any pp:Var
  /// type; for example int, string Array or Dictionary. Please see
  /// the pp:Var documentation for more details.  @param[in]
  /// var_message The message posted by the browser.
  virtual void HandleMessage(const pp::Var& var_message) {
    if (!var_message.is_string())
      return;
    std::string message = var_message.AsString();
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(message, root);
    if (!parsingSuccessful) {
      //                 << reader.getFormattedErrorMessages();
      return;
    }
    const std::string method = root.get("method",
                                        "[missing method]").asString();
    const Json::Value params = root.get("params", "{}");
    Json::Value result;
    bool handled = false;
    API api(blockchain_, credentials_);

    if (method == "set-passphrase") {
      handled = api.HandleSetPassphrase(params, result);
    }
    if (method == "set-credentials") {
      handled = api.HandleSetCredentials(params, result);
    }
    if (method == "lock") {
      handled = api.HandleLock(params, result);
    }
    if (method == "unlock") {
      handled = api.HandleUnlock(params, result);
    }
    if (method == "derive-root-node") {
      handled = api.HandleDeriveRootNode(params, result);
    }
    if (method == "generate-root-node") {
      handled = api.HandleGenerateRootNode(params, result);
    }
    if (method == "import-root-node") {
      handled = api.HandleImportRootNode(params, result);
    }
    if (method == "derive-child-node") {
      handled = api.HandleDeriveChildNode(params, result);
    }
    if (method == "restore-node") {
      handled = api.HandleRestoreNode(params, result);
    }
    if (method == "report-tx-statuses") {
      handled = api.HandleReportTxStatuses(params, result);
    }
    if (method == "report-txs") {
      handled = api.HandleReportTxs(params, result);
    }
    if (method == "create-tx") {
      handled = api.HandleCreateTx(params, result);
    }
    if (!handled) {
      result["error"]["code"] = -999;
      result["error"]["message"] = "Unrecognized method";
    }
    Json::Value response;
    response["id"] = root["id"];
    response["jsonrpc"] = "2.0";
    response["result"] = result;
    if (result.isMember("error")) {
      response["error"] = result["error"];
    }
    Json::StyledWriter writer;
    pp::Var reply_message(writer.write(response));
    PostMessage(reply_message);
  }

private:
  Blockchain blockchain_;
  Credentials credentials_;
};

/// The Module class.  The browser calls the CreateInstance() method
/// to create an instance of your NaCl module on the web page.  The
/// browser creates a new instance for each <embed> tag with
/// type="application/x-pnacl".
class HDWalletDispatcherModule : public pp::Module {
public:
  HDWalletDispatcherModule() : pp::Module() {}
  virtual ~HDWalletDispatcherModule() {}

  /// Create and return a HDWalletDispatcherInstance object.
  /// @param[in] instance The browser-side instance.
  /// @return the plugin-side instance.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new HDWalletDispatcherInstance(instance);
  }
};

namespace pp {
  /// Factory function called by the browser when the module is first
  /// loaded.  The browser keeps a singleton of this module.  It calls
  /// the CreateInstance() method on the object you return to make
  /// instances.  There is one instance per <embed> tag on the page.
  /// This is the main binding point for your NaCl module with the
  /// browser.
  Module* CreateModule() {
    return new HDWalletDispatcherModule();
  }
}  // namespace pp
