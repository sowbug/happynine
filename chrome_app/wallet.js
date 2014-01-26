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
    return new Promise(function(resolve, reject) {
      this.init();
      var rootNodes = [];
      for (var i in o.rnodes) {
        rootNodes.push(Node.fromStorableObject(o.rnodes[i]));
      }
      var nodes = [];
      for (var i in o.nodes) {
        nodes.push(Node.fromStorableObject(o.nodes[i]));
      }
      this.installRootNodes(rootNodes).then(function() {
        this.installNodes(nodes);
      }.bind(this)).then(function() {
        resolve();
      });
    }.bind(this));
  };

  this.installRootNodes = function(nodes) {
    return new Promise(function(resolve, reject) {
      var f = function() {
        if (nodes.length) {
          var node = nodes.pop();
          var params = {
            'ext_pub_b58': node.extendedPublicBase58,
            'ext_prv_enc': node.extendedPrivateEncrypted,
          };
          postRPCWithCallback(
            'restore-node',
            params,
            this.setNodeCallback.bind(this, true, f.bind(this)));
        } else {
          resolve();
        }
      };
      f.call(this);
    }.bind(this));
  };

  // *sob* learn promises
  // TODO: this is probably the same as installRootNodes
  this.installNodes = function(nodes) {
    return new Promise(function(resolve, reject) {
      var f = function() {
        if (nodes.length) {
          var node = nodes.pop();
          var params = {
            'ext_pub_b58': node.extendedPublicBase58,
            'ext_prv_enc': node.extendedPrivateEncrypted,
          };
          postRPCWithCallback(
            'restore-node',
            params,
            this.setNodeCallback.bind(this, false, f.bind(this)));
        } else {
          resolve();
        }
      };
      f.call(this);
    }.bind(this));
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

  this.decryptSecrets = function(callback) {
    if (!this.credentials.isKeyAvailable()) {
      console.log("can't decrypt without key");
      callback.call(this, false);
      return;
    }
    if (this.extendedPrivateBase58Encrypted) {
      this.credentials.decrypt(
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

  this.setNodeCallback = function(isRoot, callbackBool, response) {
    if (response.fp) {
      var node = Node.fromStorableObject(response);
      if (isRoot) {
        this.rootNodes.push(node);
      } else {
        this.nodes.push(node);
      }
      callbackBool.call(callbackBool, true);
    } else {
      callbackBool.call(callbackBool, false);
    }
  };

  this.addRandomMasterKey = function(callbackVoid) {
    postRPCWithCallback('generate-root-node', {},
                        this.setNodeCallback.bind(this, true, callbackVoid));
  };

  this.importMasterKey = function(ext_prv_b58, callbackBool) {
    var params = {
      'ext_prv_b58': ext_prv_b58
    };
    postRPCWithCallback('import-root-node', params,
                        this.setNodeCallback.bind(this, true, callbackBool));
  }

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
    if (!this.credentials.isKeyAvailable()) {
      console.log("can't remove master key; wallet is locked");
      return;
    }
    this.rootNodes = [];
  };

  this.deriveNextAccount = function(isWatchOnly, callbackBool) {
    var params = {
      'path': "m/0'",
      'isWatchOnly': isWatchOnly,
    };
    postRPCWithCallback('derive-child-node', params,
                        this.setNodeCallback.bind(this, false, callbackBool));
  };

  this.getAccountCount = function() {
    return this.nodes.length;
  };

  this.STORAGE_NAME = 'wallet';
  this.load = function() {
    return new Promise(function(resolve, reject) {
      var success = function(response) {
        if (response) {
          this.loadStorableObject(response).then(function() { resolve(); });
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
