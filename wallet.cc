// The browser can talk to your NaCl module via the postMessage() Javascript
// function.  When you call postMessage() on your NaCl module from the browser,
// this becomes a call to the HandleMessage() method of your pp::Instance
// subclass.  You can send messages back to the browser by calling the
// PostMessage() method on your pp::Instance.  Note that these two methods
// (postMessage() in Javascript and PostMessage() in C++) are asynchronous.
// This means they return immediately - there is no waiting for the message
// to be handled.  This has implications in your program design, particularly
// when mutating property values that are exposed to both the browser and the
/// NaCl module.

#include <sstream>

#include "json/reader.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "tomcrypt.h"

/// The Instance class.  One of these exists for each instance of your NaCl
/// module on the web page.  The browser will ask the Module object to create
/// a new Instance for each occurrence of the <embed> tag that has these
/// attributes:
///     src="hello_tutorial.nmf"
///     type="application/x-pnacl"
/// To communicate with the browser, you must override HandleMessage() to
/// receive messages from the browser, and use PostMessage() to send messages
/// back to the browser.  Note that this interface is asynchronous.
class BitcoinWalletInstance : public pp::Instance {
public:
  /// The constructor creates the plugin-side instance.
  /// @param[in] instance the handle to the browser-side plugin instance.
  explicit BitcoinWalletInstance(PP_Instance instance) : pp::Instance(instance)
  {}
  virtual ~BitcoinWalletInstance() {}

  std::string to_hex(const unsigned char* bytes, size_t len) {
    std::stringstream out;

    for (size_t i = 0; i < len; i++) {
      out << std::hex << static_cast<unsigned int>(bytes[i]);
    }

    return out.str();
  }

  virtual pp::Var HandleSHA256(const Json::Value& root) {
    hash_state md;
    sha256_init(&md);

    const std::string value = root.get("value", "UTF-8").asString();
    sha256_process(&md,
                   reinterpret_cast<const unsigned char*>(value.c_str()),
                   value.length());

    unsigned char hash[32];
    sha256_done(&md, hash);

    return pp::Var(to_hex(hash, 32));
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
    pp::Var reply;
    if (command == "sha256") {
      reply = HandleSHA256(root);
    }
    PostMessage(reply);
  }
};

/// The Module class.  The browser calls the CreateInstance() method to create
/// an instance of your NaCl module on the web page.  The browser creates a new
/// instance for each <embed> tag with type="application/x-pnacl".
class BitcoinWalletModule : public pp::Module {
public:
  BitcoinWalletModule() : pp::Module() {}
  virtual ~BitcoinWalletModule() {}

  /// Create and return a BitcoinWalletInstance object.
  /// @param[in] instance The browser-side instance.
  /// @return the plugin-side instance.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new BitcoinWalletInstance(instance);
  }
};

namespace pp {
  /// Factory function called by the browser when the module is first loaded.
  /// The browser keeps a singleton of this module.  It calls the
  /// CreateInstance() method on the object you return to make instances.  There
  /// is one instance per <embed> tag on the page.  This is the main binding
  /// point for your NaCl module with the browser.
  Module* CreateModule() {
    return new BitcoinWalletModule();
  }
}  // namespace pp
