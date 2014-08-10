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

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "jsoncpp/json/reader.h"

#include "base58.h"
#include "mnemonic.h"
#include "types.h"

TEST(MnemonicTest, BIP0039TestVectors) {
  std::ifstream tv_json;
  tv_json.open("bip0039_test_vectors.json");
  std::string tv_json_string;
  while (!tv_json.eof()) {
    std::string line;
    std::getline(tv_json, line);
    tv_json_string += line + "\n";
  }
  tv_json.close();

  Json::Value dictionaries(Json::objectValue);
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(tv_json_string, dictionaries);
  if (!parsingSuccessful) {
    std::cerr << reader.getFormattedErrorMessages();
    return;
  }

  Json::Value single_dict(dictionaries["english"]);
  for (size_t i = 0; i < single_dict.size(); ++i) {
    const Json::Value test_vector(single_dict[(int)i]);
    std::cout << "entropy " << test_vector[0].asString() << " ";
    std::cout << "mnemonic " << test_vector[1].asString() << " ";
    std::cout << "code " << test_vector[2].asString() << std::endl;
  }
}
