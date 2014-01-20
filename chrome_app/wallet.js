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

  this.loadStorableObject = function(o, callback) {
    this.init();
    this.rootNodes = [];
    for (var i in o.rnodes) {
      this.rootNodes.push(Node.fromStorableObject(o.rnodes[i]));
    }
    this.nodes = [];
    for (var i in o.nodes) {
      this.nodes.push(Node.fromStorableObject(o.nodes[i]));
    }
    this.deriveNodes(callback);
  }

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

  this.unlock = function(passphrase, relockCallback, callback) {
    var f = function(succeeded) {
      if (succeeded) {
        this.unlockNodes(this.credentials, callback);
      } else {
        callback.call(this, false);
      }
    };
    this.credentials.generateAndCacheKeys(
      passphrase,
      relockCallback,
      f.bind(this));
  };

  this.lock = function() {
    this.credentials.clearCachedKeys();
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
      !!this.rootNodes[0].extendedPrivateBase58Encrypted;
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

  this.addRandomMasterKey = function(callback) {
    postRPCWithCallback('create-node', {}, function(response) {
      this.addMasterKey(response.ext_prv_b58, callback);
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

  this.addMasterKey = function(ext_b58, callback) {
    var params = {
      'ext_b58': ext_b58
    };
    postRPCWithCallback(
      'get-node',
      params,
      Node.fromGetNodeResponse.bind(
        this,
        this.credentials,
        false,
        this.addNewNode.bind(this, true, callback)));
  }

  this.removeMasterKey = function() {
    if (!this.credentials.isKeyAvailable()) {
      console.log("can't remove master key; wallet is locked");
      return;
    }
    this.rootNodes = [];
  };

  this.deriveNextAccount = function(isWatchOnly, callback) {
    if (!this.credentials.isKeyAvailable()) {
      console.log("Can't derive account when wallet is unlocked");
      // TODO(miket): all these synchronous callbacks need to be async
      callback(this, false);
      return;
    }
    if (!this.isExtendedPrivateSet()) {
      console.log("Can't derive account without root private key");
      // Callers are depending on this method to be asynchonous.
      window.setTimeout(callback.bind(this, false), 0);
      return;
    }

    this.rootNodes[0].deriveChildNode(
      this.credentials,
      0,  // TODO
      isWatchOnly,
      this.addNewNode.bind(this, false, callback.bind(this, true)));
  };

  this.getAccountCount = function() {
    return this.nodes.length;
  };

  this.load = function(callback) {
    loadStorage2('wallet', function(object) {
      if (object) {
        this.loadStorableObject(object, callback);
      } else {
        this.init();
        callback.call(this);
      }
    }.bind(this));
  };

  this.save = function() {
    saveStorage2('wallet', this.toStorableObject());
  };
}
