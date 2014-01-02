#include <iostream>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "key_deriver.h"
#include "types.h"

TEST(KeyDeriverTest, Basic) {
  const std::string passphrase = "foobar";
  const std::string passphrase_2 = "something_else";

  bytes_t key(32, 0);
  bytes_t salt(32, 0);
  KeyDeriver kd;

  bytes_t other_key(32, 1);
  bytes_t other_salt(32, 1);

  kd.Derive(passphrase, key, salt);
  EXPECT_TRUE(kd.Verify(passphrase, key, salt));
  EXPECT_FALSE(kd.Verify(passphrase_2, key, salt));
  EXPECT_FALSE(kd.Verify(passphrase, other_key, salt));
  EXPECT_FALSE(kd.Verify(passphrase, key, other_salt));
}

TEST(KeyDeriverTest, BadInput) {
  const std::string passphrase;
  const std::string passphrase_2 = "something_else";

  KeyDeriver kd;
  bytes_t key;
  bytes_t salt;
  bytes_t other_key(32, 1);
  bytes_t other_salt(32, 1);

  EXPECT_FALSE(kd.Derive(passphrase, key, salt));
  EXPECT_FALSE(kd.Verify(passphrase, key, salt));
  EXPECT_FALSE(kd.Verify(passphrase_2, key, salt));
  EXPECT_FALSE(kd.Verify(passphrase, other_key, salt));
  EXPECT_FALSE(kd.Verify(passphrase, key, other_salt));
}
