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
function Wallet(credentials) {
  this.credentials = credentials;

  this.init = function() {
    this.rootNodes = [];
    this.nodes = [];
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
            this.checkForNotifications(response);
            resolve();
          } else {
            this.checkForNotifications(response);
            reject(response);
          }
        }.bind(this));
    }.bind(this));
  };

  this.checkForNotifications = function(response) {
    if (response.address_statuses) {
      console.log("noticed address_statuses");
      var as = response.address_statuses;
      for (var i in as) {
        console.log(as[i].addr_b58);
      }
      $.event.trigger({
        'type': 'address_statuses',
        'message': as,
        time: new Date(),
      });

    } else if (response.tx_requests) {
      console.log("noticed tx_requests");
    }
  };

  this.deriveNodes = function(callback) {
    var ns = this.rootNodes.concat(this.nodes);
    var f = function() {
      if (ns.length) {
        ns.pop().deriveSelf(f.bind(this));
      } else {
        callback.call(this);
      }
    };
    f();
  };

  this.unlockNodes = function(credentials, callback) {
    var ns = this.rootNodes.concat(this.nodes);
    var f = function() {
      if (ns.length) {
        ns.pop().unlock(credentials, f.bind(this));
      } else {
        callback.call(this, true);
      }
    };
    f();
  };

  this.isKeySet = function() {
    return this.rootNodes.length > 0;
  };

  this.getAccounts = function() {
    return this.nodes;
  };

  this.getNextAccountNumber = function() {
    return 99;  // TODO
  };

  this.getExtendedPrivateBase58 = function() {
    if (!this.credentials.isKeyAvailable()) {
      console.log("warning: getPrivateBase58 with locked wallet");
    }
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

  this.setNodeCallback = function(isRoot, response) {
    if (response.fp) {
      var node = Node.fromStorableObject(response);
      if (isRoot) {
        this.rootNodes.push(node);
      } else {
        this.nodes.push(node);
      }
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

  this.addNewNode = function(isRoot, callback, node) {
    if (node) {
      if (isRoot) {
        this.rootNodes.push(node);
      } else {
        this.nodes.push(node);
      }
      this.deriveNodes(callback.bind(this, true));
    } else {
      callback.call(this, false);
    }
  };

  this.removeMasterKey = function() {
    this.rootNodes = [];
  };

  this.deriveNextAccount = function(isWatchOnly, callbackBool) {
    var params = {
      'path': "m/0'",
      'is_watch_only': isWatchOnly,
    };
    postRPC('derive-child-node', params)
      .then(function(response) {
        var node = Node.fromStorableObject(response);
        this.restoreNode(node).then(resolve);
      }.bind(this));
  };

  this.getAccountCount = function() {
    return this.nodes.length;
  };

  this.handleGetHistory = function(txs) {
    return new Promise(function(resolve, reject) {
      postRPC('report-tx-statuses', { 'tx_statuses': txs })
        .then(function(response) {
          console.log("cool:", response);
          resolve();
        }.bind(this));
    }.bind(this));
  };

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
