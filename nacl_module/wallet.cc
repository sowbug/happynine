#include "wallet.h"

#include <sstream>
#include <string>

#include "bigint.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"
#include "secp256k1.h"

const std::string CURVE_ORDER_BYTES(
"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
const BigInt CURVE_ORDER(CURVE_ORDER_BYTES);

Wallet::Wallet(const MasterKey& master_key) :
  master_key_(master_key),
  version_(0),
  depth_(0),
  parent_fingerprint_(0),
  child_num_(0) {
}

Wallet::~Wallet() {
}

bool Wallet::GetChildNode(uint32_t i, Wallet& child) const {
  // If the caller is asking for a private derivation but we don't
  // have the private key, exit with error.
  bool wants_private = (i & 0x80000000) != 0;
  if (wants_private && !master_key_.is_private()) {
    return false;
  }
  bytes_t secret_key = master_key_.secret_key();
  bytes_t public_key = master_key_.public_key();

  // https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#private-child-key-derivation
  bytes_t child_data;

  if (wants_private) {
    // Push a zero, then the parent's private key
    child_data.push_back((uint8_t)0x00);
    child_data.insert(child_data.end(), secret_key.begin(), secret_key.end());
  } else {
    // Push just the parent's public key
    child_data.insert(child_data.end(), public_key.begin(), public_key.end());
  }
  // Push i
  child_data.push_back(i >> 24);
  child_data.push_back((i >> 16) & 0xff);
  child_data.push_back((i >> 8) & 0xff);
  child_data.push_back(i & 0xff);

  // Now HMAC the whole thing
  bytes_t digest;
  digest.reserve(EVP_MAX_MD_SIZE);
  HMAC(EVP_sha512(),
       &master_key_.chain_code()[0],
       master_key_.chain_code().size(),
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
  if (master_key_.is_private()) {
    BigInt k(secret_key);
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
    secp256k1_point K(public_key);
    K.generator_mul(left32);
    if (K.is_at_infinity()) {
      // TODO: "and one should proceed with the next value for i."
      return false;
    }
    new_child_key = K.bytes();
  }

  child.version_ = version_;
  child.depth_ = depth_ + 1;
  child.parent_fingerprint_ = master_key_.fingerprint();
  child.child_num_ = i;

  // Chain code is right half of HMAC output
  MasterKey child_master_key(new_child_key, right32);
  child.master_key_ = child_master_key;

  return true;
}

std::string Wallet::toString() const {
  std::stringstream ss;
  ss << "version: " << std::hex << version_ << std::endl
     << "depth: " << depth_ << std::endl
     << "parent_fingerprint: " << std::hex << parent_fingerprint_ << std::endl
     << "child_num: " << child_num_ << std::endl
     << "master_key: " << master_key_.toString() << std::endl;
  return ss.str();
}
