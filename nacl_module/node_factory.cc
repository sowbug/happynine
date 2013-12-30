#include "node_factory.h"

#include <string>

#include "bigint.h"
#include "node.h"
#include "openssl/hmac.h"
//#include "openssl/ripemd.h"
#include "openssl/sha.h"
#include "secp256k1.h"
#include "types.h"

const std::string CURVE_ORDER_BYTES("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\
FEBAAEDCE6AF48A03BBFD25E8CD0364141");
const BigInt CURVE_ORDER(CURVE_ORDER_BYTES);

Node* NodeFactory::CreateNodeFromSeed(const bytes_t& seed) {
  const std::string BIP0032_HMAC_KEY("Bitcoin seed");
  bytes_t digest;
  digest.resize(EVP_MAX_MD_SIZE);
  HMAC(EVP_sha512(),
       BIP0032_HMAC_KEY.c_str(),
       BIP0032_HMAC_KEY.size(),
       &seed[0],
       seed.size(),
       &digest[0],
       NULL);

  return new Node(bytes_t(&digest[0], &digest[32]),
                  bytes_t(&digest[32], &digest[64]),
                  0,
                  0,
                  0,
                  0);
}

Node* NodeFactory::CreateNodeFromExtended(const bytes_t& bytes) {
  if (bytes.size() != 78) {
    return NULL;
  }

  uint32_t version = ((uint32_t)bytes[0] << 24) |
    ((uint32_t)bytes[1] << 16) |
    ((uint32_t)bytes[2] << 8) |
    (uint32_t)bytes[3];
  unsigned int depth = bytes[4];
  uint32_t parent_fingerprint = ((uint32_t)bytes[5] << 24) |
    ((uint32_t)bytes[6] << 16) |
    ((uint32_t)bytes[7] << 8) |
    (uint32_t)bytes[8];
  uint32_t child_num = ((uint32_t)bytes[9] << 24) |
    ((uint32_t)bytes[10] << 16) |
    ((uint32_t)bytes[11] << 8) |
    (uint32_t)bytes[12];
  bytes_t chain_code(&bytes[13], &bytes[45]);
  bytes_t secret_key(&bytes[45], &bytes[78]);
  // If the secret key is private, it needs to be 32 bytes.
  if (secret_key[0] == 0x00) {
    secret_key.assign(secret_key.begin() + 1, secret_key.end());
  }

  //    if (version_ != version) {
  // TODO
  //}
  // TODO(miket): validity checks

  return new Node(secret_key,
                  chain_code,
                  version,
                  depth,
                  parent_fingerprint,
                  child_num);
}

Node* NodeFactory::DeriveChildNode(const Node& parent_node,
                                   uint32_t i) {
  // If the caller is asking for a private derivation but we don't
  // have the private key, exit with error.
  bool wants_private = (i & 0x80000000) != 0;
  if (wants_private && !parent_node.is_private()) {
    return NULL;
  }

  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#private-child-key-derivation
  bytes_t child_data;

  if (wants_private) {
    // Push the parent's private key
    child_data.push_back(0x00);
    child_data.insert(child_data.end(),
                      parent_node.secret_key().begin(),
                      parent_node.secret_key().end());
  } else {
    // Push just the parent's public key
    child_data.insert(child_data.end(),
                      parent_node.public_key().begin(),
                      parent_node.public_key().end());
  }
  // Push i
  child_data.push_back(i >> 24);
  child_data.push_back((i >> 16) & 0xff);
  child_data.push_back((i >> 8) & 0xff);
  child_data.push_back(i & 0xff);

  // Now HMAC the whole thing
  bytes_t digest;
  digest.resize(EVP_MAX_MD_SIZE);
  HMAC(EVP_sha512(),
       &parent_node.chain_code()[0],
       parent_node.chain_code().size(),
       &child_data[0],
       child_data.size(),
       &digest[0],
       NULL);

  // Split HMAC into two pieces.
  const bytes_t left32(digest.begin(), digest.begin() + 32);
  const bytes_t right32(digest.begin() + 32, digest.begin() + 64);
  const BigInt iLeft(left32);
  if (false && iLeft >= CURVE_ORDER) {
    // TODO: "and one should proceed with the next value for i."
    return NULL;
  }

  bytes_t new_child_key;
  if (parent_node.is_private()) {
    BigInt k(parent_node.secret_key());
    k += iLeft;
    k %= CURVE_ORDER;
    if (k.isZero()) {
      // TODO: "and one should proceed with the next value for i."
      return NULL;
    }
    bytes_t child_key = k.getBytes();
    // pad with zeros to make it 32 bytes
    bytes_t padded_key(32 - child_key.size(), 0);
    padded_key.insert(padded_key.end(), child_key.begin(), child_key.end());
    new_child_key = padded_key;
  } else {
    secp256k1_point K(parent_node.public_key());
    K.generator_mul(left32);
    if (K.is_at_infinity()) {
      // TODO: "and one should proceed with the next value for i."
      return NULL;
    }
    new_child_key = K.bytes();
  }

  // Chain code is right half of HMAC output
  return new Node(new_child_key,
                  right32,
                  parent_node.version(),  // TODO(miket): ?
                  parent_node.depth() + 1,
                  parent_node.fingerprint(),
                  i);
}
