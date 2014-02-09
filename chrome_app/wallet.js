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
  this.initAddresses = function() {
    this.watchedAddresses = {};
    this.publicAddresses = [];
    this.changeAddresses = [];
    this.recentTransactions = [];
    this.balance = 0;
  };

  this.init = function() {
    this.rootNodes = [];
    this.nodes = [];
    this.initAddresses();
  };
  this.init();
  electrum.issueHeadersSubscribe()
    .then(function(response) {
      this.handleBlockGetHeader(response);
    });

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
    return Promise.all(nodes.map(this.describeNode.bind(this)));
  };

  this.describeNode = function(node) {
    return new Promise(function(resolve, reject) {
      var params = {
        'ext_pub_b58': node.extendedPublicBase58,
      };
      postRPC('describe-node', params)
        .then(function(response) {
          if (response.ext_pub_b58) {
            var dnode = Node.fromStorableObject(response);
            // If we had the private key, pass it through.
            if (node.extendedPrivateEncrypted) {
              dnode.extendedPrivateEncrypted = node.extendedPrivateEncrypted;
            }
            // TODO(miket): don't insert duplicates
            if (dnode.isMaster()) {
              this.rootNodes.push(dnode);
            } else {
              this.nodes.push(dnode);
            }
            resolve();
          } else {
            reject(response);
          }
        }.bind(this));
    }.bind(this));
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
            if (node.isMaster()) {
              resolve();
            } else {
              this.initAddresses();
              this.getAddresses().then(resolve);
            }
          } else {
            reject(response);
          }
        }.bind(this));
    }.bind(this));
  };

  this.removeNode = function(node) {
    return new Promise(function(resolve, reject) {
      var newNodes = [];
      var oldNodes;
      if (node.isMaster()) {
        oldNodes = this.rootNodes;
      } else {
        oldNodes = this.nodes;
      }
      for (var i in oldNodes) {
        var n = oldNodes[i];
        if (!(n.fingerprint == node.fingerprint &&
              n.childNum == node.childNum &&
              n.parentFingerprint == node.parentFingerprint)) {
          newNodes.push(n);
        }
      }
      if (node.childNum == 0) {
        this.rootNodes = newNodes;
      } else {
        this.nodes = newNodes;
      }
      resolve();
      // TODO(miket): whoever's in charge of the current account needs
      // to keep it up to date after this operation.
      //
      // TODO(miket): pretty sure this will have to be async, so
      // setting it up that way now
    }.bind(this));
  };

  this.isKeySet = function() {
    return this.rootNodes.length > 0;
  };

  this.getChildNodes = function() {
    return this.nodes;
  };

  this.getNextChildNodeNumber = function() {
    var next = 0x80000000;
    for (var i in this.nodes) {
      if (this.nodes[i].childNum >= next) {
        next = this.nodes[i].childNum + 1;
      }
    }
    return next - 0x80000000;
  };

  this.addRandomMasterKey = function() {
    return new Promise(function(resolve, reject) {
      postRPC('generate-root-node', {})
        .then(function(response) {
          var node = Node.fromStorableObject(response);
          this.describeNode(node).then(resolve);
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
            this.describeNode(node).then(resolve);
          } else {
            reject();
          }
        }.bind(this));
    }.bind(this));
  };

  this.removeMasterKey = function() {
    this.rootNodes = [];
  };

  this.deriveChildNode = function(childNum, isWatchOnly) {
    return new Promise(function(resolve, reject) {
      var params = {
        'path': "m/" + childNum + "'",
        'is_watch_only': isWatchOnly,
      };
      postRPC('derive-child-node', params)
        .then(function(response) {
          var node = Node.fromStorableObject(response);
          this.describeNode(node).then(resolve);
        }.bind(this));
    }.bind(this));
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
            logImportant("got", response.tx);
            electrum.issueTransactionBroadcast(response.tx)
              .then(resolve);
          } else {
            reject("create-tx failed");
          }
        }.bind(this));
    }.bind(this));
  };

  this.getChildNodeCount = function() {
    return this.nodes.length;
  };

  this.getBlockHeader = function(height) {
    return new Promise(function(resolve, reject) {
      electrum.issueBlockGetHeader(height)
        .then(function(response) {
          this.handleBlockGetHeader(response)
            .then(this.getHistory.bind(this))
            .then(resolve);
        }.bind(this));
    }.bind(this));
  };

  this.handleAddressGetHistory = function(txs) {
    return new Promise(function(resolve, reject) {
      postRPC('report-tx-statuses', { 'tx_statuses': txs })
        .then(function(response) {
          var tx_hashes = [];
          var heights = [];
          for (var i in txs) {
            tx_hashes.push(txs[i].tx_hash);
            heights.push(txs[i].height);
          }
          var promises = tx_hashes.map(this.handleTransactionGet.bind(this))
            .concat(heights.map(this.getBlockHeader.bind(this)));
          var catchError = function(err) {
            logFatal("handleAddressGetHistory", err);
          };
          Promise.all(promises).then(resolve).catch(catchError.bind(this));
        }.bind(this));
    }.bind(this));
  };

  this.handleAddressSubscribe = function(response) {
    return new Promise(function(resolve, reject) {
      logInfo("Subscribed to address", response);
    }.bind(this));
  };

  this.handleTransactionGet = function(tx_hash) {
    return new Promise(function(resolve, reject) {
      electrum.issueTransactionGet(tx_hash)
        .then(function(response) {
          postRPC('report-txs', { 'txs': [{ 'tx': response }] })
            .then(function(response) {
              this.getHistory()  // TODO: move this to caller so less waste
                .then(this.getAddresses.bind(this))
                .then(resolve);
            }.bind(this));
        }.bind(this));
    }.bind(this))
  };

  this.recalculateWalletBalance = function() {
    this.balance = 0;
    for (var i in this.watchedAddresses) {
      this.balance += this.watchedAddresses[i].value;
    }
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
    electrum.issueAddressGetHistory(addr_b58)
      .then(this.handleAddressGetHistory.bind(this));
    electrum.issueAddressSubscribe(addr_b58);
  };

  this.updateAddress = function(addr_status) {
    this.watchedAddresses[addr_status.addr_b58] = addr_status;
  };

  this.getAddressBalance = function(addr_b58) {
    if (this.isWatching(addr_b58)) {
      return this.watchedAddresses[addr_b58].value;
    }
    return 0;
  }

  this.getAddressTxCount = function(addr_b58) {
    if (this.isWatching(addr_b58)) {
      return this.watchedAddresses[addr_b58].tx_count;
    }
    return 0;
  }

  this.getBalance = function() {
    return this.balance;
  };

  this.getAddresses = function() {
    return new Promise(function(resolve, reject) {
      postRPC('get-addresses', {})
        .then(function(response) {
          for (var i in response.addresses) {
            var addr = response.addresses[i];
            this.watchAddress(addr.addr_b58, addr.is_public);
            this.updateAddress(addr);
          }
          this.recalculateWalletBalance();
          resolve();
        }.bind(this));
    }.bind(this));
  };

  this.getHistory = function() {
    return new Promise(function(resolve, reject) {
      postRPC('get-history', {})
        .then(function(response) {
          this.recentTransactions = response.history;
          resolve();
        }.bind(this));
    }.bind(this));
  };

  this.handleBlockGetHeader = function(h) {
    logInfo("new block", h);
    return new Promise(function(resolve, reject) {
      var params = { 'timestamp': h.timestamp,
                     'block_height': h.block_height };
      postRPC('confirm-block', params)
        .then(resolve);
    }.bind(this));
  };

  $(document).on("blockchain.address.subscribe", function(evt) {
    var params = evt.message;
    electrum.issueAddressGetHistory(params[0])
      .then(this.handleAddressGetHistory.bind(this));
  }.bind(this));

  $(document).on("blockchain.headers.subscribe", function(evt) {
    var params = evt.message;
    this.handleBlockGetHeader(params[0]);
  }.bind(this));

  this.setActiveMasterNode = function(node) {
    return new Promise(function(resolve, reject) {
      if (this.activeMasterNode == node) {
        resolve();
      } else {
        this.activeMasterNode = node;
        if (node) {
          this.initAddresses();
          this.restoreNode(this.activeMasterNode).then(resolve);
        } else {
          this.activeMasterNode = undefined;
          resolve();
        }
      }
    }.bind(this));
  };

  this.setActiveMasterNodeByIndex = function(index) {
    return new Promise(function(resolve, reject) {
      var node;
      if (index < this.rootNodes.length) {
        node = this.rootNodes[index];
      }
      this.setActiveMasterNode(node).then(resolve);
    }.bind(this));
  };

  this.setActiveChildNode = function(node) {
    return new Promise(function(resolve, reject) {
      if (this.activeChildNode == node) {
        resolve();
      } else {
        this.activeChildNode = node;
        if (node) {
          this.initAddresses();
          this.restoreNode(this.activeChildNode).then(resolve);
        } else {
          this.activeChildNode = undefined;
          resolve();
        }
      }
    }.bind(this));
  };

  this.setActiveChildNodeByIndex = function(index) {
    return new Promise(function(resolve, reject) {
      var node;
      if (index < this.nodes.length) {
        node = this.nodes[index];
      }
      this.setActiveChildNode(node).then(resolve);
    }.bind(this));
  };

  this.restoreInitialMasterNode = function() {
    return new Promise(function(resolve, reject) {
      this.setActiveMasterNode(0).then(resolve);
    }.bind(this));
  };

  this.restoreInitialChildNode = function() {
    return new Promise(function(resolve, reject) {
      this.setActiveChildNode(0).then(resolve);
    }.bind(this));
  };

  this.STORAGE_NAME = 'wallet';
  this.load = function() {
    return new Promise(function(resolve, reject) {
      var success = function(response) {
        if (response) {
          this.loadStorableObject(response)
            .then(this.restoreInitialMasterNode.bind(this))
            .then(this.restoreInitialChildNode.bind(this))
            .then(resolve)
            .catch(function(err) { logFatal(err); });
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
