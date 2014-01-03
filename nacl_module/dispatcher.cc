#include "node.h"

#include "json/reader.h"
#include "json/writer.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

class HDWalletDispatcherInstance : public pp::Instance {
public:
  explicit HDWalletDispatcherInstance(PP_Instance instance)
  : pp::Instance(instance) {}
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
    const std::string command = root.get("command",
                                         "[missing command]").asString();
    Json::Value result;
    bool handled = false;
    API api;
    if (command == "create-node") {
      handled = api.HandleCreateNode(root, result);
    }
    if (command == "get-node") {
      handled = api.HandleGetNode(root, result);
    }
    if (command == "get-addresses") {
      handled = api.HandleGetAddresses(root, result);
    }
    if (command == "set-passphrase") {
      handled = api.HandleSetPassphrase(root, result);
    }
    if (command == "unlock-wallet") {
      handled = api.HandleUnlockWallet(root, result);
    }
    if (command == "encrypt-item") {
      handled = api.HandleEncryptItem(root, result);
    }
    if (command == "decrypt-item") {
      handled = api.HandleDecryptItem(root, result);
    }
    if (!handled) {
      result["error_code"] = -999;
    }
    result["id"] = root["id"];
    result["command"] = command;
    Json::StyledWriter writer;
    pp::Var reply_message(writer.write(result));
    PostMessage(reply_message);
  }
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
