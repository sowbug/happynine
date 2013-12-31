#include <iostream>
#include <fstream>
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
    Node *parent_node = NodeFactory::CreateNodeFromSeed(seed);

    Json::Value chains(test_vector["chains"]);
    for (size_t j = 0; j < chains.size(); ++j) {
      const Json::Value chain(chains[(int)j]);
      const std::string path(chain["chain"].asString());
      Node* child_node = NodeFactory::DeriveChildNodeWithPath(*parent_node,
                                                              path);

      const std::string fp_expected(chain["fingerprint"].asString());
      std::stringstream fp;
      fp << std::hex << std::setw(8) << child_node->fingerprint();
      EXPECT_EQ(fp_expected.substr(2), fp.str());
      EXPECT_EQ(unhexlify(chain["secret_hex"].asString()),
                child_node->secret_key());
      EXPECT_EQ(unhexlify(chain["public_hex"].asString()),
                child_node->public_key());
      EXPECT_EQ(unhexlify(chain["chain_code"].asString()),
                child_node->chain_code());
      EXPECT_EQ(unhexlify(chain["ext_prv_hex"].asString()),
                child_node->toSerialized());
      EXPECT_EQ(chain["ext_prv_b58"].asString(),
                Base58::toBase58Check(child_node->toSerialized()));
      // EXPECT_EQ(unhexlify(chain["ext_pub_hex"].asString()),
      //           child_node->toSerialized());
      // EXPECT_EQ(chain["ext_pub_b58"].asString(),
      //           Base58::toBase58Check(child_node->toSerialized()));

      delete child_node;
    }
    delete parent_node;
  }
}
