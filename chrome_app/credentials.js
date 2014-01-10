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

'use strict';

// The Credentials model keeps track of secrets that lock/unlock and
// encrypt/decrypt things.
function Credentials() {
  var STORABLE = ['salt',
                  'check',
                  'internalKeyEncrypted',
                 ];

  // key is the passphrase key -- the thing that you get when you have
  // the passphrase and salt and run them through PBKDF2.
  this.key = null;

  // internalKey is the key used to encrypt everything. The passphrase
  // key encrypts it. The reason to have both a key and internalKey is
  // so that when the user changes his passphrase, we don't have to
  // re-encrypt everything in the world.
  this.internalKey = null;

  // storable
  this.salt = null;
  this.check = null;
  this.internalKeyEncrypted = null;
  this.accounts = [];

  this.needsAccountsRetrieval = false;
  this.accountChangeCounter = 0;

  this.isPassphraseSet = function() {
    return !!this.internalKeyEncrypted;
  };

  this.isKeyAvailable = function() {
    return !!this.key;
  }

  this.generateAndCacheKeys = function(passphrase, relockCallback, callback) {
    var message = {};
    message.command = 'unlock-wallet';
    message.salt = this.salt;
    message.check = this.check;
    message.internal_key_encrypted = this.internalKeyEncrypted;
    message.passphrase = passphrase;

    postMessageWithCallback(message, function(response) {
      if (response.key) {
        this.cacheKeys(response.key,
                       response.internal_key,
                       relockCallback,
                       callback.bind(this, true));
        console.log("cool!");
      } else {
        callback.call(this, false);
      }
    }.bind(this));
  };

  this.cacheKeys = function(key,
                            internalKey,
                            cacheExpirationCallback,
                            callback) {
    // TODO(miket): make clear time a pref
    this.key = key;
    this.internalKey = internalKey;

    if (this.cacheTimeoutId) {
      window.clearTimeout(this.cacheTimeoutId);
    }
    this.cacheTimeoutId = window.setTimeout(function() {
      this.clearCachedKeys();
      if (cacheExpirationCallback)
        cacheExpirationCallback.call(this);
    }.bind(this), 1000 * 60 * 1);

    if (this.extendedPrivateBase58Encrypted) {
      this.decrypt(this.extendedPrivateBase58Encrypted, function(item) {
        if (item) {
          this.extendedPrivateBase58 = item;
        }
        callback.call(this);
      }.bind(this));
    } else {
      callback.call(this);
    }
  };

  this.clearCachedKeys = function() {
    if (this.cacheTimeoutId) {
      window.clearTimeout(this.cacheTimeoutId);
    }
    this.cacheTimeoutId = null;
    console.log("cached keys cleared");
    this.key = null;
    this.internalKey = null;
    this.extendedPrivateBase58 = null;
  };

  this.encrypt = function(item, callback) {
    if (!this.key) {
      console.log("no key available");
      callback.call(this);
      return;
    }
    var message = {};
    message.command = 'encrypt-item';
    message.item = item;
    message.internal_key = this.internalKey;

    postMessageWithCallback(message, function(response) {
      if (response.item_encrypted) {
        callback.call(this, response.item_encrypted);
      } else {
        callback.call(this);
      }
    }.bind(this));
  };

  this.decrypt = function(item, callback) {
    if (!this.key) {
      console.log("no key available");
      callback.call(this);
      return;
    }
    var message = {};
    message.command = 'decrypt-item';
    message.item_encrypted = item;
    message.internal_key = this.internalKey;

    postMessageWithCallback(message, function(response) {
      if (response.item) {
        callback.call(this, response.item);
      } else {
        callback.call(this);
      }
    }.bind(this));
  };

  this.setPassphrase = function(newPassphrase,
                                cacheExpirationCallback,
                                callback) {
    if (this.isPassphraseSet()) {
      if (!this.isWalletUnlocked()) {
        console.log("Can't change passphrase because wallet is locked.");
        callback.call(this, false);
        return;
      }
    } else {
      if (this.internalKeyEncrypted) {
        console.log("PROBLEM: internalKeyEncrypted set but no passphrase");
        callback.call(this, false);
        return;
      }
      if (this.extendedPrivateBase58Encrypted) {
        console.log(
          "PROBLEM: extendedPrivateBase58Encrypted set but no passphrase");
        callback.call(this, false);
        return;
      }
    }

    var message = {};
    message.command = 'set-passphrase';
    message.new_passphrase = newPassphrase;
    if (this.isPassphraseSet()) {
      if (this.key) {
        message.key = this.key;
        message.check = this.check;
        message.internal_key_encrypted = this.internalKeyEncrypted;
      }
    }
    postMessageWithCallback(message, function(response) {
      this.cacheKeys(response.key,
                     response.internal_key,
                     cacheExpirationCallback,
                     function() {
                       this.salt = response.salt;
                       this.check = response.check;
                       this.internalKeyEncrypted =
                         response.internal_key_encrypted;
                       callback.call(this, true);
                     }.bind(this));
    }.bind(this));
  };

  this.load = function(callback) {
    loadStorage('credentials', this, STORABLE, callback);
  };

  this.save = function() {
    saveStorage('credentials', this, STORABLE);
  };
}
