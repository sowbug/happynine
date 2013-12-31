#include "node.h"

#include <sstream>
#include <string>

#include "base58.h"
#include "openssl/hmac.h"
#include "openssl/ripemd.h"
#include "openssl/sha.h"
#include "secp256k1.h"

Node::Node(const bytes_t& key,
           const bytes_t& chain_code,
           uint32_t version,
           unsigned int depth,
           uint32_t parent_fingerprint,
           uint32_t child_num) :
  version_(version),
  depth_(depth),
  parent_fingerprint_(parent_fingerprint),
  child_num_(child_num) {
  set_key(key);
  set_chain_code(chain_code);
}

Node::~Node() {
}

std::string Node::toString() const {
  std::stringstream ss;
  ss << "version: " << std::hex << version_ << std::endl
     << "fingerprint: " << std::hex << fingerprint_ << std::endl
     << "secret_key: " << to_hex(secret_key_) << std::endl
     << "public_key: " << to_hex(public_key_) << std::endl
     << "chain_code: " << to_hex(chain_code_) << std::endl
     << "parent_fingerprint: " << std::hex << parent_fingerprint_ << std::endl
     << "depth: " << depth_ << std::endl
     << "child_num: " << child_num_ << std::endl
     << "serialized: " << to_hex(toSerialized()) << std::endl
     << "serialized-base58: " <<
    Base58::toBase58Check(toSerialized()) << std::endl
    ;

  return ss.str();
}

void Node::set_key(const bytes_t& new_key) {
  secret_key_.clear();
  // TODO(miket): check key_num validity
  is_private_ = new_key.size() == 32;
  version_ = is_private_ ? 0x0488ADE4 : 0x0488B21E;
  if (is_private()) {
    secret_key_ = new_key;
    secp256k1_key curvekey;
    curvekey.setPrivKey(secret_key_);
    public_key_ = curvekey.getPubKey();
  } else {
    public_key_ = new_key;
  }
  update_fingerprint();
}

void Node::set_chain_code(const bytes_t& new_code) {
  chain_code_ = new_code;
}

void Node::update_fingerprint() {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &public_key_[0], public_key_.size());
  SHA256_Final(hash, &sha256);

  unsigned char ripemd_hash[RIPEMD160_DIGEST_LENGTH];
  RIPEMD160_CTX ripemd;
  RIPEMD160_Init(&ripemd);
  RIPEMD160_Update(&ripemd, hash, SHA256_DIGEST_LENGTH);
  RIPEMD160_Final(ripemd_hash, &ripemd);

  fingerprint_ = (uint32_t)ripemd_hash[0] << 24 |
    (uint32_t)ripemd_hash[1] << 16 |
    (uint32_t)ripemd_hash[2] << 8 |
    (uint32_t)ripemd_hash[3];
}

bytes_t Node::toSerialized(bool private_if_available) const {
  bytes_t s;
  bool should_generate_private = is_private() && private_if_available;

  // 4 byte: version bytes (mainnet: 0x0488B21E public, 0x0488ADE4 private;
  // testnet: 0x043587CF public, 0x04358394 private)
  uint32_t version = should_generate_private ? 0x0488ADE4 : 0x0488B21E;
  s.push_back((uint32_t)version >> 24);
  s.push_back(((uint32_t)version >> 16) & 0xff);
  s.push_back(((uint32_t)version >> 8) & 0xff);
  s.push_back((uint32_t)version & 0xff);

  // 1 byte: depth: 0x00 for master nodes, 0x01 for level-1 descendants, etc.
  s.push_back(depth_);

  // 4 bytes: the fingerprint of the parent's key (0x00000000 if master key)
  s.push_back((uint32_t)parent_fingerprint_ >> 24);
  s.push_back(((uint32_t)parent_fingerprint_ >> 16) & 0xff);
  s.push_back(((uint32_t)parent_fingerprint_ >> 8) & 0xff);
  s.push_back((uint32_t)parent_fingerprint_ & 0xff);

  // 4 bytes: child number. This is the number i in xi = xpar/i, with xi
  // the key being serialized. This is encoded in MSB order.
  // (0x00000000 if master key)
  s.push_back((uint32_t)child_num_ >> 24);
  s.push_back(((uint32_t)child_num_ >> 16) & 0xff);
  s.push_back(((uint32_t)child_num_ >> 8) & 0xff);
  s.push_back((uint32_t)child_num_ & 0xff);

  // 32 bytes: the chain code
  s.insert(s.end(), chain_code().begin(), chain_code().end());

  // 33 bytes: the public key or private key data (0x02 + X or 0x03 + X
  // for public keys, 0x00 + k for private keys)
  bool use_private = is_private() && private_if_available;
  const bytes_t& key = use_private ? secret_key() : public_key();
  if (use_private) {
    s.push_back(0x00);
  }
  s.insert(s.end(), key.begin(), key.end());

  return s;
}

bytes_t Node::toSerializedPublic() const {
  return toSerialized(false);
}

bytes_t Node::toSerialized() const {
  return toSerialized(true);
}
