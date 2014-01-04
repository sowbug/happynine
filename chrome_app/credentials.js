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
function Credentials(settings) {
  this.settings = settings;

  // key is the passphrase key -- the thing that you get when you have
  // the passphrase and salt and run them through PBKDF2.
  this.key = null;

  // internalKey is the key used to encrypt everything. The passphrase
  // key encrypts it. The reason to have both a key and internalKey is
  // so that when the user changes his passphrase, we don't have to
  // re-encrypt everything in the world.
  this.internalKey = null;

  this.storable = {};
  this.storable.salt = null;
  this.storable.check = null;
  this.storable.internalKeyEncrypted = null;
  this.storable.masterKeyEncrypted = null;
  this.storable.masterKeyPublic = null;

  function key() { return key; }
  function salt() { return storable.salt; }
  function check() { return storable.check; }
  function internalKey() { return internalKey; }
  function internalKeyEncrypted() { return storable.internalKeyEncrypted; }

  this.isPassphraseSet = function() {
    return !!this.storable.internalKeyEncrypted;
  }.bind(this);

  // locked means either that a passphrase is set and the key is not cached,
  // or that there is no passphrase set.
  this.isWalletUnlocked = function() {
    return isPassphraseSet() && !!passphraseKey;
  }.bind(this);

  this.clearCachedKeys = function() {
    console.log("cached keys cleared");
    key = null;
    internalKey = null;
  }.bind(this);

  this.cacheKeys = function(key, internalKey, callback) {
    // TODO(miket): make clear time a pref
    console.log("caching keys");
    this.key = key;
    this.internalKey = internalKey;

    var t = this;
    window.setTimeout(function() {
      t.clearCachedKeys();
      if (callback)
        callback.call();
    }, 1000 * 60 * 1);
  }.bind(this);

  function handleSetPassphraseResponse(response) {
    this.cacheKeys(response.key, response.internal_key);
    this.storable.salt = response.salt;
    this.storable.check = response.check;
    this.storable.internalKeyEncrypted = response.internal_key_encrypted;
    this.save();
  };

  this.setPassphrase = function(newPassphrase) {
    if (this.isPassphraseSet()) {
      if (!this.isWalletUnlocked()) {
        console.log("Can't change passphrase because wallet is locked.");
        return;
      }
    } else {
      if (this.storable.internalKeyEncrypted) {
        console.log("PROBLEM: internalKeyEncrypted set but no passphrase");
        return;
      }
      if (this.storable.masterKeyEncrypted) {
        console.log("PROBLEM: masterKeyEncrypted set but no passphrase");
        return;
      }
    }

    var message = {};
    message.command = 'set-passphrase';
    message.new_passphrase = newPassphrase;
    if (this.isPassphraseSet()) {
      if (message.key) {
        message.key = key;
        message.check = check;
        message.internal_key_encrypted = this.storable.internalKeyEncrypted;
      }
    }
    postMessageWithCallback(message, this.handleSetPassphraseResponse);

  }.bind(this);

  function setMasterKey(newMasterKey) {
    if (!isWalletUnlocked()) {
      console.log("can't set master key; wallet is locked");
      return;
    }
    storable.masterKeyPublic = newMasterKey.xpub;
    if (newMasterKey.xprv) {
      var message = {};
      message.command = 'encrypt-item';
      message.item = newMasterKey.xprv;
      message.internal_key = internalKey;

      var t = this;
      postMessageWithCallback(message, function(response) {
        console.log("master key is now set.");
        t.storable.masterKeyEncrypted = response.item_encrypted;
        t.save();
      });
    } else {
      storable.masterKeyEncrypted = null;
      save();
    }
  };

  function load(callback) {
    loadStorage('credentials', storable, callback);
  };

  function save() {
    saveStorage('credentials', storable);
  };
}
