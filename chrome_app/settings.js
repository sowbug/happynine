// Copyright 2014 Mike Tsao <mike@sowbug.com>

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

var settings = new Settings();
settings.load();

function Settings() {
  var thisSettings = this;
  this.availableUnits = {
    "btc": "BTC",
    "mbtc": "mBTC",
    "ubtc": "uBTC",
    "sats": "Satoshis"
  };
  this.units = "mbtc";
  this.passphraseSalt = null;

  // These are here only for data bindings. Never serialize!
  this.confirmPassphrase = null;
  this.currentPassphrase = null;
  this.masterKey = null;
  this.newPassphrase = null;
  this.passphraseKey = null;

  var SERIALIZED_FIELDS = [
    'masterKeyEncrypted',
    'passphraseCheck',
    'passphraseSalt',
    'units',
  ];

  this.load = function() {
    if (chrome && chrome.storage) {
      chrome.storage.local.get("settings", function(items) {
        var loadedSettings = items.settings;
        if (loadedSettings) {
          console.log("loaded");
          console.log(loadedSettings);
          for (var field in SERIALIZED_FIELDS) {
            var field_name = SERIALIZED_FIELDS[field];
            thisSettings[field_name] = loadedSettings[field_name];
          }
          thisSettings.$scope.$apply();
        }
      });
    }
  };

  this.save = function() {
    if (chrome && chrome.storage) {
      var toSave = {};
      for (var field in SERIALIZED_FIELDS) {
        var field_name = SERIALIZED_FIELDS[field];
        toSave[field_name] = thisSettings[field_name];
      }
      console.log("saving");
      console.log(toSave);
      chrome.storage.local.set({'settings': toSave}, function() {});
    }
  };

  this.clear = function() {
    if (chrome && chrome.storage) {
      chrome.storage.local.clear(function() {
        console.log("settings cleared");
      });
    }
  };

  this.isPassphraseSet = function() {
    return !!thisSettings.passphraseSalt;
  };

  this.isPassphraseCached = function() {
    return !!thisSettings.passphraseKey;
  };

  // echo -n "Bitcoin Wallet Copyright 2014 Mike Tsao." | sha256sum
  var PASSPHRASE_CHECK_HEX =
    "d961579ab5f288d5424816577a092435f32223347a562992811e71c31e2d12ea";

  this.cachePassphraseKey = function(key) {
    // TODO(miket): key should be cleared after something like 5 min.
    console.log("caching passphrase key");
    thisSettings.passphraseKey = key;
    window.setTimeout(function() {
      thisSettings.passphraseKey = null;
      thisSettings.$scope.$apply();
      console.log("cleared passphrase key");
    }, 1000 * 60 * 1);
  }

  this.setPassphrase = function() {
    if (thisSettings.isPassphraseSet()) {
      console.log("can't set passphrase -- already set");
    }

    if (thisSettings.newPassphrase &&
        thisSettings.newPassphrase.length > 0 &&
        thisSettings.newPassphrase == thisSettings.confirmPassphrase) {
      var message = {};
      message.command = 'derive-key';
      message.passphrase = thisSettings.newPassphrase;

      // We don't want these items lurking in the DOM.
      thisSettings.currentPassphrase = null;
      thisSettings.newPassphrase = null;
      thisSettings.confirmPassphrase = null;

      postMessageWithCallback(message, function(response) {
        thisSettings.cachePassphraseKey(response.key);
        thisSettings.passphraseSalt = response.salt;

        var message = {};
        message.command = 'encrypt-bytes';
        message.plaintext_hex = PASSPHRASE_CHECK_HEX;
        message.key = thisSettings.passphraseKey;

        postMessageWithCallback(message, function(response) {
          console.log("passphraseCheck updated");
          thisSettings.passphraseCheck = response.ciphertext_hex;
          if (thisSettings.masterKey) {
            thisSettings.setMasterKey();
          }
          thisSettings.$scope.$apply();
          thisSettings.save();
        });
      });
    }
  };

  this.changePassphrase = function() {
    thisSettings.verifyPassphrase(function(verified) {
      if (verified) {
        this.removePassphrase(function() {
          this.setPassphrase();
        });
      } else {
        console.log("change failed -- not verified");
        // nope
      }
    });
  };

  this.verifyPassphrase = function(callback) {
    var message = {};
    message.command = 'derive-key';
    message.passphrase =  thisSettings.currentPassphrase;
    message.salt =  thisSettings.passphraseSalt;

    postMessageWithCallback(message, function(response) {
      var message = {};
      message.command = 'encrypt-bytes';
      message.plaintext_hex = PASSPHRASE_CHECK_HEX;
      message.key = response.key;

      postMessageWithCallback(message, function(response) {
        console.log("verify: " +
                    (response.ciphertext_hex == thisSettings.passphraseCheck));
        callback.call(
          response.ciphertext_hex == thisSettings.passphraseCheck);
      });
    });
  };

  this.removePassphrase = function(callback) {
    if (!thisSettings.isPassphraseCached()) {
      callback.call(false);
    }
    if (!thisSettings.masterKeyEncrypted) {
      // TODO(miket): success or failure?
      callback.call(true);
    }

    // decrypt any encrypted stuff
    var message = {};
    message.command = 'decrypt-bytes';
    message.ciphertext_hex = thisSettings.masterKeyEncrypted;
    message.key = thisSettings.passphraseKey;
    
    postMessageWithCallback(message, function(response) {
      console.log("master key is now cached in plaintext");
      thisSettings.masterKey = response.plaintext_hex;
      thisSettings.masterKeyEncrypted = null;

      // clear current passphrase
      thisSettings.passphraseKey = null;
      thisSettings.passphraseSalt = null;
      thisSettings.passphraseCheck = null;
      thisSettings.$scope.$apply();

      callback.call(true);
    });
  };

  this.setMasterKey = function() {
    if (!thisSettings.isPassphraseCached()) {
      console.log("can't set master key; passphrase is not cached");
      return false;
    }
    var message = {};
    message.command = 'encrypt-bytes';
    message.plaintext_hex = thisSettings.masterKey;
    message.key = thisSettings.passphraseKey;

    postMessageWithCallback(message, function(response) {
      console.log("master key is now set.");
      thisSettings.masterKeyEncrypted = response.ciphertext_hex;
      thisSettings.masterKey = null;
      thisSettings.save();
    });
  };

  this.satoshiToUnit = function(satoshis) {
    switch (thisSettings.units) {
    case "btc":
      return satoshis / 100000000;
    case "mbtc":
       return satoshis / 100000;
    case "ubtc":
       return satoshis / 100;
    default:
      return satoshis;
    }
  };

  this.unitLabel = function() {
    return thisSettings.availableUnits[thisSettings.units];
  };

  this.clearEverything = function() {
    thisSettings.clear();
  };
}
