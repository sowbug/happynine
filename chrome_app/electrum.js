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

/**
 * @constructor
 */
function Electrum() {
  this.SERVERS = [
    "b.1209k.com"
  ];
  this.currentServerHostname = this.SERVERS[0];
  this.callbacks = {};
  this.callbackId = 1;
  this.socketId = undefined;
  this.stringBuffer = "";

  this.issueAddressGetHistory = function(addr_b58) {
    return new Promise(function(resolve, reject) {
      this._enqueueRpc("blockchain.address.get_history", [addr_b58])
        .then(resolve);
    }.bind(this));
  };

  this.issueAddressSubscribe = function(addr_b58) {
    return new Promise(function(resolve, reject) {
      this._enqueueRpc("blockchain.address.subscribe", [addr_b58])
        .then(resolve);
    }.bind(this));
  };

  this.issueTransactionGet = function(tx_hash) {
    return new Promise(function(resolve, reject) {
      this._enqueueRpc("blockchain.transaction.get", [tx_hash])
        .then(resolve);
    }.bind(this));
  };

  this.issueTransactionBroadcast = function(tx) {
    return new Promise(function(resolve, reject) {
      this._enqueueRpc("blockchain.transaction.broadcast", [tx])
        .then(resolve);
    }.bind(this));
  };

  this.issueHeadersSubscribe = function() {
    return new Promise(function(resolve, reject) {
      this._enqueueRpc("blockchain.headers.subscribe", [])
        .then(resolve);
    }.bind(this));
  };

  this.issueBlockGetHeader = function(block_num) {
    return new Promise(function(resolve, reject) {
      this._enqueueRpc("blockchain.block.get_header", [block_num])
        .then(resolve);
    }.bind(this));
  };

  this.handleResponse = function(o) {
    var id = o["id"];
    if (this.callbacks[id]) {
      this.callbacks[id].resolve(o["result"]);
      delete this.callbacks[id];
      this.pendingRpcCount--;
      this.$scope.$apply();
    } else {
      logInfo("notification from electrum", o);
      var ALLOWED_METHODS = [
        "blockchain.address.subscribe",
        "blockchain.headers.subscribe",
        "blockchain.numblocks.subscribe"
      ];
      if (ALLOWED_METHODS.indexOf(o["method"]) != -1) {
        $.event.trigger({
          "type": o["method"],
          "message": o["params"],
          time: new Date()
        });
      }
    }
  };

  this.onSocketReceive = function(receiveInfo) {
    arrayBuffer2String(receiveInfo.data, function(str) {
      this.stringBuffer += str;
      var parts = this.stringBuffer.split("\n");
      if (parts.length > 1) {
        for (var i = 0; i < parts.length - 1; ++i) {
          var part = parts[i];
          if (part.length == 0) {
            continue;
          }
          this.handleResponse(JSON.parse(part));
        }
        if (parts[parts.length - 1].length > 0) {
          this.stringBuffer = parts[parts.length - 1];
        }
      }
    }.bind(this));
  };

  this.onSocketReceiveError = function(receiveErrorInfo) {
    logFatal("receive error", receiveErrorInfo);
  };

  this.onSendComplete = function(sendInfo) {
  };

  this.connectToServer = function() {
    return new Promise(function(resolve, reject) {
      function onConnectComplete(result) {
        if (result != 0) {
          logFatal("onConnectComplete", result);
          reject(result);
        } else {
          resolve();
        }
      };

      function onSocketCreate(socketInfo) {
        this.socketId = socketInfo.socketId;
        chrome.sockets.tcp.onReceive.addListener(
          this.onSocketReceive.bind(this));
        chrome.sockets.tcp.onReceiveError.addListener(
          this.onSocketReceiveError.bind(this));
        chrome.sockets.tcp.connect(this.socketId,
                                   this.currentServerHostname,
                                   50001,
                                   onConnectComplete.bind(this));
      }

      chrome.sockets.tcp.create({
        "name": "electrum",
        "persistent": true,
        "bufferSize": 16384
      }, onSocketCreate.bind(this));
    }.bind(this));
  }

  // TODO(miket): there's just no way this will work
  this.pendingRpcCount = 0;

  this.areRequestsPending = function() {
    return this.pendingRpcCount > 0;
  };

  this._enqueueRpc = function(method, params) {
    return new Promise(function(resolve, reject) {
      var rpc = {
        "id": this.callbackId++,
        "method": method,
        "params": params
      };
      string2ArrayBuffer(
        JSON.stringify(rpc) + "\n",
        function(arrayBuffer) {
          chrome.sockets.tcp.send(this.socketId,
                                  arrayBuffer,
                                  this.onSendComplete.bind(this));
        }.bind(this));
      this.callbacks[rpc["id"]] = {"resolve": resolve, "reject": reject};
      this.pendingRpcCount++;
    }.bind(this));
  };
}
