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

#include "credentials.h"
#include "gtest/gtest.h"

const std::string PP1 = "secret";
const std::string PP2 = "not-secret";

TEST(CredentialsTest, HappyPath) {
  bytes_t salt;
  bytes_t check;
  bytes_t encrypted_ephemeral_key;

  Credentials c;

  // Set it up.
  EXPECT_TRUE(c.isLocked());
  EXPECT_FALSE(c.isPassphraseSet());
  EXPECT_TRUE(c.SetPassphrase(PP1, salt, check, encrypted_ephemeral_key));
  EXPECT_EQ(32, salt.size());
  EXPECT_FALSE(c.isLocked());
  EXPECT_TRUE(c.isPassphraseSet());

  // Lock it. Does it stay locked?
  EXPECT_TRUE(c.Lock());
  EXPECT_TRUE(c.isLocked());

  // Unlock with wrong passphrase.
  EXPECT_FALSE(c.Unlock(PP2));
  EXPECT_TRUE(c.isLocked());

  // Unlock with right passphrase.
  EXPECT_TRUE(c.Unlock(PP1));
  EXPECT_FALSE(c.isLocked());

  // Could we get the key out?
  EXPECT_TRUE(c.ephemeral_key().size() != 0);

  EXPECT_TRUE(c.Lock());
  EXPECT_TRUE(c.isLocked());

  // Is key locked away again?
  EXPECT_TRUE(c.ephemeral_key().empty());

  // Change passphrase
  EXPECT_TRUE(c.Unlock(PP1));
  EXPECT_TRUE(c.SetPassphrase(PP2, salt, check, encrypted_ephemeral_key));
  EXPECT_FALSE(c.isLocked());
  EXPECT_TRUE(c.Lock());

  // Unlock with wrong passphrase.
  EXPECT_FALSE(c.Unlock(PP1));
  EXPECT_TRUE(c.isLocked());

  // Unlock with right passphrase.
  EXPECT_TRUE(c.Unlock(PP2));
  EXPECT_FALSE(c.isLocked());
}
