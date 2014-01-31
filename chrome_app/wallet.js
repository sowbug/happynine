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

// A Wallet is a collection of Nodes with a bunch of helper functions.
function Wallet(electrum) {
  this.init = function() {
    this.rootNodes = [];
    this.nodes = [];
    this.watchedAddresses = {};
    this.publicAddresses = [];
    this.changeAddresses = [];
  };
  this.init();

  this.toStorableObject = function() {
    var o = {};
    o.rnodes = [];
    for (var i in this.rootNodes) {
      o.rnodes.push(this.rootNodes[i].toStorableObject());
    }
    o.nodes = [];
    for (var i in this.nodes) {
      o.nodes.push(this.nodes[i].toStorableObject());
    }
    return o;
  };

  this.loadStorableObject = function(o) {
    this.init();
    var nodes = [];
    for (var i in o.rnodes) {
      nodes.push(Node.fromStorableObject(o.rnodes[i]));
    }
    for (var i in o.nodes) {
      nodes.push(Node.fromStorableObject(o.nodes[i]));
    }
    return Promise.all(nodes.map(this.restoreNode.bind(this)));
  };

  this.restoreNode = function(node) {
    return new Promise(function(resolve, reject) {
      var params = {
        'ext_pub_b58': node.extendedPublicBase58,
        'ext_prv_enc': node.extendedPrivateEncrypted,
      };
      postRPC('restore-node', params)
        .then(function(response) {
          if (response.fp) {
            var node = Node.fromStorableObject(response);
            if (response.pfp == "0x00000000") {
              this.rootNodes.push(node);
            } else {
              this.nodes.push(node);
            }
            resolve();
          } else {
            reject(response);
          }
        }.bind(this));
    }.bind(this));
  };

  this.isKeySet = function() {
    return this.rootNodes.length > 0;
  };

  this.getAccounts = function() {
    return this.nodes;
  };

  this.getExtendedPrivateBase58 = function() {
    if (this.rootNodes.length > 0) {
      return this.rootNodes[0].extendedPrivateBase58;
    }
  };

  this.isExtendedPrivateSet = function() {
    return this.rootNodes.length > 0 &&
      !!this.rootNodes[0].extendedPrivateEncrypted;
  };

  this.getExtendedPublicBase58 = function() {
    if (this.rootNodes.length > 0) {
      return this.rootNodes[0].extendedPublicBase58;
    }
  };

  this.getFingerprint = function() {
    if (this.rootNodes.length > 0) {
      return this.rootNodes[0].fingerprint;
    }
  };

  this.addRandomMasterKey = function() {
    return new Promise(function(resolve, reject) {
      postRPC('generate-root-node', {})
        .then(function(response) {
          var node = Node.fromStorableObject(response);
          this.restoreNode(node).then(resolve);
        }.bind(this));
    }.bind(this));
  };

  this.importMasterKey = function(ext_prv_b58) {
    return new Promise(function(resolve, reject) {
      var params = {
        'ext_prv_b58': ext_prv_b58
      };
      postRPC('import-root-node', params)
        .then(function(response) {
          if (response.fp) {
            var node = Node.fromStorableObject(response);
            this.restoreNode(node).then(resolve);
          } else {
            reject();
          }
        }.bind(this));
    }.bind(this));
  };

  this.removeMasterKey = function() {
    this.rootNodes = [];
  };

  this.deriveChildNode = function(childNodeIndex, isWatchOnly) {
    return new Promise(function(resolve, reject) {
      var params = {
        'path': "m/" + childNodeIndex + "'",
        'is_watch_only': isWatchOnly,
      };
      postRPC('derive-child-node', params)
        .then(function(response) {
          var node = Node.fromStorableObject(response);
          if (node) {
            this.restoreNode(node).then(resolve);
          } else {
            reject();
          }
        }.bind(this));
    }.bind(this));
  }

  this.suggestedChildNodeIndex = function() {
    var max_index = 0;
    for (var i in this.nodes) {
      if (this.nodes[i].childNum > max_index) {
        max_index = this.nodes[i].childNum;
      }
    }
    // TODO(miket): someone wanting to break this could do so.
    return (max_index + 1) & 0x7fffffff;
  };

  this.sendFunds = function(sendTo, sendValue, sendFee) {
    return new Promise(function(resolve, reject) {
      var params = {
        'sign': true,
        'fee': sendFee,
        'recipients': [{'addr_b58': sendTo, 'value': sendValue}],
      };
      postRPC('create-tx', params)
        .then(function(response) {
          if (response.tx) {
            console.log("got", response.tx);
            electrum.enqueueRpc("blockchain.transaction.broadcast",
                                [response.tx])
              .then(resolve);
          } else {
            reject("create-tx failed");
          }
        }.bind(this));
    }.bind(this));
  };

  this.getAccountCount = function() {
    return this.nodes.length;
  };

  this.handleAddressGetHistory = function(txs) {
    return new Promise(function(resolve, reject) {
      postRPC('report-tx-statuses', { 'tx_statuses': txs })
        .then(function(response) {
          resolve();
        }.bind(this));
    }.bind(this));
  };

  this.handleAddressSubscribe = function(response) {
    return new Promise(function(resolve, reject) {
      console.log("for address subscribe", response);
    }.bind(this));
  };

  this.handleTransactionGet = function(tx) {
    return new Promise(function(resolve, reject) {
      postRPC('report-txs', { 'txs': [{'tx': tx}] })
        .then(function(response) {
          resolve();
        }.bind(this));
    }.bind(this));
  };

  this.isWatching = function(addr_b58) {
    return !!this.watchedAddresses[addr_b58];
  }

  this.watchAddress = function(addr_b58, is_public) {
    if (this.isWatching(addr_b58)) {
      return;
    }
    this.watchedAddresses[addr_b58] = {};
    if (is_public) {
      // We assume the backend will always tell us about these
      // addresses in order.
      this.publicAddresses.push(addr_b58);
    } else {
      this.changeAddresses.push(addr_b58);
    }
    electrum.enqueueRpc("blockchain.address.get_history",
                        [addr_b58])
      .then(this.handleAddressGetHistory.bind(this));
    electrum.enqueueRpc("blockchain.address.subscribe",
                        [addr_b58])
      .then(this.handleAddressSubscribe.bind(this));
  };

  this.updateAddress = function(addr_status) {
    this.watchedAddresses[addr_status.addr_b58] = addr_status;
  };

  $(document).on("address_statuses", function(evt) {
    var addrs = evt.message;
    for (var a in addrs) {
      var addr = addrs[a];
      this.watchAddress(addr.addr_b58, addr.is_public);
      this.updateAddress(addr);
    }
  }.bind(this));

  $(document).on("tx_requests", function(evt) {
    var tx_requests = evt.message;
    for (var t in tx_requests) {
      console.log("requesting", tx_requests[t]);
      electrum.enqueueRpc("blockchain.transaction.get",
                          [tx_requests[t]])
        .then(this.handleTransactionGet.bind(this));
    }
  }.bind(this));

  $(document).on("blockchain.address.subscribe", function(evt) {
    var params = evt.message;
    electrum.enqueueRpc("blockchain.address.get_history",
                        [params[0]])
      .then(this.handleAddressGetHistory.bind(this));
  }.bind(this));

  this.STORAGE_NAME = 'wallet';
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
