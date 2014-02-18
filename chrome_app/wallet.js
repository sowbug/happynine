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

/**
 * @constructor
 */
function Wallet(api_client, electrum) {
  this.api_client = api_client;
  this.electrum = electrum;
  this.init();

  $(document).on("blockchain.address.subscribe", function(evt) {
    var params = evt.message;
    this.electrum.issueAddressGetHistory(params[0])
      .then(this.handleAddressGetHistory.bind(this));
  }.bind(this));

  $(document).on("blockchain.headers.subscribe", function(evt) {
    var params = evt.message;
    this.handleBlockGetHeader(params[0]);
  }.bind(this));
}

Wallet.prototype.initAddresses = function() {
  this.watchedAddresses = {};
  this.publicAddresses = [];
  this.changeAddresses = [];
  this.recentTransactions = [];
  this.balance = 0;
};

Wallet.prototype.init = function() {
  this.masterNodes = [];
  this.nodes = [];
  this.currentHeight = 0;
  this.initAddresses();
};

Wallet.prototype.startElectrum = function() {
  this.electrum.connectToServer()
    .then(this.electrum.issueHeadersSubscribe.bind(this.electrum))
    .then(this.handleBlockGetHeader.bind(this));
};

Wallet.prototype.toStorableObject = function() {
  var o = {};
  o['rnodes'] = [];
  for (var i in this.masterNodes) {
    o['rnodes'].push(this.masterNodes[i].toStorableObject());
  }
  o['nodes'] = [];
  for (var i in this.nodes) {
    o['nodes'].push(this.nodes[i].toStorableObject());
  }
  o['height'] = this.currentHeight;
  return o;
};

Wallet.prototype.loadStorableObject = function(o) {
  this.init();
  var nodes = [];
  for (var i in o['rnodes']) {
    nodes.push(Node.fromStorableObject(o['rnodes'][i]));
  }
  for (var i in o['nodes']) {
    nodes.push(Node.fromStorableObject(o['nodes'][i]));
  }
  this.currentHeight = o['height'];
  return Promise.all(nodes.map(this.describeNode.bind(this)));
};

