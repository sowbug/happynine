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

std::string to_fingerprint(uint32_t fingerprint) {
  std::stringstream stream;
  stream << std::setfill ('0') << std::setw(sizeof(uint32_t) * 2)
         << std::hex << fingerprint;
  return stream.str();
}

// http://stackoverflow.com/a/9622208/344467
int to_int(int c) {
  if (not isxdigit(c)) return -1; // error: non-hexadecimal digit found
  if (isdigit(c)) return c - '0';
  if (isupper(c)) c = tolower(c);
  return c - 'a' + 10;
}

bytes_t unhexlify(const std::string& s) {
  bytes_t result(s.size() >> 1);
  unhexlify(&s[0], &s[s.size()], &result[0]);
  return result;
}
