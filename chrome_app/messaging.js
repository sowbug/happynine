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

var postRPCWithCallback = function(method, params, callback) {
  var rpc = { 'jsonrpc': '2.0',
              'id': callbackId++,
              'method': method,
              'params': params,
            };
  callbacks[rpc.id] = callback;
  common.naclModule.postMessage(JSON.stringify(rpc));
  if (shouldLog)
    console.log(rpc);
};

function handleMessage(message) {
  var message_object = JSON.parse(message.data);
  if (shouldLog)
    console.log(message_object);
  var id = message_object.id;
  if (callbacks[id]) {
    callbacks[id].call(this, message_object.result);
    delete callbacks[id];
  } else {
    console.log("strange: unrecognized id", message_object);
  }
}
