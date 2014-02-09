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

#include <fstream>
#include <iostream>

#include "gtest/gtest.h"
#include "jsoncpp/json/reader.h"

#include "base58.h"

TEST(Base58Test, ReferenceTests) {
  std::ifstream json;
  json.open("base58_encode_decode.json");
  std::string json_string;
  while (!json.eof()) {
    std::string line;
    std::getline(json, line);
    json_string += line + "\n";
  }
  json.close();

  Json::Value obj;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(json_string, obj);
  if (!parsingSuccessful) {
    std::cerr << reader.getFormattedErrorMessages();
    return;
  }

  for (Json::Value::iterator i = obj.begin();
       i != obj.end();
       ++i) {
    Json::Value& pair = *i;
    EXPECT_EQ(unhexlify(pair[0].asString()),
              Base58::fromBase58(pair[1].asString()));
    EXPECT_EQ(pair[1].asString(),
              Base58::toBase58(unhexlify(pair[0].asString())));
  }
}
