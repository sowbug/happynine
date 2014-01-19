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

  this.init = function() {
    this.accounts = [];
    this.extendedPrivateBase58 = null;
    this.extendedPrivateBase58Encrypted = null;
    this.extendedPublicBase58 = null;
    this.fingerprint = null;
    this.nextAccount = 0;
  };
  this.init();

  this.toStorableObject = function() {
    var o = {};
    o.accounts = [];
    for (var i in this.accounts) {
      o.accounts.push(this.accounts[i].toStorableObject());
    }
    o.extendedPrivateBase58Encrypted = this.extendedPrivateBase58Encrypted;
    o.extendedPublicBase58 = this.extendedPublicBase58;
    o.fingerprint = this.fingerprint;
    o.nextAccount = this.nextAccount;

    return o;
  };

  this.loadStorableObject = function(o) {
    this.init();
    this.accounts = [];
    for (var i in o.accounts) {
      this.accounts.push(Account.fromStorableObject(o.accounts[i]));
    }
    this.extendedPrivateBase58Encrypted = o.extendedPrivateBase58Encrypted;
    this.extendedPublicBase58 = o.extendedPublicBase58;
    this.fingerprint = o.fingerprint;
    this.nextAccount = o.nextAccount;
  }

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
    return !!this.extendedPublicBase58;
  };

  this.decryptSecrets = function(callback) {
    if (!this.credentials.isKeyAvailable()) {
      console.log("can't decrypt without key");
      callback.call(this, false);
      return;
    }
    if (this.extendedPrivateBase58Encrypted) {
      this.credentials.decrypt(
        this.extendedPrivateBase58Encrypted,
        function(item) {
          this.extendedPrivateBase58 = item;
          callback.call(this, !!item);
        }.bind(this));
    } else {
      // Callers are depending on this method to be asynchonous.
      window.setTimeout(callback.bind(this, true), 0);
    }
  };

  this.getNextAccountNumber = function() {
    return this.nextAccount;
  };

  this.getExtendedPrivateBase58 = function() {
    if (!this.credentials.isKeyAvailable()) {
      console.log("warning: getPrivateBase58 with locked wallet");
    }
    return this.extendedPrivateBase58;
  };

  this.isExtendedPrivateSet = function() {
    return !!this.extendedPrivateBase58Encrypted;
  };

  this.getExtendedPublicBase58 = function() {
    return this.extendedPublicBase58;
  };

  this.getFingerprint = function() {
    return this.fingerprint;
  };

  this.createRandomMasterKey = function(callback) {
    postRPCWithCallback('create-node', {}, function(response) {
      this._setMasterKey(response.ext_pub_b58,
                         response.ext_prv_b58,
                         response.fingerprint,
                         callback.bind(this));
    }.bind(this));
  };

  this.importMasterKey = function(ext_b58, callback) {
    var params = {
      'ext_b58': ext_b58
    };
    postRPCWithCallback('get-node', params, function(response) {
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
      this.credentials.encrypt(extendedPrivateBase58, function(itemEncrypted) {
        this.init();
        this.extendedPrivateBase58 = extendedPrivateBase58;

        this.extendedPrivateBase58Encrypted = itemEncrypted;
        this.extendedPublicBase58 = extendedPublicBase58;
        this.fingerprint = fingerprint;
        callback.call(this);
      }.bind(this));
    } else {
      this.init();
      this.extendedPublicBase58 = extendedPublicBase58;
      this.fingerprint = fingerprint;
      callback.call(this);
    }
  };

  this.removeMasterKey = function() {
    if (!this.credentials.isKeyAvailable()) {
      console.log("can't remove master key; wallet is locked");
      return;
    }
    this.extendedPublicBase58 = null;
    this.extendedPrivateBase58Encrypted = null;
    this.extendedPublicBase58 = null;
  };

  this.deriveNextAccount = function(isWatchOnly, callback) {
    if (!this.credentials.isKeyAvailable()) {
      console.log("Can't derive account when wallet is unlocked");
      // TODO(miket): all these synchronous callbacks need to be async
      callback(this, false);
      return;
    }

    var params = {
      'ext_b58': this.extendedPrivateBase58,
      'path': "m/" + this.nextAccount++ + "'"
    };
    postRPCWithCallback('get-node', params, function(response) {
      if (isWatchOnly) {
        this.accounts.push(Account.fromStorableObject({
          'hexId': response.hex_id,
          'fingerprint': response.fingerprint,
          'parentFingerprint': this.fingerprint,
          'path': params.path,
          'extendedPublicBase58': response.ext_pub_b58,
          'extendedPrivateBase58Encrypted': undefined
        }));
        callback(this, true);
      } else {
        this.credentials.encrypt(
          response.ext_prv_b58,
          function(encryptedItem) {
            this.accounts.push(Account.fromStorableObject({
              'hexId': response.hex_id,
              'fingerprint': response.fingerprint,
              'parentFingerprint': this.fingerprint,
              'path': params.path,
              'extendedPublicBase58': response.ext_pub_b58,
              'extendedPrivateBase58Encrypted': encryptedItem
            }));
            callback(this, true);
          }.bind(this));
      }
    }.bind(this));
  };

  this.getAccounts = function() {
    return this.accounts;
  };

  this.getAccountCount = function() {
    return this.getAccounts().length;
  };

  this.hasMultipleAccounts = function() {
    return this.getAccountCount() > 1;
  };

  this.load = function(callback) {
    loadStorage2('wallet', function(object) {
      if (object) {
        this.loadStorableObject(object);
      } else {
        this.init();
      }
      callback.call(this);
    }.bind(this));
  };

  this.save = function() {
    saveStorage2('wallet', this.toStorableObject());
  };
}
