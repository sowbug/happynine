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

function handleMessage(message) {
  var message_object = JSON.parse(message.data);
  if (shouldLog)
    console.log(message_object);
  var id = message_object.id;
  if (id) {
    if (callbacks[id]) {
      if (message_object.error) {
        logRpcError(message_object.error);
        callbacks[id].reject(message_object.error);
      } else {
        callbacks[id].resolve(message_object.result);
      }
      delete callbacks[id];
    } else {
      console.log("strange: unrecognized id", message_object);
    }
  } else {
    console.log("posting", message_object);
    $.event.trigger({
      'type': message_object.method,
      'message': message_object.result,
      time: new Date(),
    });


//    $(document).on("invalidFormData", function (evt) {
  //    $('txtForSubmissionAttempts').val( "Event fired from "
    //                               + evt.currentTarget.nodeName + " at "
      //                                   + evt.time.toLocaleString() + ": " + evt.message
        //                               );
  //  });

  }
}
