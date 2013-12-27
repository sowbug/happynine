#include <iomanip>
#include <iostream>
#include <sstream>

#include "json/reader.h"
#include "json/writer.h"
#include "openssl/sha.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

#include "wallet.h"

class HDWalletDispatcherInstance : public pp::Instance {
public:
  explicit HDWalletDispatcherInstance(PP_Instance instance)
  : pp::Instance(instance) {}
  virtual ~HDWalletDispatcherInstance() {}

  std::string to_hex(const bytes_t bytes) {
    std::stringstream out;
    size_t len = bytes.size();

    for (size_t i = 0; i < len; i++) {
      out << std::setw(2) << std::hex << std::setfill('0') <<
        static_cast<unsigned int>(bytes[i]);
    }

    return out.str();
  }

  virtual bool HandleSHA256(const Json::Value& args,
                            Json::Value& result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;

    const std::string value = args.get("value", "UTF-8").asString();

    SHA256_Init(&sha256);
    SHA256_Update(&sha256,
                  reinterpret_cast<const unsigned char*>(value.c_str()),
                  value.length());
    SHA256_Final(hash, &sha256);
    bytes_t hash_bytes(hash, hash + SHA256_DIGEST_LENGTH);

    result["hash"] = to_hex(hash_bytes);

    return true;
  }

  virtual bool HandleCreateWallet(const Json::Value& args,
                                  Json::Value& result) {
    const unsigned char seed[16] = { 0x00, 0x01, 0x02, 0x03, 0x04,
                                     0x05, 0x06, 0x07, 0x08, 0x09,
                                     0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
                                     0x0f };
    const bytes_t seed_bytes(seed, &seed[16]);
    // args.get("seed", "UTF-8").asString()
    MasterKey master_key(seed_bytes);
    Wallet wallet(master_key);

    result["secret_key"] = to_hex(master_key.secret_key());
    result["chain_code"] = to_hex(master_key.chain_code());
    result["public_key"] = to_hex(wallet.public_key());
    result["fingerprint"] = to_hex(wallet.fingerprint());

    return true;
  }

  /// Handler for messages coming in from the browser via postMessage().  The
  /// @a var_message can contain be any pp:Var type; for example int, string
  /// Array or Dictionary. Please see the pp:Var documentation for more details.
  /// @param[in] var_message The message posted by the browser.
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
    const std::string command = root.get("command", "UTF-8").asString();
    Json::Value result;
    bool handled = false;
    if (command == "sha256") {
      handled = HandleSHA256(root, result);
    }
    if (command == "create-wallet") {
      handled = HandleCreateWallet(root, result);
    }
    if (handled) {
      result["command"] = command;
      Json::StyledWriter writer;
      pp::Var reply_message(writer.write(result));
      PostMessage(reply_message);
    }
  }
};

/// The Module class.  The browser calls the CreateInstance() method to create
/// an instance of your NaCl module on the web page.  The browser creates a new
/// instance for each <embed> tag with type="application/x-pnacl".
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
  /// Factory function called by the browser when the module is first loaded.
  /// The browser keeps a singleton of this module.  It calls the
  /// CreateInstance() method on the object you return to make instances.  There
  /// is one instance per <embed> tag on the page.  This is the main binding
  /// point for your NaCl module with the browser.
  Module* CreateModule() {
    return new HDWalletDispatcherModule();
  }
}  // namespace pp
