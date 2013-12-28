#include "node.h"

#include <sstream>
#include <string>

#include "base58.h"
#include "bigint.h"
#include "openssl/hmac.h"
#include "openssl/ripemd.h"
#include "openssl/sha.h"
#include "secp256k1.h"

const std::string CURVE_ORDER_BYTES("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\
FEBAAEDCE6AF48A03BBFD25E8CD0364141");
const BigInt CURVE_ORDER(CURVE_ORDER_BYTES);

Node::Node() :
  depth_(0),
  parent_fingerprint_(0),
  child_num_(0) {
}

Node::Node(const bytes_t& seed) :
  depth_(0),
  parent_fingerprint_(0),
  child_num_(0) {
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
  set_key(bytes_t(&digest[0], &digest[32]));
  set_chain_code(bytes_t(&digest[32], &digest[64]));
}

Node::Node(const bytes_t& bytes, bool unused) {
  if (bytes.size() == 78) {
    uint32_t version = ((uint32_t)bytes[0] << 24) |
      ((uint32_t)bytes[1] << 16) |
      ((uint32_t)bytes[2] << 8) |
      (uint32_t)bytes[3];
    depth_ = bytes[4];
    parent_fingerprint_ = ((uint32_t)bytes[5] << 24) |
      ((uint32_t)bytes[6] << 16) |
      ((uint32_t)bytes[7] << 8) |
      (uint32_t)bytes[8];
    child_num_ = ((uint32_t)bytes[9] << 24) |
      ((uint32_t)bytes[10] << 16) |
      ((uint32_t)bytes[11] << 8) |
      (uint32_t)bytes[12];
    bytes_t chain_code(&bytes[13], &bytes[45]);
    bytes_t secret_key(&bytes[45], &bytes[78]);

    // If the secret key is private, it needs to be 32 bytes.
    if (secret_key[0] == 0x00) {
      secret_key.assign(secret_key.begin() + 1, secret_key.end());
    }
    set_key(secret_key);
    set_chain_code(chain_code);

    if (version_ != version) {
      // TODO
    }
  }
}

Node::Node(const bytes_t& key,
           const bytes_t& chain_code,
           unsigned int depth,
           uint32_t parent_fingerprint,
           uint32_t child_num) :
  depth_(depth),
  parent_fingerprint_(parent_fingerprint),
  child_num_(child_num) {
  set_key(key);
  set_chain_code(chain_code);
}

Node::~Node() {
}

bool Node::GetChildNode(uint32_t i, Node& child) const {
  // If the caller is asking for a private derivation but we don't
  // have the private key, exit with error.
  bool wants_private = (i & 0x80000000) != 0;
  if (wants_private && !is_private()) {
    return false;
  }

  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#private-child-key-derivation
  bytes_t child_data;

  if (wants_private) {
    // Push the parent's private key
    child_data.push_back(0x00);
    child_data.insert(child_data.end(),
                      secret_key().begin(),
                      secret_key().end());
  } else {
    // Push just the parent's public key
    child_data.insert(child_data.end(),
                      public_key().begin(),
                      public_key().end());
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
       &chain_code()[0],
       chain_code().size(),
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
    return false;
  }

  bytes_t new_child_key;
  if (is_private()) {
    BigInt k(secret_key());
    k += iLeft;
    k %= CURVE_ORDER;
    if (k.isZero()) {
      // TODO: "and one should proceed with the next value for i."
      return false;
    }
    bytes_t child_key = k.getBytes();
    // pad with zeros to make it 32 bytes
    bytes_t padded_key(32 - child_key.size(), 0);
    padded_key.insert(padded_key.end(), child_key.begin(), child_key.end());
    new_child_key = padded_key;
  } else {
    secp256k1_point K(public_key());
    K.generator_mul(left32);
    if (K.is_at_infinity()) {
      // TODO: "and one should proceed with the next value for i."
      return false;
    }
    new_child_key = K.bytes();
  }

  // Chain code is right half of HMAC output
  child = Node(new_child_key, right32, depth_ + 1, fingerprint(), i);

  return true;
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
    secret_key_.insert(secret_key_.end(), new_key.begin(), new_key.end());
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

bytes_t Node::toSerialized() const {
  bytes_t s;

  // 4 byte: version bytes (mainnet: 0x0488B21E public, 0x0488ADE4 private;
  // testnet: 0x043587CF public, 0x04358394 private)
  s.push_back((uint32_t)version() >> 24);
  s.push_back(((uint32_t)version() >> 16) & 0xff);
  s.push_back(((uint32_t)version() >> 8) & 0xff);
  s.push_back((uint32_t)version() & 0xff);

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
  const bytes_t& key = is_private() ? secret_key() : public_key();
  if (is_private()) {
    s.push_back(0x00);
  }
  s.insert(s.end(), key.begin(), key.end());

  return s;
}
