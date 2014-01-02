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
  this.currentPassphrase = null;
  this.newPassphrase = null;
  this.confirmPassphrase = null;

  var SERIALIZED_FIELDS = [
    'units',
    'passphraseSalt'
  ];

  this.load = function() {
    if (chrome && chrome.storage) {
      chrome.storage.local.get("settings", function(items) {
        var loadedSettings = items.settings;
        if (loadedSettings) {
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
      chrome.storage.local.set({'settings': toSave}, function() {});
    }
  };

  this.isPassphraseSet = function() {
    return thisSettings.passphraseSalt;
  };

  this.setPassphrase = function() {
    console.log(thisSettings.newPassphrase);
    console.log(thisSettings.confirmPassphrase);

    if (thisSettings.newPassphrase &&
        thisSettings.newPassphrase.length > 0 &&
        thisSettings.newPassphrase == thisSettings.confirmPassphrase) {
      console.log("here");
      var message = {
        'command': 'derive-key',
        'passphrase': thisSettings.newPassphrase
      };

      // We don't want these items lurking in the DOM.
      thisSettings.currentPassphrase = null;
      thisSettings.newPassphrase = null;
      thisSettings.confirmPassphrase = null;

      postMessageWithCallback(message, function(response) {
        // TODO(miket): key should be cleared after something like 5 min.
        thisSettings.passphraseKey = response.key;
        thisSettings.passphraseSalt = response.salt;
        thisSettings.$scope.$apply();
        thisSettings.save();
      });
    }
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
}
