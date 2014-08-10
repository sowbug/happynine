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
#include <iostream>  // cerr
#include <sstream>

#include <openssl/bn.h>

#include "bigint.h"
#include "crypto.h"
#include "mnemonic.h"
#include "errors.h"

#include "bip0039_dicts/english.cc"

Mnemonic::Mnemonic() {
  for (size_t i = 0; i < sizeof(static_words) / sizeof(char*); ++i) {
    words_.push_back(static_words[i]);
  }
  if (words_.size() != BIP_0039_DICTIONARY_SIZE) {
    std::cerr << "Unexpected dictionary size: " <<
      words_.size() << std::endl;
  }
}

Mnemonic::~Mnemonic() {
}

bool Mnemonic::CodeToEntropy(const std::string& code,
                             bytes_t& entropy) {
  std::vector<int> indexes;
  {
    std::istringstream iss(code);

    do {
      std::string word;
      iss >> word;
      if (word.empty()) {
        continue;
      }

      std::vector<std::string>::iterator word_ptr = std::find(words_.begin(),
                                                              words_.end(),
                                                              word);
      if (word_ptr == words_.end()) {
        std::cerr << "code word " << word << " not in dictionary" << std::endl;
        return false;
      }
      size_t index = std::distance(words_.begin(), word_ptr);
      indexes.push_back(index);
    } while (iss);
  }

  // ENT in BIP
  const size_t entropy_length_bits = indexes.size() * 32 * 11 / 33;

  // CS in BIP
  const size_t checksum_length_bits = entropy_length_bits / 32;

  if (entropy_length_bits < 128 || entropy_length_bits > 256) {
    std::cerr << "unexpected entropy size: " <<
      entropy_length_bits << std::endl;
    return false;
  }

  BigInt bi_entropy;

  for (size_t i = 0; i < entropy_length_bits + checksum_length_bits; i += 11) {
    // Make room for incoming word index.
    bi_entropy <<= 11;

    // Convert the next word index into a BigInt.
    BigInt bi_index = indexes[i / 11];

    // Add the word index into the running number.
    bi_entropy += bi_index;
  }

  // Isolate the checksum and copy to bytes.
  BigInt bi_checksum(bi_entropy);
  bi_checksum.maskBits(checksum_length_bits);
  bytes_t checksum_bytes(bi_checksum.getBytes());
  checksum_bytes.resize((checksum_length_bits + 7) / 8);

  // Strip the checksum from the entropy, then copy entropy to bytes.
  bi_entropy >>= checksum_length_bits;
  entropy = bi_entropy.getBytes();
  entropy.resize((entropy_length_bits + 7) / 8);

  // Verify the checksum.
  bytes_t entropy_hashed(Crypto::SHA256(entropy));
  unsigned char checksum(checksum_bytes[0]);
  unsigned char calculated_checksum(entropy_hashed[0]);
  calculated_checksum >>= 8 - checksum_length_bits;
  if (checksum != calculated_checksum) {
    std::cerr << "checksum failed: expected " << std::hex << int(checksum) <<
      " but got " << int(calculated_checksum) <<
      checksum_length_bits << std::endl;
    entropy.clear();
    return false;
  }

  return true;
}

bool Mnemonic::CodeToSeed(const std::string& /*code*/, bytes_t& /*seed*/) {
  return false;
}
