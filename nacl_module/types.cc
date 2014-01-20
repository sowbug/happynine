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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "types.h"

std::string to_hex(const bytes_t& bytes) {
  std::stringstream out;
  size_t len = bytes.size();

  for (size_t i = 0; i < len; i++) {
    out << std::setw(2) << std::hex << std::setfill('0') <<
      static_cast<unsigned int>(bytes[i]);
  }

  return out.str();
}

std::string to_hex_reversed(const bytes_t& bytes) {
  bytes_t bytes_reversed(bytes);
  std::reverse(bytes_reversed.begin(), bytes_reversed.end());
  return to_hex(bytes_reversed);
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
