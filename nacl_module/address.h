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

#if !defined(__ADDRESS_H__)
#define __ADDRESS_H__

#include <vector>

#include "types.h"

class Address {
 public:
  Address(const bytes_t& hash160, uint32_t child_num, bool is_public);
  
  const bytes_t& hash160() const { return hash160_; }
  uint32_t child_num() const { return child_num_; }
  bool is_public() const { return is_public_; }
  uint64_t balance() const { return balance_; }
  void set_balance(uint64_t balance) { balance_ = balance; }

  typedef std::vector<const Address*> addresses_t;

 private:
  bytes_t hash160_;
  uint32_t child_num_;
  bool is_public_;
  uint64_t balance_;
};

#endif  // #if !__ADDRESS_H__