Wallet.prototype.describeNode = function(node) {
  return new Promise(function(resolve, reject) {
    this.api_client.describeNode(node.extendedPublicBase58)
      .then(function(response) {
        if (response['ext_pub_b58']) {
          var dnode = Node.fromStorableObject(response);
          // If we had the private key, pass it through.
          if (node.extendedPrivateEncrypted) {
            dnode.extendedPrivateEncrypted = node.extendedPrivateEncrypted;
          }
          // TODO(miket): don't insert duplicates
          if (dnode.isMaster()) {
            this.masterNodes.push(dnode);
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

Wallet.prototype.restoreNode = function(node) {
  return new Promise(function(resolve, reject) {
    this.api_client.restoreNode(node.extendedPublicBase58,
                                node.extendedPrivateEncrypted)
      .then(function(response) {
        if (response['fp']) {
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

Wallet.prototype.removeNode = function(node) {
  return new Promise(function(resolve, reject) {
    var newNodes = [];
    var oldNodes;
    if (node.isMaster()) {
      oldNodes = this.masterNodes;
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
      this.masterNodes = newNodes;
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

Wallet.prototype.isKeySet = function() {
  return this.masterNodes.length > 0;
};

Wallet.prototype.getMasterNodes = function() {
  return this.masterNodes;
};

Wallet.prototype.getChildNodes = function() {
  return this.nodes;
};

Wallet.prototype.getNextChildNodeNumber = function() {
  var next = 0x80000000;
  for (var i in this.nodes) {
    if (this.nodes[i].childNum >= next) {
      next = this.nodes[i].childNum + 1;
    }
  }
  return next - 0x80000000;
};

Wallet.prototype.addRandomMasterKey = function() {
  return new Promise(function(resolve, reject) {
    this.api_client.generateMasterNode()
      .then(function(response) {
        var node = Node.fromStorableObject(response);
        this.describeNode(node).then(resolve);
      }.bind(this));
  }.bind(this));
};

Wallet.prototype.importMasterKey = function(extendedPrivateBase58) {
  return new Promise(function(resolve, reject) {
    this.api_client.importMasterNode(extendedPrivateBase58)
      .then(function(response) {
        if (response['fp']) {
          var node = Node.fromStorableObject(response);
          this.describeNode(node).then(resolve);
        } else {
          reject(response['error']['message']);
        }
      }.bind(this),
            reject);
  }.bind(this));
};

Wallet.prototype.removeMasterKey = function() {
  this.masterNodes = [];
};

Wallet.prototype.retrievePrivateKey = function(node) {
  return new Promise(function(resolve, reject) {
    this.api_client.describePrivateNode(node.extendedPrivateEncrypted)
      .then(function(response) {
        if (response['ext_prv_b58']) {
          resolve(response['ext_prv_b58']);
        } else {
          reject();
        }
      }.bind(this));
  }.bind(this));
};

Wallet.prototype.deriveChildNode = function(childNum, isWatchOnly) {
  return new Promise(function(resolve, reject) {
    this.api_client.deriveChildNode(childNum, isWatchOnly)
      .then(function(response) {
        var node = Node.fromStorableObject(response);
        this.describeNode(node).then(resolve);
      }.bind(this));
  }.bind(this));
};

Wallet.prototype.sendFunds = function(sendTo, sendValue, sendFee) {
  return new Promise(function(resolve, reject) {
    this.api_client.createTx([{'addr_b58': sendTo, 'value': sendValue}],
                             sendFee, true)
      .then(function(response) {
        if (response['tx']) {
          logImportant("GENERATED TX", response['tx']);
          this.electrum.issueTransactionBroadcast(response['tx'])
            .then(resolve);
        } else {
          reject("create-tx failed");
        }
      }.bind(this));
  }.bind(this));
};

Wallet.prototype.getChildNodeCount = function() {
  return this.nodes.length;
};

Wallet.prototype.getBlockHeader = function(height) {
  return new Promise(function(resolve, reject) {
    this.electrum.issueBlockGetHeader(height)
      .then(function(response) {
        this.handleBlockGetHeader(response)
          .then(this.getHistory.bind(this))
          .then(resolve);
      }.bind(this));
  }.bind(this));
};

Wallet.prototype.handleAddressGetHistory = function(txs) {
  return new Promise(function(resolve, reject) {
    if (txs.length == 0) {
      resolve();
      return;
    }
    this.api_client.reportTxStatuses(txs)
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

Wallet.prototype.handleAddressSubscribe = function(response) {
  return new Promise(function(resolve, reject) {
    logInfo("Subscribed to address", response);
  }.bind(this));
};

Wallet.prototype.handleTransactionGet = function(tx_hash) {
  return new Promise(function(resolve, reject) {
    this.electrum.issueTransactionGet(tx_hash)
      .then(function(response) {
        this.api_client.reportTxs([{ 'tx': response }])
          .then(function(response) {
            this.getHistory()  // TODO: move this to caller so less waste
              .then(this.getAddresses.bind(this))
              .then(resolve);
          }.bind(this));
      }.bind(this));
  }.bind(this))
};

Wallet.prototype.recalculateWalletBalance = function() {
  this.balance = 0;
  for (var i in this.watchedAddresses) {
    this.balance += this.watchedAddresses[i].value;
  }
};

Wallet.prototype.isWatching = function(addr_b58) {
  return !!this.watchedAddresses[addr_b58];
}

Wallet.prototype.watchAddress = function(addr_b58, is_public) {
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
  this.electrum.issueAddressGetHistory(addr_b58)
    .then(this.handleAddressGetHistory.bind(this));
  this.electrum.issueAddressSubscribe(addr_b58);
};

Wallet.prototype.updateAddress = function(addr_status) {
  this.watchedAddresses[addr_status.addr_b58] = addr_status;
};

Wallet.prototype.getAddressBalance = function(addr_b58) {
  if (this.isWatching(addr_b58)) {
    return this.watchedAddresses[addr_b58].value;
  }
  return 0;
}

Wallet.prototype.getAddressTxCount = function(addr_b58) {
  if (this.isWatching(addr_b58)) {
    return this.watchedAddresses[addr_b58].tx_count;
  }
  return 0;
}

Wallet.prototype.getBalance = function() {
  return this.balance;
};

Wallet.prototype.getAddresses = function() {
  return new Promise(function(resolve, reject) {
    this.api_client.getAddresses()
      .then(function(response) {
        for (var i in response['addresses']) {
          var addr = response['addresses'][i];
          this.watchAddress(addr['addr_b58'], addr['is_public']);
          this.updateAddress(addr);
        }
        this.recalculateWalletBalance();
        resolve();
      }.bind(this));
  }.bind(this));
};

Wallet.prototype.getHistory = function() {
  return new Promise(function(resolve, reject) {
    this.api_client.getHistory()
      .then(function(response) {
        this.recentTransactions = response['history'];
        resolve();
      }.bind(this));
  }.bind(this));
};

Wallet.prototype.handleBlockGetHeader = function(h) {
  logInfo("new block", h);
  return new Promise(function(resolve, reject) {
    var height = h['block_height'];
    var timestamp = h['timestamp'];
    if (this.currentHeight == undefined || height > this.currentHeight) {
      this.currentHeight = height;
    }
    this.api_client.confirmBlock(height, timestamp)
      .then(resolve);
  }.bind(this));
};

Wallet.prototype.setActiveMasterNode = function(node) {
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

Wallet.prototype.setActiveMasterNodeByIndex = function(index) {
  return new Promise(function(resolve, reject) {
    var node;
    if (index < this.masterNodes.length) {
      node = this.masterNodes[index];
    }
    this.setActiveMasterNode(node).then(resolve);
  }.bind(this));
};

Wallet.prototype.setActiveChildNode = function(node) {
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

Wallet.prototype.setActiveChildNodeByIndex = function(index) {
  return new Promise(function(resolve, reject) {
    var node;
    if (index < this.nodes.length) {
      node = this.nodes[index];
    }
    this.setActiveChildNode(node).then(resolve);
  }.bind(this));
};

Wallet.prototype.restoreInitialMasterNode = function() {
  return new Promise(function(resolve, reject) {
    this.setActiveMasterNode(0).then(resolve);
  }.bind(this));
};

Wallet.prototype.restoreInitialChildNode = function() {
  return new Promise(function(resolve, reject) {
    this.setActiveChildNode(0).then(resolve);
  }.bind(this));
};

Wallet.STORAGE_NAME = 'wallet';
Wallet.prototype.load = function() {
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
    loadStorage(Wallet.STORAGE_NAME).then(success.bind(this),
                                          failure.bind(this));
  }.bind(this));
};

Wallet.prototype.save = function() {
  saveStorage(Wallet.STORAGE_NAME, this.toStorableObject());
};
