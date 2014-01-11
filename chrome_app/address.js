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

function Address() {
  this.init = function() {
    this.address = undefined;
    this.balance = 0;
    this.index = 0;
    this.key = undefined;
    this.path = undefined;
    this.tx_count = 0;
  };
  this.init();

  this.toStorableObject = function() {
    var o = {};
    o.address = this.address;
    o.balance = this.balance;
    o.index = this.index;
    o.path = this.path;
    o.tx_count = this.tx_count;

    return o;
  };
}

Address.fromStorableObject = function(o) {
  var s = new Address();

  if (o.address != undefined) s.address = o.address;
  if (o.balance != undefined) s.balance = o.balance;
  if (o.index != undefined) s.index = o.index;
  if (o.path != undefined) s.path = o.path;
  if (o.tx_count != undefined) s.tx_count = o.tx_count;

  // Items that shouldn't be passed from a truly persisted object, but
  // that might be serialized in-memory.
  if (o.key != undefined) s.key = o.key;

  return s;
};
