#include "node.h"

#include <iomanip>
#include <sstream>
#include <stdint.h>

#include "base58.h"
#include "json/reader.h"
#include "json/writer.h"
#include "node_factory.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "rng.h"
#include "types.h"

class HDWalletDispatcherInstance : public pp::Instance {
public:
  explicit HDWalletDispatcherInstance(PP_Instance instance)
  : pp::Instance(instance) {}
  virtual ~HDWalletDispatcherInstance() {}

  void PopulateDictionaryFromNode(Json::Value& dict, Node* node) {
    dict["hex_id"] = to_hex(node->hex_id());
    dict["fingerprint"] = "0x" + to_fingerprint(node->fingerprint());
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

  virtual bool HandleGetWalletNode(const Json::Value& args,
                                   Json::Value& result) {
    const std::string seed_hex = args.get("seed_hex", "").asString();
    const bytes_t seed_bytes(unhexlify(seed_hex));

    Node *parent_node = NULL;
    if (seed_bytes.size() == 78) {
      parent_node = NodeFactory::CreateNodeFromExtended(seed_bytes);
    } else if (seed_hex[0] == 'x') {
      parent_node =
        NodeFactory::CreateNodeFromExtended(Base58::fromBase58Check(seed_hex));
    } else {
      parent_node = NodeFactory::CreateNodeFromSeed(seed_bytes);
    }

    const std::string node_path = args.get("path", "m").asString();
    Node* node =
      NodeFactory::DeriveChildNodeWithPath(*parent_node, node_path);
    delete parent_node;

    PopulateDictionaryFromNode(result, node);
    delete node;

    return true;
  }

  virtual bool HandleCreateRandomNode(const Json::Value& /*args*/,
                                      Json::Value& result) {
    RNG rng;
    const size_t SEED_SIZE = 32;
    const bytes_t seed_bytes = rng.GetRandomBytes(SEED_SIZE);
    if (seed_bytes.size() != SEED_SIZE) {
      result["error_code"] = -1;
      result["error_message"] =
        std::string("The PRNG has not been seeded with enough "
                    "randomness to ensure an unpredictable byte sequence.");
      return true;
    }

    Node *node = NodeFactory::CreateNodeFromSeed(seed_bytes);
    PopulateDictionaryFromNode(result, node);
    delete node;
    return true;
  }

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
    const std::string command = root.get("command", "UTF-8").asString();
    Json::Value result;
    bool handled = false;
    if (command == "get-wallet-node") {
      handled = HandleGetWalletNode(root, result);
    }
    if (command == "create-random-node") {
      handled = HandleCreateRandomNode(root, result);
    }
    if (handled) {
      result["command"] = command;
      Json::StyledWriter writer;
      pp::Var reply_message(writer.write(result));
      PostMessage(reply_message);
    }
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
