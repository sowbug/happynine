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

// ApiClient knows how to talk to the NaCl module.

/**
 * @constructor
 */
function ApiClient() {
}

ApiClient.prototype.describeNode = function(extendedPublicBase58) {
  return new Promise(function(resolve, reject) {
    var params = {
      'ext_pub_b58': extendedPublicBase58
    };
    postRPC('describe-node', params).then(resolve);
  });
};

ApiClient.prototype.describePrivateNode = function(extendedPrivateEncrypted) {
  return new Promise(function(resolve, reject) {
    var params = {
      'ext_prv_enc': extendedPrivateEncrypted
    };
    postRPC('describe-private-node', params).then(resolve);
  });
};

ApiClient.prototype.restoreNode = function(extendedPublicBase58,
                                           extendedPrivateEncrypted) {
  return new Promise(function(resolve, reject) {
    var params = {
      'ext_pub_b58': extendedPublicBase58,
      'ext_prv_enc': extendedPrivateEncrypted
    };
    postRPC('restore-node', params).then(resolve);
  });
};

ApiClient.prototype.generateMasterNode = function() {
  return new Promise(function(resolve, reject) {
    postRPC('generate-master-node', {}).then(resolve);
  });
};

ApiClient.prototype.importMasterNode = function(extendedPrivateBase58) {
  return new Promise(function(resolve, reject) {
    var params = {
      'ext_prv_b58': extendedPrivateBase58
    };
    postRPC('import-master-node', params).then(resolve);
  });
};

ApiClient.prototype.deriveChildNode = function(childNum, isWatchOnly) {
  return new Promise(function(resolve, reject) {
    var params = {
      'path': "m/" + childNum + "'",
      'is_watch_only': isWatchOnly
    };
    postRPC('derive-child-node', params).then(resolve);
  });
};

ApiClient.prototype.createTx = function(recipients, fee, shouldSign) {
  return new Promise(function(resolve, reject) {
    var params = {
      'recipients': recipients,  // [{'addr_b58': sendTo, 'value': sendValue}]
      'fee': fee,
      'sign': shouldSign
    };
    postRPC('create-tx', params).then(resolve);
  });
};

ApiClient.prototype.reportTxStatuses = function(tx_statuses) {
  return new Promise(function(resolve, reject) {
    var params = {
      'tx_statuses': tx_statuses
    };
    postRPC('report-tx-statuses', params).then(resolve);
  });
};

ApiClient.prototype.reportTxs = function(txs) {
  return new Promise(function(resolve, reject) {
    var params = {
      'txs': txs
    };
    postRPC('report-txs', params).then(resolve);
  });
};

ApiClient.prototype.getAddresses = function() {
  return new Promise(function(resolve, reject) {
    postRPC('get-addresses', {}).then(resolve);
  });
};

ApiClient.prototype.getHistory = function() {
  return new Promise(function(resolve, reject) {
    postRPC('get-history', {}).then(resolve);
  });
};

ApiClient.prototype.confirmBlock = function(blockHeight, timestamp) {
  return new Promise(function(resolve, reject) {
    var params = {
      'block_height': blockHeight,
      'timestamp': timestamp,
    };
    postRPC('confirm-block', params).then(resolve);
  });
};
