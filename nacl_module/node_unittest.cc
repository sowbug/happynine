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

#include "base58.h"
#include "gtest/gtest.h"
#include "jsoncpp/json/reader.h"
#include "node.h"
#include "node_factory.h"
#include "types.h"

TEST(NodeTest, BIP0032TestVectors) {
  std::ifstream tv_json;
  tv_json.open("bip0032_test_vectors.json");
  std::string tv_json_string;
  while (!tv_json.eof()) {
    std::string line;
    std::getline(tv_json, line);
    tv_json_string += line + "\n";
  }
  tv_json.close();

  Json::Value test_vectors(Json::arrayValue);
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(tv_json_string, test_vectors);
  if (!parsingSuccessful) {
    std::cerr << reader.getFormattedErrorMessages();
    return;
  }
  for (size_t i = 0; i < test_vectors.size(); ++i) {
    const Json::Value test_vector(test_vectors[(int)i]);
    const bytes_t& seed(unhexlify(test_vector["seed"].asString()));
    std::auto_ptr<Node> parent_node(NodeFactory::CreateNodeFromSeed(seed));

    Json::Value chains(test_vector["chains"]);
    for (size_t j = 0; j < chains.size(); ++j) {
      const Json::Value chain(chains[(int)j]);
      const std::string path(chain["chain"].asString());
      std::auto_ptr<Node> child_node(NodeFactory::
                                     DeriveChildNodeWithPath(*parent_node,
                                                             path));

      EXPECT_EQ(unhexlify(chain["hex_id"].asString()),
                child_node->hex_id());
      const std::string fp_expected(chain["fingerprint"].asString());
      std::stringstream fp;
      fp << std::hex << std::setw(8) << child_node->fingerprint();
      EXPECT_EQ(fp_expected.substr(2), fp.str());
      EXPECT_EQ(chain["address"].asString(),
                Base58::toAddress(child_node->public_key()));
      if (!chain["hash160"].empty()) {
        EXPECT_EQ(unhexlify(chain["hash160"].asString()),
                  Base58::toHash160(child_node->public_key()));
      }
      EXPECT_EQ(unhexlify(chain["secret_hex"].asString()),
                child_node->secret_key());
      EXPECT_EQ(chain["secret_wif"].asString(),
                Base58::toPrivateKey(child_node->secret_key()));
      EXPECT_EQ(unhexlify(chain["public_hex"].asString()),
                child_node->public_key());
      EXPECT_EQ(unhexlify(chain["chain_code"].asString()),
                child_node->chain_code());
      EXPECT_EQ(unhexlify(chain["ext_prv_hex"].asString()),
                child_node->toSerialized());
      EXPECT_EQ(chain["ext_prv_b58"].asString(),
                Base58::toBase58Check(child_node->toSerialized()));
      EXPECT_EQ(unhexlify(chain["ext_pub_hex"].asString()),
                child_node->toSerializedPublic());
      EXPECT_EQ(chain["ext_pub_b58"].asString(),
                Base58::toBase58Check(child_node->toSerializedPublic()));
    }
  }
}

TEST(NodeTest, BIP0032TestVectorsPublicOnly) {
  std::ifstream tv_json;
  tv_json.open("bip0032_test_vectors.json");
  std::string tv_json_string;
  while (!tv_json.eof()) {
    std::string line;
    std::getline(tv_json, line);
    tv_json_string += line + "\n";
  }
  tv_json.close();

  Json::Value test_vectors(Json::arrayValue);
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(tv_json_string, test_vectors);
  if (!parsingSuccessful) {
    std::cerr << reader.getFormattedErrorMessages();
    return;
  }
  for (size_t i = 0; i < test_vectors.size(); ++i) {
    const Json::Value test_vector(test_vectors[(int)i]);
    const bytes_t& seed(unhexlify(test_vector["ext_pub_hex"].asString()));
    std::auto_ptr<Node> parent_node(NodeFactory::CreateNodeFromExtended(seed));

    Json::Value chains(test_vector["chains"]);
    for (size_t j = 0; j < chains.size(); ++j) {
      const Json::Value chain(chains[(int)j]);
      const std::string path(chain["chain"].asString());
      std::auto_ptr<Node> child_node(NodeFactory::
                                     DeriveChildNodeWithPath(*parent_node,
                                                             path));

      if (path.find('\'') != std::string::npos) {
        EXPECT_EQ(NULL, child_node.get());
        continue;
      }
      EXPECT_EQ(unhexlify(chain["hex_id"].asString()),
                child_node->hex_id());

      EXPECT_EQ(unhexlify(chain["hex_id"].asString()),
                child_node->hex_id());
      const std::string fp_expected(chain["fingerprint"].asString());
      std::stringstream fp;
      fp << std::hex << std::setw(8) << child_node->fingerprint();
      EXPECT_EQ(fp_expected.substr(2), fp.str());
      EXPECT_EQ(chain["address"].asString(),
                Base58::toAddress(child_node->public_key()));
      EXPECT_EQ(bytes_t(), child_node->secret_key());
      EXPECT_EQ(std::string(), Base58::toPrivateKey(child_node->secret_key()));
      EXPECT_EQ(unhexlify(chain["public_hex"].asString()),
                child_node->public_key());
      EXPECT_EQ(unhexlify(chain["chain_code"].asString()),
                child_node->chain_code());
      EXPECT_EQ(bytes_t(), child_node->toSerializedPrivate());
      EXPECT_EQ(std::string(),
                Base58::toBase58Check(child_node->toSerializedPrivate()));
      EXPECT_EQ(unhexlify(chain["ext_pub_hex"].asString()),
                child_node->toSerializedPublic());
      EXPECT_EQ(chain["ext_pub_b58"].asString(),
                Base58::toBase58Check(child_node->toSerializedPublic()));
    }
  }
}
