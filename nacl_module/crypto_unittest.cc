#include <iostream>
#include <fstream>
#include <string>

#include "crypto.h"
#include "gtest/gtest.h"
#include "types.h"

TEST(RNGTest, Basic) {
  bytes_t bytes1(128, 0);
  bytes_t bytes2(128, 0);

  EXPECT_EQ(bytes1, bytes2);

  Crypto::GetRandomBytes(bytes1);
  EXPECT_NE(bytes1, bytes2);

  bytes2 = bytes1;
  EXPECT_EQ(bytes1, bytes2);

  Crypto::GetRandomBytes(bytes1);
  EXPECT_NE(bytes1, bytes2);

  Crypto::GetRandomBytes(bytes1);
  Crypto::GetRandomBytes(bytes2);
  EXPECT_NE(bytes1, bytes2);
}

TEST(KeyDeriverTest, Basic) {
  const std::string passphrase = "foobar";
  const std::string passphrase_2 = "something_else";

  bytes_t salt(32, 0);
  bytes_t key(32, 0);

  bytes_t other_salt(32, 1);
  bytes_t other_key(32, 1);

  Crypto::GetRandomBytes(salt);
  Crypto::GetRandomBytes(other_salt);
  EXPECT_TRUE(salt != other_salt);

  EXPECT_TRUE(Crypto::DeriveKey(passphrase, salt, key));
  EXPECT_TRUE(Crypto::DeriveKey(passphrase, salt, other_key));
  EXPECT_EQ(key, other_key);

  EXPECT_TRUE(Crypto::DeriveKey(passphrase, other_salt, other_key));
  EXPECT_NE(key, other_key);

  EXPECT_TRUE(Crypto::DeriveKey(passphrase_2, salt, other_key));
  EXPECT_NE(key, other_key);
}

TEST(KeyDeriverTest, BadInput) {
  const std::string passphrase;
  const std::string passphrase_2 = "something_else";

  bytes_t salt;
  bytes_t key;
  bytes_t other_salt(32, 1);
  bytes_t other_key(32, 1);

  EXPECT_FALSE(Crypto::DeriveKey(passphrase, salt, key));
  EXPECT_FALSE(Crypto::DeriveKey(passphrase, salt, other_key));
  EXPECT_FALSE(Crypto::DeriveKey(passphrase, other_salt, key));
}
