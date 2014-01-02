#include <iostream>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "key_deriver.h"
#include "rng.h"
#include "types.h"

TEST(KeyDeriverTest, Basic) {
  const std::string passphrase = "foobar";
  const std::string passphrase_2 = "something_else";

  bytes_t salt(32, 0);
  bytes_t key(32, 0);

  bytes_t other_salt(32, 1);
  bytes_t other_key(32, 1);

  RNG::GetRandomBytes(salt);
  RNG::GetRandomBytes(other_salt);
  EXPECT_TRUE(salt != other_salt);

  EXPECT_TRUE(KeyDeriver::Derive(passphrase, salt, key));
  EXPECT_TRUE(KeyDeriver::Derive(passphrase, salt, other_key));
  EXPECT_EQ(key, other_key);

  EXPECT_TRUE(KeyDeriver::Derive(passphrase, other_salt, other_key));
  EXPECT_NE(key, other_key);

  EXPECT_TRUE(KeyDeriver::Derive(passphrase_2, salt, other_key));
  EXPECT_NE(key, other_key);
}

TEST(KeyDeriverTest, BadInput) {
  const std::string passphrase;
  const std::string passphrase_2 = "something_else";

  bytes_t salt;
  bytes_t key;
  bytes_t other_salt(32, 1);
  bytes_t other_key(32, 1);

  EXPECT_FALSE(KeyDeriver::Derive(passphrase, salt, key));
  EXPECT_FALSE(KeyDeriver::Derive(passphrase, salt, other_key));
  EXPECT_FALSE(KeyDeriver::Derive(passphrase, other_salt, key));
}
