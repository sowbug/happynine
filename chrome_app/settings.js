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

// The Settings model keeps track of miscellaneous options/preferences
// that are expected to be saved across app restarts.
function Settings() {
  this.availableUnits = {
    "btc": "BTC",
    "mbtc": "mBTC",
    "ubtc": "uBTC",
    "sats": "Satoshis"
  };

  this.init = function() {
    this.units = "mbtc";
  };
  this.init();

  this.toStorableObject = function() {
    var o = {};
    o.units = this.units;
    return o;
  };

  this.loadStorableObject = function(o) {
    this.init();
    this.units = o.units;
  };

  this.satoshiToUnit = function(satoshis) {
    switch (this.units) {
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

  this.unitToSatoshi = function(units) {
    switch (this.units) {
    case "btc":
      return units * 100000000;
    case "mbtc":
       return units * 100000;
    case "ubtc":
       return units * 100;
    default:
      return units;
    }
  };

  this.unitLabel = function() {
    return this.availableUnits[this.units];
  };

  this.STORAGE_NAME = 'settings';
  this.load = function() {
    return new Promise(function(resolve, reject) {
      var success = function(response) {
        if (response) {
          this.loadStorableObject(response);
        } else {
          this.init();
        }
        resolve();
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
