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

/**
 * @constructor
 */
function Credentials() {
  this.init = function() {
    this.check = undefined;
    this.ephemeralKeyEncrypted = undefined;
    this.salt = undefined;

    this.isLocked = true;
  };
  this.init();

  this.toStorableObject = function() {
    var o = {};
    o["check"] = this.check;
    o["ekey_enc"] = this.ephemeralKeyEncrypted;
    o["salt"] = this.salt;
    return o;
  };

  this.loadStorableObject = function(o) {
    return new Promise(function(resolve, reject) {
      this.init();
      this.check = o["check"];
      this.ephemeralKeyEncrypted = o["ekey_enc"];
      this.salt = o["salt"];
      this.loadCredentials().then(resolve);
    }.bind(this));
  };

  this.isPassphraseSet = function() {
    return !!this.check;
  };

  // TODO(miket): rename to just isLocked (& avoid collision)
  this.isWalletLocked = function() {
    return this.isLocked;
  };

  this.setPassphrase = function(newPassphrase,
                                relockCallbackVoid) {
    return new Promise(function(resolve, reject) {
      if (this.isPassphraseSet() && this.isWalletLocked()) {
        reject("passphrase set/wallet is unlocked; can't set passphrase");
        return;
      }

      var success = function(response) {
        this.check = response.check;
        this.ephemeralKeyEncrypted = response.ekey_enc;
        this.salt = response.salt;
        this.setRelockTimeout(relockCallbackVoid);
        resolve();
      };

      var params = {
        'new_passphrase': newPassphrase
      };
      postRPC('set-passphrase', params).then(success.bind(this), reject);
    }.bind(this));
  };

  this.loadCredentials = function() {
    return new Promise(function(resolve, reject) {
      var params = {
        'check': this.check,
        'ekey_enc': this.ephemeralKeyEncrypted,
        'salt': this.salt
      };
      postRPC('set-credentials', params).then(resolve);
    }.bind(this));
  };

  this.clearRelockTimeout = function() {
    if (this.relockTimeoutId) {
      window.clearTimeout(this.relockTimeoutId);
      this.relockTimeoutId = undefined;
    }
  };

  this.setRelockTimeout = function(callbackVoid) {
    this.clearRelockTimeout();
    this.isLocked = false;
    this.relockTimeoutId = window.setTimeout(function() {
      this.lock(callbackVoid);
    }.bind(this), 1000 * 60 * 1);
  };

  this.lock = function() {
    return new Promise(function(resolve, reject) {
      this.isLocked = true;
      postRPC('lock', {}).then(resolve);
    }.bind(this));
  };

  this.unlock = function(passphrase, relockCallbackVoid) {
    return new Promise(function(resolve, reject) {
      postRPC('unlock',
              {'passphrase': passphrase}).then(function(response) {
                if (response.success) {
                  this.setRelockTimeout(relockCallbackVoid);
                  resolve();
                } else {
                  reject();
                }
              }.bind(this), reject);
    }.bind(this));
  };

  this.STORAGE_NAME = 'credentials';
  this.load = function() {
    return new Promise(function(resolve, reject) {
      var success = function(response) {
        if (response) {
          this.loadStorableObject(response).then(resolve);
        } else {
          this.init();
          resolve();
        }
      };
      var failure = function(response) {
        reject(response);
      };
      loadStorage(this.STORAGE_NAME).then(success.bind(this),
                                          failure.bind(this));
    }.bind(this));
  };

  this.save = function() {
    saveStorage(this.STORAGE_NAME, this.toStorableObject());
  };
}
