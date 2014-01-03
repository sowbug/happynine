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
  this.internalKey = null;
  this.masterKey = null;
  this.newPassphrase = null;
  this.passphraseKey = null;

  var SERIALIZED_FIELDS = [
    'masterKeyEncrypted',
    'internalKeyEncrypted',
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

  // locked means either that a passphrase is set and the key is not cached,
  // or that there is no passphrase set.
  this.isWalletUnlocked = function() {
    return thisSettings.isPassphraseSet() && thisSettings.passphraseKey;
  };

  this.lockWallet = function() {
    thisSettings.passphraseKey = null;
    thisSettings.internalKey = null;
    console.log("cleared passphrase key");
  };

  this.cachePassphraseKey = function(key, internalKey) {
    // TODO(miket): make clear time a pref
    console.log("caching passphrase key");
    thisSettings.passphraseKey = key;
    thisSettings.internalKey = internalKey;
    window.setTimeout(function() {
      thisSettings.lockWallet();
      thisSettings.$scope.$apply();
    }, 1000 * 60 * 1);
  }

  this.handleSetPassphraseResponse = function(response) {
    thisSettings.cachePassphraseKey(response.key, response.internal_key);
    thisSettings.passphraseSalt = response.salt;
    thisSettings.passphraseCheck = response.check;
    thisSettings.internalKeyEncrypted = response.internal_key_encrypted;
    thisSettings.$scope.$apply();
    thisSettings.save();
  };

  this.setPassphrase = function() {
    // TODO: angularjs can probably do this check for us
    if (!thisSettings.newPassphrase ||
        thisSettings.newPassphrase.length == 0 ||
        thisSettings.newPassphrase != thisSettings.confirmPassphrase) {
      console.log("new didn't match confirm");
      return;
    }

    if (thisSettings.isPassphraseSet()) {
      if (!thisSettings.isWalletUnlocked()) {
        console.log("Can't change passphrase because wallet is locked.");
        return;
      }
    } else {
      if (thisSettings.internalKeyEncrypted) {
        console.log("PROBLEM: internalKeyEncrypted set but no passphrase");
        return;
      }
      if (thisSettings.masterKeyEncrypted) {
        console.log("PROBLEM: masterKeyEncrypted set but no passphrase");
        return;
      }
    }

    var message = {};
    message.command = 'set-passphrase';
    message.new_passphrase = thisSettings.newPassphrase;
    if (thisSettings.isPassphraseSet()) {
      message.key = thisSettings.passphraseKey;
      message.check = thisSettings.passphraseCheck;
      message.internal_key_encrypted = thisSettings.internalKeyEncrypted;
    }

    // We don't want these items lurking in the DOM.
    thisSettings.newPassphrase = null;
    thisSettings.confirmPassphrase = null;

    postMessageWithCallback(message,
                            thisSettings.handleSetPassphraseResponse);
  };

  this.setMasterKey = function(masterKey) {
    if (!thisSettings.isWalletUnlocked()) {
      console.log("can't set master key; wallet is locked");
      return;
    }
    var message = {};
    message.command = 'encrypt-item';
    message.item = masterKey;
    message.internal_key = thisSettings.internalKey;

    postMessageWithCallback(message, function(response) {
      console.log("master key is now set.");
      thisSettings.masterKeyEncrypted = response.item_encrypted;
      thisSettings.save();
    });
  };

  this.unlockWallet = function() {
    var message = {};
    message.command = 'unlock-wallet';
    message.salt = thisSettings.passphraseSalt;
    message.check = thisSettings.passphraseCheck;
    message.internal_key_encrypted = thisSettings.internalKeyEncrypted;
    message.passphrase = thisSettings.newPassphrase;
    thisSettings.newPassphrase = null;

    postMessageWithCallback(message,function(response) {
      if (response.key) {
        $("#unlock-wallet-modal").modal('hide');
        thisSettings.cachePassphraseKey(response.key, response.internal_key);
        thisSettings.$scope.$apply();
      }
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
