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

#if !defined(__BASE58_H__)
#define __BASE58_H__

#include <string>

#include "types.h"

class Base58 {
 public:
  static std::string toBase58Check(const bytes_t& bytes);
  static bytes_t fromBase58Check(const std::string s);
  static bytes_t fromAddress(const std::string addr_b58);

  static bytes_t toHash160(const bytes_t& public_key);
  static std::string hash160toAddress(const bytes_t& hash160);
  static std::string toAddress(const bytes_t& public_key);
  static std::string toPrivateKey(const bytes_t& key);
};

#endif  // #if !defined(__BASE58_H__)
