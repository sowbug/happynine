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

var callbacks = {};
var callbackId = 1;

var shouldLog = true;

var postRPC = function(method, params) {
  return new Promise(function(resolve, reject) {
    var rpc = { 'jsonrpc': '2.0',
                'id': callbackId++,
                'method': method,
                'params': params,
              };
    callbacks[rpc.id] = {'resolve': resolve, 'reject': reject};
    common.naclModule.postMessage(JSON.stringify(rpc));
    if (shouldLog)
      console.log(rpc);
  });
};

var checkForNotifications = function(response) {
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
  }
  if (response.tx_requests) {
    console.log("noticed tx_requests");
    var tx_requests = response.tx_requests;
    for (var i in tx_requests) {
      console.log(tx_requests[i]);
    }
    $.event.trigger({
      'type': 'tx_requests',
      'message': tx_requests,
      time: new Date(),
    });
  }
};

function handleMessage(message) {
  var o = JSON.parse(message.data);
  if (shouldLog)
    console.log(o);
  var id = o.id;
  if (id) {
    if (callbacks[id]) {
      if (o.error) {
        logRpcError(o.error);
        callbacks[id].reject(o.error);
      } else {
        callbacks[id].resolve(o.result);
        checkForNotifications(o.result);
      }
      delete callbacks[id];
    } else {
      console.log("strange: unrecognized id", o);
    }
  } else {
    console.log("posting", o);
    $.event.trigger({
      'type': o.method,
      'message': o.result,
      time: new Date(),
    });
  }
}
