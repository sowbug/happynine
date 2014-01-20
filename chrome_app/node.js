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

// A Node is a node in a BIP0032 tree. If it's a root node, its only
// power is to mint child nodes. Otherwise it generates either child
// nodes or addresses.
function Node() {
  this.init = function() {
    this.extendedPrivateEncrypted = undefined;
    this.extendedPublicBase58 = undefined;
    this.fingerprint = undefined;

    this.nextChangeAddressIndex = 8;
    this.nextPublicAddressIndex = 8;
    this.path = undefined;

    this.publicAddresses = {};
    this.changeAddresses = {};
    this.balance = 0;
    this.batchCount = 8;
    this.hexId = undefined;
    this.nextAddress = 0;
    this.parentFingerprint = undefined;
    this.transactions = [];
    this.unspent_txos = [];  // unserialized
  };
  this.init();

  this.toStorableObject = function() {
    var o = {};
    o.ext_prv_enc = this.extendedPrivateEncrypted;
    o.ext_pub_b58 = this.extendedPublicBase58;
    o.fp = this.fingerprint;
    o.next_change_addr = this.nextChangeAddressIndex;
    o.next_pub_addr = this.nextPublicAddressIndex;
    o.path = this.path;
    return o;
  };

  this.lock = function() {
    this.extendedPrivateBase58 = null;
  };

  this.unlock = function(credentials, callback) {
    if (this.hasPrivateKey()) {
      credentials.decrypt(
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

  this.isKeyAvailable = function() {
    return !!this.extendedPrivateBase58;
  };

  this.hasPrivateKey = function() {
    return !!this.extendedPrivateBase58Encrypted;
  };

  this.deriveIdentifiers = function(callback) {
    var params = {
      'ext_b58': this.extendedPublicBase58
    };
    postRPCWithCallback('get-node', params, function(response) {
      this.hexId = response.hex_id;
      this.fingerprint = response.fingerprint;
      this.parentFingerprint = response.parent_fingerprint;
      callback.call(this);
    }.bind(this));
  };

  this.isRootKey = function() {
    return this.parentFingerprint == "0x00000000";
  }

  this.deriveAddresses = function(isPublic, callback) {
    if (this.isRootKey()) {
      callback.call(this);
      return;
    }
    var params = {
      'ext_b58': this.extendedPublicBase58,
      'path': (isPublic ? "m/0" : "m/1"),
      'start': 0,
      'count': (isPublic ?
                this.nextPublicAddressIndex :
                this.nextChangeAddressIndex)
    };
    postRPCWithCallback('get-addresses', params, function(response) {
      for (var i in response.addresses) {
        var a = response.addresses[i];
        var address = Address.fromStorableObject({
          'address': a.address,
          'index': a.index,
          'path': a.path
        });
        if (isPublic) {
          this.publicAddresses[address.address] = address;
        } else {
          this.changeAddresses[address.address] = address;
        }
      }
      callback.call(this);
    }.bind(this));
  };

  this.derivePublicAddresses = function(callback) {
    this.deriveAddresses.call(this, true, callback);
  };

  this.deriveChangeAddresses = function(callback) {
    this.deriveAddresses.call(this, false, callback);
  };

  this.deriveSelf = function(callback) {
    if (!this.isDerivationComplete) {
      this.deriveIdentifiers(function() {
        this.derivePublicAddresses(function() {
          this.deriveChangeAddresses(function() {
            this.isDerivationComplete = true;
            callback.call(this);
          }.bind(this));
        }.bind(this));
      }.bind(this));
    } else {
      // Callers are depending on this method to be asynchonous.
      window.setTimeout(callback.bind(this), 0);
    }
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

  this.getPublicAddresses = function() {
    return this.publicAddresses;
  };

  this.getChangeAddresses = function() {
    return this.changeAddresses;
  };

  this.addTransactionFromElectrum = function(tx_hex) {
    var params = {
      'txs': [tx_hex],
    };
    postRPCWithCallback(
      'report-transactions',
      params,
      function(response) {
        var params = {
          'ext_b58': this.extendedPublicBase58,
          'pub_n': this.nextPublicAddressIndex,
          'change_n': this.nextChangeAddressIndex
        };
        postRPCWithCallback(
          'get-unspent-txos',
          params,
          function(response) {
            this.unspent_txos = response["unspent_txos"];
            this.calculateBalance();
          }.bind(this));
      }.bind(this));
  };

  this.addTransactionHashesFromElectrum = function(electrum, address, txs) {
    var tx_hashes = [];
    for (var tx in txs) {
      var t = txs[tx];
      if (t.height == 0) {
        console.log("skipping unconfirmed", t.tx_hash);
      } else {
        tx_hashes.push({'tx_hash': t.tx_hash});
      }
    }
    var params = {
      'history': tx_hashes,
    };
    postRPCWithCallback(
      'report-address-history',
      params,
      function(response) {
        for (var i in response.unknown_txs) {
          electrum.enqueueRpc("blockchain.transaction.get",
                              [response.unknown_txs[i]],
                              this.addTransactionFromElectrum.bind(this));
        }
      }.bind(this));
    //this.transactions.concat(txs);
  };

  this.calculateBalance = function() {
    for (var a in this.publicAddresses) {
      this.publicAddresses[a].balance = 0;
    }
    for (var a in this.changeAddresses) {
      this.changeAddresses[a].balance = 0;
    }
    for (var i in this.unspent_txos) {
      var utxo = this.unspent_txos[i];
      var uta = utxo.addr_b58;
      if (uta in this.publicAddresses) {
        this.publicAddresses[uta].balance += utxo.value;
      }
      if (uta in this.changeAddresses) {
        this.changeAddresses[uta].balance += utxo.value;
      }
    }
    this.balance = 0;
    for (var a in this.publicAddresses) {
      this.balance += this.publicAddresses[a].balance;
    }
    for (var a in this.changeAddresses) {
      this.balance += this.changeAddresses[a].balance;
    }
    console.log("New balance", this.balance);
  };

  this.retrieveAllTransactions = function(electrum) {
    var addrs = [];
    for (var a in this.getPublicAddresses()) {
      addrs.push(a);
    }
    for (var a in this.getChangeAddresses()) {
      addrs.push(a);
    }
    for (var a in addrs) {
      electrum.enqueueRpc(
        "blockchain.address.get_history",
        [addrs[a]],
        function(txs) {
          this.addTransactionHashesFromElectrum(electrum, addrs[a], txs);
        }.bind(this)
      );
    }
  };

  this.deriveChildNode = function(credentials, index, isWatchOnly, callback) {
    if (!isWatchOnly && !credentials.isKeyAvailable()) {
      console.log("Can't derive non-watch child node with unlocked wallet");
      // TODO(miket): all these synchronous callbacks need to be async
      callback(this, undefined);
      return;
    }
    if (!this.isKeyAvailable()) {
      console.log("Can't derive child node without key");
      // TODO(miket): all these synchronous callbacks need to be async
      callback(this, undefined);
      return;
    }
    if (this.parentFingerprint != '0x00000000') {
      console.log("Only root nodes should derive children.");
      // TODO(miket): all these synchronous callbacks need to be async
      callback(this, undefined);
      return;
    }

    var params = {
      'ext_b58': this.extendedPrivateBase58,
      'path': "m/" + index + "'"
    };
    postRPCWithCallback(
      'get-node',
      params,
      Node.fromGetNodeResponse.bind(this, credentials, isWatchOnly, callback));
  };
}

Node.fromGetNodeResponse = function(credentials,
                                    isWatchOnly,
                                    callback,
                                    response) {
  if (response.error) {
    console.log("error", response.error.code, response.error.message);
    callback.call(this, undefined);
    return;
  }
  if (response.ext_prv_b58 && !isWatchOnly) {
    credentials.encrypt(response.ext_prv_b58,
                        function(encryptedItem) {
                          callback.call(this, Node.fromStorableObject({
                            'ext_prv_b58': response.ext_prv_b58,
                            'ext_prv_b58_enc': encryptedItem,
                            'ext_pub_b58': response.ext_pub_b58,
                            'path': response.path,
                          }));
                        }.bind(this));
  } else {
    callback.call(this, Node.fromStorableObject({
      'ext_pub_b58': response.ext_pub_b58,
      'path': response.path
    }));
  }

};

Node.fromStorableObject = function(o) {
  var s = new Node();

  if (o.ext_prv_enc != undefined)
    s.extendedPrivateEncrypted = o.ext_prv_enc;
  if (o.fp != undefined)
    s.fingerprint = o.fp;
  if (o.ext_pub_b58 != undefined)
    s.extendedPublicBase58 = o.ext_pub_b58;
  if (o.next_change_addr != undefined)
    s.nextChangeAddressIndex = o.next_change_addr;
  if (o.next_pub_addr != undefined)
    s.nextPublicAddressIndex = o.next_pub_addr;
  if (o.path != undefined)
    s.path = o.path;

  return s;
};
