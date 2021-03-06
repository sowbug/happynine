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

#if !defined(__MNEMONIC_H__)
#define __MNEMONIC_H__

#include <string>
#include <vector>

#include "types.h"

#define BIP_0039_DICTIONARY_SIZE (2048)

class Mnemonic {
 public:
  Mnemonic();
  virtual ~Mnemonic();

  bool CodeToEntropy(const std::string& code, bytes_t& entropy);
  bool CodeToSeed(const std::string& code,
                  const std::string& passphrase,
                  bytes_t& seed);

 private:
  std::vector<std::string> words_;

  DISALLOW_EVIL_CONSTRUCTORS(Mnemonic);
};

#endif  // #if !defined(__MNEMONIC_H__)
