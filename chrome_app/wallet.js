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

// The Wallet model holds master keys and addresses that make up a
// BIP-0032 HD wallet.
function Wallet(credentials) {
  this.credentials = credentials;

  // ephemeral
  this.clearEphemeral = function() {
    this.ephemeral = {};
    this.ephemeral.accounts = {};
    this.ephemeral.extendedPrivateBase58 = null;
  };
  this.clearEphemeral();

  // storable
  this.clearStorable = function() {
    this.storable = {};
    this.storable.accounts = [];
    this.storable.extendedPrivateBase58Encrypted = null;
    this.storable.extendedPublicBase58 = null;
    this.storable.fingerprint = null;
    this.storable.nextAccount = 0;
  };
  this.clearStorable();

  this.unlock = function(passphrase, relockCallback, callback) {
    this.credentials.generateAndCacheKeys(
      passphrase,
      relockCallback,
      callback);
  };

  this.lock = function() {
    this.credentials.clearCachedKeys();
  };

  this.isKeySet = function() {
    return !!this.storable.extendedPublicBase58;
  };

  this.decryptSecrets = function(callback) {
    if (!this.credentials.isKeyAvailable()) {
      console.log("can't decrypt without key");
      callback.call(this, false);
      return;
    }
    if (this.storable.extendedPrivateBase58Encrypted) {
      this.credentials.decrypt(
        this.storable.extendedPrivateBase58Encrypted,
        function(item) {
          this.ephemeral.extendedPrivateBase58 = item;
          callback.call(this, !!item);
        }.bind(this));
    } else {
      // Callers are depending on this method to be asynchonous.
      window.setTimeout(callback.bind(this, true), 0);
    }
  };

  this.getNextAccountNumber = function() {
    return this.storable.nextAccount;
  };

  this.getExtendedPrivateBase58 = function() {
    if (!this.credentials.isKeyAvailable()) {
      console.log("warning: getPrivateBase58 with locked wallet");
    }
    return this.ephemeral.extendedPrivateBase58;
  };

  this.isExtendedPrivateSet = function() {
    return !!this.storable.extendedPrivateBase58Encrypted;
  };

  this.getExtendedPublicBase58 = function() {
    return this.storable.extendedPublicBase58;
  };

  this.getFingerprint = function() {
    return this.storable.fingerprint;
  };

  this.createRandomMasterKey = function(callback) {
    var message = {
      'command': 'create-node'
    };
    postMessageWithCallback(message, function(response) {
      this._setMasterKey(response.ext_pub_b58,
                         response.ext_prv_b58,
                         response.fingerprint,
                         callback.bind(this));
    }.bind(this));
  };

  this.importMasterKey = function(seed, callback) {
    var message = {
      'command': 'get-node',
      'seed': seed
    };
    postMessageWithCallback(message, function(response) {
      if (response.ext_pub_b58) {
        this._setMasterKey(response.ext_pub_b58,
                           response.ext_prv_b58,
                           response.fingerprint,
                           callback.bind(this, true));
      } else {
        callback.call(this, false);
      }
    }.bind(this));
  };

  this._setMasterKey = function(extendedPublicBase58,
                                extendedPrivateBase58,
                                fingerprint,
                                callback) {
    if (!this.credentials.isKeyAvailable()) {
      console.log("can't set master key; wallet is locked");
      callback.call(this);
      return;
    }
    if (extendedPrivateBase58) {
      console.log("calling encrypt");
      this.credentials.encrypt(extendedPrivateBase58, function(itemEncrypted) {
        this.clearEphemeral();
        this.clearStorable();
        this.ephemeral.extendedPrivateBase58 = extendedPrivateBase58;

        this.storable.extendedPrivateBase58Encrypted = itemEncrypted;
        this.storable.extendedPublicBase58 = extendedPublicBase58;
        this.storable.fingerprint = fingerprint;
        callback.call(this);
      }.bind(this));
    } else {
      this.clearEphemeral();
      this.clearStorable();
      this.storable.extendedPublicBase58 = extendedPublicBase58;
      this.storable.fingerprint = fingerprint;
      callback.call(this);
    }
  };

  this.removeMasterKey = function() {
    if (!this.credentials.isKeyAvailable()) {
      console.log("can't remove master key; wallet is locked");
      return;
    }
    this.ephemeral.extendedPublicBase58 = null;
    this.storable.extendedPrivateBase58Encrypted = null;
    this.storable.extendedPublicBase58 = null;
    console.log('removeMasterKey');
  };

  this.deriveNextAccount = function(callback) {
    if (!this.credentials.isKeyAvailable()) {
      console.log("Can't derive account when wallet is unlocked");
      callback(this, false);
      return;
    }

    var message = {
      'command': 'get-node',
      'seed': this.extendedPrivateBase58,
      'path': "m/" + this.storable.nextAccount++ + "'"
    };
    postMessageWithCallback(message, function(response) {
      this.credentials.encrypt(
        response.ext_prv_b58,
        function(encryptedItem) {
          var newAccount = {};
          newAccount.path = message.path;
          newAccount.parentFingerprint = this.storable.fingerprint;
          newAccount.extendedPublicBase58 = response.ext_pub_b58;
          newAccount.fingerprint = response.fingerprint;
          newAccount.extendedPrivateBase58Encrypted = encryptedItem;
          this.storable.accounts.push(newAccount);
          callback(this, true);
        }.bind(this));
    }.bind(this));
  };

  this.getAccounts = function() {
    return this.storable.accounts;
  };

  this.getAccountCount = function() {
    return this.getAccounts().length;
  };

  this.hasMultipleAccounts = function() {
    return this.getAccountCount() > 1;
  };

  ////////////////////////////////////////////////

  this.salt = null;
  this.check = null;
  this.internalKeyEncrypted = null;
  this.accounts = [];

  this.needsAccountsRetrieval = false;
  this.accountChangeCounter = 0;

  // Used for a $watch in controller.
  this.digestMasterKey = function() {
    return this.extendedPublicBase58 + ':' + !! this.extendedPrivateBase58;
  };

  this.clearCachedKeys = function() {
    console.log("cached keys cleared");
    this.key = null;
    this.internalKey = null;
    this.extendedPrivateBase58 = null;
  };

  this.decrypt = function(item, callback) {
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

  this.cacheKeys = function(key,
                            internalKey,
                            cacheExpirationCallback,
                            callback) {
    // TODO(miket): make clear time a pref
    this.key = key;
    this.internalKey = internalKey;

    window.setTimeout(function() {
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

  this.setPassphrase = function(newPassphrase,
                                cacheExpirationCallback,
                                callback) {
    if (this.isPassphraseSet()) {
      if (!this.credentials.isKeyAvailable()) {
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

  this.retrieveAccounts = function(callback) {
    if (!this.credentials.isKeyAvailable()) {
      console.log("Can't add account when wallet is unlocked");
      callback(this, false);
      return;
    }
    this.accounts = [];
    this.needsAccountsRetrieval = false;
    var message = {
      'command': 'get-node',
      'seed': this.extendedPrivateBase58,
      'path': "m/0'"  // Get the 0th (prime) account of master key
    };
    postMessageWithCallback(message, function(response) {
      var account = {};
      account.extendedPublicBase58 = response.ext_pub_b58;
      account.extendedPrivateBase58 = response.ext_prv_b58;
      account.fingerprint = response.fingerprint;

      var message = {};
      message.command = 'encrypt-item';
      message.item = account.extendedPrivateBase58;
      message.internal_key = this.internalKey;

      postMessageWithCallback(message, function(response) {
        account.extendedPrivateBase58Encrypted = response.item_encrypted;
        this.accounts.push(account);
        this.accountChangeCounter++;
        callback(this, true);
      }.bind(this));
    }.bind(this));
  };

  this.accountXprvIfAvailable = function(index) {
    if (this.accounts.length <= index) {
      return null;
    }
    var account = this.accounts[index];
    if (account.extendedPrivateBase58) {
      return account.extendedPrivateBase58;
    } else {
      return account.extendedPublicBase58;
    }
  };

  this.load = function(callback) {
    loadStorage2('wallet', function(items) {
      if (items) {
        this.storable = items;
      } else {
        this.clearStorable();
      }
      callback.call(this);
    }.bind(this));
  };

  this.save = function() {
    saveStorage2('wallet', this.storable);
  };
}
