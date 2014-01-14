#include <string>

#include "types.h"

class Base58 {
 public:
  static std::string toBase58Check(const bytes_t& bytes);
  static bytes_t fromBase58Check(const std::string s);

  static bytes_t toHash160(const bytes_t& bytes);
  static std::string toAddress(const bytes_t& bytes);
  static std::string toPrivateKey(const bytes_t& bytes);
};
