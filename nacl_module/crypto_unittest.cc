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


TEST(EncryptionTest, Basic) {
  const std::string passphrase_1 = "foo";
  const std::string passphrase_2 = "something_else";
  //                                    12345678901234567890123456789012
  const std::string plaintext_string = "This is a test!!This is a test!!";
  const bytes_t plaintext(plaintext_string.c_str(),
                          plaintext_string.c_str() + plaintext_string.size());

  bytes_t salt_1(32, 0);
  bytes_t salt_2(32, 0);
  bytes_t key_1(32, 0);
  bytes_t key_2(32, 0);

  Crypto::GetRandomBytes(salt_1);
  Crypto::DeriveKey(passphrase_1, salt_1, key_1);
  Crypto::GetRandomBytes(salt_2);
  Crypto::DeriveKey(passphrase_2, salt_2, key_2);

  // Do we get back what we put in?
  bytes_t ciphertext_1;
  EXPECT_TRUE(Crypto::Encrypt(key_1, plaintext, ciphertext_1));
  bytes_t plaintext_output_1;
  EXPECT_TRUE(Crypto::Decrypt(key_1, ciphertext_1, plaintext_output_1));
  EXPECT_EQ(plaintext_output_1, plaintext);

  // Different key: ciphertext different?
  bytes_t ciphertext_2;
  EXPECT_TRUE(Crypto::Encrypt(key_2, plaintext, ciphertext_2));
  EXPECT_NE(ciphertext_2, ciphertext_1);

  // Different key: decryption garbled?
  bytes_t plaintext_output_2;
  EXPECT_TRUE(Crypto::Decrypt(key_2, ciphertext_1, plaintext_output_2));
  EXPECT_NE(plaintext_output_2, plaintext);

  // Same salt, different key: decryption garbled?
  plaintext_output_2 = plaintext;
  EXPECT_EQ(plaintext_output_2, plaintext_output_1);
  Crypto::DeriveKey(passphrase_2, salt_1, key_2);
  EXPECT_TRUE(Crypto::Decrypt(key_2, ciphertext_1, plaintext_output_2));
  EXPECT_NE(plaintext_output_2, plaintext);
}
