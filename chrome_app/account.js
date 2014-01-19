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

function Account() {
  this.init = function() {
    this.addresses = {};
    this.balance = 0;
    this.batchCount = 8;
    this.extendedPrivateBase58 = undefined;
    this.extendedPrivateBase58Encrypted = undefined;  // unserialized
    this.extendedPublicBase58 = undefined;
    this.fingerprint = undefined;
    this.hexId = undefined;
    this.nextAddress = 0;
    this.parentFingerprint = undefined;
    this.path = undefined;
    this.transactions = [];
    this.unspent_txos = [];  // unserialized
  };
  this.init();

  this.toStorableObject = function() {
    var o = {};
    o.addresses = {};
    for (var i in this.addresses) {
      var a = this.addresses[i];
      o.addresses[a.address] = a.toStorableObject();
    }
    o.balance = this.balance;
    o.batchCount = this.batchCount;
    o.extendedPrivateBase58Encrypted = this.extendedPrivateBase58Encrypted;
    o.extendedPublicBase58 = this.extendedPublicBase58;
    o.fingerprint = this.fingerprint;
    o.hexId = this.hexId;
    o.nextAddress = this.nextAddress;
    o.parentFingerprint = this.parentFingerprint;
    o.path = this.path;
    o.transactions = this.transactions;

    return o;
  };

  this.getHexId = function() {
    return this.hexId;
  };

  this.extendedPrivateOrPublic = function() {
    if (this.extendedPrivateBase58) {
      return this.extendedPrivateBase58;
    }
    return this.extendedPublicBase58;
  };

  // TODO(miket): NaCl module crashes if you pass in m/0'/0 here and
  // key is public.
  this.fetchAddresses = function(callback) {
    var params = {
      'ext_b58': this.extendedPrivateOrPublic(),
      'path': "m/0",  // external addresses
      'start': this.nextAddress,
      'count': this.batchCount
    };
    postRPCWithCallback('get-addresses', params, function(response) {
      for (var i in response.addresses) {
        var a = response.addresses[i];
        var address = Address.fromStorableObject({
          'address': a.address,
          'key': a.key,
          'index': a.index,
          'path': a.path
        });
        this.addresses[address.address] = address;
      }
      callback.call(this);
    }.bind(this));
  };

  this.buildAddressList = function() {
    var addrList = [];
    for (var i in this.addresses) {
      var address = this.addresses[i];
      addrList.push(address.address);
    }
    return addrList;
  };

  this.fetchBalances = function($http, callback) {
    var url = 'https://blockchain.info/multiaddr?format=json' +
      '&active=' +
      this.buildAddressList().join('|');

    $http({method: 'GET', url: url}).
      success(function(data, status, headers, config) {
        for (var i in data.addresses) {
          var addr = data.addresses[i];
          this.addresses[addr.address].balance = addr.final_balance;
          this.addresses[addr.address].tx_count = addr.n_tx;
        }
        this.transactions = [];
        for (var i in data.txs) {
          var tx = data.txs[i];
          this.transactions.push({
            'date': tx.time,
            'address': tx.inputs[0].prev_out.addr,
            'hash': tx.hash,
            'amount': Math.floor(Math.random() * 1000000000)
          });
        }
        this.calculateBalance();
        callback.call(this, true);
      }.bind(this)).
      error(function(data, status, headers, config) {
        console.log("error", status, data);
        callback.call(this, false);
      }.bind(this));
  };

  this.fetchUnspent = function($http, callback) {
    var url = 'https://blockchain.info/unspent?' +
      '&active=' +
      this.buildAddressList().join('|');

    $http({method: 'GET', url: url}).
      success(function(data, status, headers, config) {
        this.unspent_txos = data.unspent_outputs;
        this.calculateUnspent();
        callback.call(this, true);
      }.bind(this)).
      error(function(data, status, headers, config) {
        console.log("error", status, data);
        callback.call(this, false);
      }.bind(this));
  };

  this.calculateBalance = function() {
    this.balance = 0;
    for (var i in this.addresses) {
      var address = this.addresses[i];
      this.balance += address.balance;
    }
  };

  this.calculateUnspent = function() {
    var value = 0;
    var tx_count = 0;
    for (var i in this.unspent_txos) {
      var txo = this.unspent_txos[i];
      value += txo.value;
      tx_count++;
    }
    console.log("unspent", value, tx_count);
  };

  this.hasTransactions = function() {
    return this.transactions.length > 0;
  };

  this.name = function() {
    return this.path;
  };

  this.getAddresses = function() {
    return this.addresses;
  };

  this.handleWalletLockChange = function(credentials, callback) {
    if (!credentials.isKeyAvailable() ||
        !this.extendedPrivateBase58Encrypted) {
      this.extendedPrivateBase58 = undefined;
      window.setTimeout(function() { callback.call(this); }, 0);
      return;
    }
    credentials.decrypt(
      this.extendedPrivateBase58Encrypted,
      function(item) {
        this.extendedPrivateBase58 = item;
      }.bind(this));
  };

  this.sendFunds = function($http, recipient, value, fee, callback) {
    var params = {
      'ext_prv_b58': this.extendedPrivateBase58,
      'unspent_txos': this.unspent_txos,
      'recipients': [{
        'address': recipient,
        'value': value
      }],
      'fee': fee,
      'change_index': 0
    };
    postRPCWithCallback(
      'get-signed-transaction',
      params,
      function(response) {
        console.log(response);
      if (response.signed_tx) {
        var headers = { 'Content-Type': 'application/x-www-form-urlencoded' };
        $http.post("https://blockchain.info/pushtx",
                   "tx=" + response.signed_tx,
                   { 'headers': headers }).
          success(function(data, status, headers, config) {
            console.log("pushtx", data, status);
          }).
          error(function(data, status, headers, config) {
            console.log("pushtx-error", data, status);
          });
      }
      callback.call(this);
    });
  };
}

Account.fromStorableObject = function(o) {
  var s = new Account();

  if (o.addresses != undefined) {
    for (var i in o.addresses) {
      var address = Address.fromStorableObject(o.addresses[i]);
      s.addresses[address.address] = address;
    }
  }
  if (o.balance != undefined) s.balance = o.balance;
  if (o.batchCount != undefined) s.batchCount = o.batchCount;
  if (o.extendedPrivateBase58Encrypted != undefined)
    s.extendedPrivateBase58Encrypted = o.extendedPrivateBase58Encrypted;
  if (o.extendedPublicBase58 != undefined)
    s.extendedPublicBase58 = o.extendedPublicBase58;
  if (o.fingerprint != undefined) s.fingerprint = o.fingerprint;
  if (o.hexId != undefined)
    s.hexId = o.hexId;
  if (o.nextAddress != undefined)
    s.nextAddress = o.nextAddress;
  if (o.parentFingerprint != undefined)
    s.parentFingerprint = o.parentFingerprint;
  if (o.path != undefined) s.path = o.path;
  if (o.transactions != undefined)
    s.transactions = o.transactions;

  // Items that shouldn't be passed from a truly persisted object, but
  // that might be serialized in-memory.
  if (o.extendedPrivateBase58 != undefined)
    s.extendedPrivateBase58 = o.extendedPrivateBase58;
  return s;
};
