#include "types.h"

class Node;

class NodeFactory {
 public:
  // From a seed. Typically for master key generation.
  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#master-key-generation
  static Node* CreateNodeFromSeed(const bytes_t& seed);

  // From a decoded-from-Base58 xpub/xprv string. Used for both root
  // and children.
  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#serialization-format
  static Node* CreateNodeFromExtended(const bytes_t& bytes);

  // TODO
  static Node* DeriveChildNode(const Node& parent_node,
                               uint32_t i);
};
