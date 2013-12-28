#include <string>

#include "types.h"

class Base58 {
 public:
  static std::string toBase58Check(const bytes_t& bytes);
  static bytes_t fromBase58Check(const std::string s);
};
