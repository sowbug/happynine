// Copyright 2014 Mike Tsao <mike@sowbug.com>

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>
#include <fstream>
#include <string>

#include "api.h"
#include "gtest/gtest.h"
#include "jsoncpp/json/reader.h"
#include "jsoncpp/json/writer.h"
#include "types.h"

const std::string BAD_HEX =
  "dcc6f85d0733485b7d50a5faabeec8b330a35dbe9bca4ff227d147adf151823e";

TEST(SetPassphraseTest, Basic) {
  Json::Value request;
  Json::Value response;
  API api;

  // Set new passphrase
  request["new_passphrase"] = "foobarbaz";
  EXPECT_TRUE(api.HandleSetPassphrase(request, response));
  const bytes_t salt(unhexlify(response["salt"].asString()));
  const bytes_t key(unhexlify(response["key"].asString()));
  const bytes_t check(unhexlify(response["check"].asString()));
  const bytes_t internal_key(unhexlify(response["internal_key"]
                                       .asString()));
  const bytes_t
    internal_key_encrypted(unhexlify(response["internal_key_encrypted"]
                                     .asString()));

  // Unlock wallet with correct passphrase
  request = Json::Value();
  response = Json::Value();
  request["salt"] = to_hex(salt);
  request["check"] = to_hex(check);
  request["passphrase"] = "foobarbaz";
  request["internal_key_encrypted"] = to_hex(internal_key_encrypted);
  EXPECT_TRUE(api.HandleUnlockWallet(request, response));
  EXPECT_EQ(key, unhexlify(response["key"].asString()));
  EXPECT_EQ(internal_key, unhexlify(response["internal_key"].asString()));

  // Unlock wallet with wrong passphrase
  Json::Value request_wrong_passphrase(request);
  response = Json::Value();
  request_wrong_passphrase["passphrase"] = "wrong";
  EXPECT_TRUE(api.HandleUnlockWallet(request_wrong_passphrase, response));
  EXPECT_EQ(-2, response["error_code"].asInt());

  // Unlock wallet with wrong salt
  Json::Value request_wrong_salt(request);
  response = Json::Value();
  request_wrong_salt["salt"] = BAD_HEX;
  response = Json::Value();
  EXPECT_TRUE(api.HandleUnlockWallet(request_wrong_salt, response));
  EXPECT_EQ(-2, response["error_code"].asInt());

  // Unlock wallet with wrong check
  Json::Value request_wrong_check(request);
  response = Json::Value();
  request_wrong_check["check"] = BAD_HEX;
  response = Json::Value();
  EXPECT_TRUE(api.HandleUnlockWallet(request_wrong_check, response));
  EXPECT_EQ(-2, response["error_code"].asInt());

  // Encrypt something
  request = Json::Value();
  response = Json::Value();
  request["item"] = "plaintext";
  request["internal_key"] = to_hex(internal_key);
  EXPECT_TRUE(api.HandleEncryptItem(request, response));
  const bytes_t
    item_encrypted(unhexlify(response["item_encrypted"].asString()));

  // Decrypt with wrong key
  request = Json::Value();
  response = Json::Value();
  request["item_encrypted"] = to_hex(item_encrypted);
  request["internal_key"] = BAD_HEX;
  EXPECT_TRUE(api.HandleDecryptItem(request, response));
  EXPECT_EQ(-1, response["error_code"].asInt());

  // Decrypt with right key
  request = Json::Value();
  response = Json::Value();
  request["item_encrypted"] = to_hex(item_encrypted);
  request["internal_key"] = to_hex(internal_key);
  EXPECT_TRUE(api.HandleDecryptItem(request, response));
  EXPECT_EQ("plaintext", response["item"].asString());

  // Change passphrase with wrong credentials
  request = Json::Value();
  response = Json::Value();
  request["key"] = BAD_HEX;
  request["check"] = to_hex(check);
  request["internal_key_encrypted"] = to_hex(internal_key_encrypted);
  request["new_passphrase"] = "New Passphrase";
  EXPECT_TRUE(api.HandleSetPassphrase(request, response));
  EXPECT_EQ(-2, response["error_code"].asInt());

  // Change passphrase with good credentials
  request = Json::Value();
  response = Json::Value();
  request["key"] = to_hex(key);
  request["check"] = to_hex(check);
  request["internal_key_encrypted"] = to_hex(internal_key_encrypted);
  request["new_passphrase"] = "New Passphrase";
  EXPECT_TRUE(api.HandleSetPassphrase(request, response));
  // Internal key should never change.
  EXPECT_EQ(internal_key, unhexlify(response["internal_key"].asString()));
  // Other items should.
  EXPECT_NE(salt, unhexlify(response["salt"].asString()));
  EXPECT_NE(key, unhexlify(response["key"].asString()));
  EXPECT_NE(check, unhexlify(response["check"].asString()));
  EXPECT_NE(internal_key_encrypted,
            unhexlify(response["internal_key_encrypted"].asString()));
  const bytes_t new_salt(unhexlify(response["salt"].asString()));
  const bytes_t new_key(unhexlify(response["key"].asString()));
  const bytes_t new_check(unhexlify(response["check"].asString()));
  const bytes_t new_internal_key(unhexlify(response["internal_key"]
                                           .asString()));
  const bytes_t
    new_internal_key_encrypted(unhexlify(response["internal_key_encrypted"]
                                         .asString()));

  // Unlock wallet with new passphrase
  request = Json::Value();
  response = Json::Value();
  request["salt"] = to_hex(new_salt);
  request["check"] = to_hex(new_check);
  request["passphrase"] = "New Passphrase";
  request["internal_key_encrypted"] = to_hex(new_internal_key_encrypted);
  EXPECT_TRUE(api.HandleUnlockWallet(request, response));
  EXPECT_EQ(new_internal_key, unhexlify(response["internal_key"].asString()));
}
