#include <iomanip>
#include <iostream>
#include <sstream>

#include "types.h"

std::string to_hex(const bytes_t bytes) {
  std::stringstream out;
  size_t len = bytes.size();

  for (size_t i = 0; i < len; i++) {
    out << std::setw(2) << std::hex << std::setfill('0') <<
      static_cast<unsigned int>(bytes[i]);
  }

  return out.str();
}

