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

// A Node is a node in a BIP0032 tree. If it's a master node, its only
// power is to mint child nodes. Otherwise it generates either child
// nodes or addresses.

/**
 * @constructor
 */
function Node() {
  this.init = function() {
    this.childNum = undefined;
    this.extendedPrivateEncrypted = undefined;
    this.extendedPublicBase58 = undefined;
    this.fingerprint = undefined;
    this.parentFingerprint = undefined;
    this.path = undefined;

    this.nextChangeAddressIndex = 8;
    this.nextPublicAddressIndex = 8;

    this.publicAddresses = {};
    this.changeAddresses = {};
    this.balance = 0;
    this.batchCount = 8;
    this.hexId = undefined;
    this.nextAddress = 0;
    this.transactions = [];
    this.unspent_txos = [];  // unserialized
  };
  this.init();

  this.toStorableObject = function() {
    var o = {};
    o["child_num"] = this.childNum;
    o["ext_prv_enc"] = this.extendedPrivateEncrypted;
    o["ext_pub_b58"] = this.extendedPublicBase58;
    o["fp"] = this.fingerprint;
    o["next_change_addr"] = this.nextChangeAddressIndex;
    o["next_pub_addr"] = this.nextPublicAddressIndex;
    o["pfp"] = this.parentFingerprint;
    o["path"] = this.path;
    return o;
  };

  this.isMaster = function() {
    return this.childNum == 0;
  };

  this.hasPrivateKey = function() {
    return !!this.extendedPrivateEncrypted;
  };

  this.prettyChildNum = function() {
    if (this.childNum >= 0x80000000) {
      return this.childNum - 0x80000000;
    }
    return this.childNum;
  };
}

Node.fromStorableObject = function(o) {
  var s = new Node();

  if (o["child_num"] != undefined)
    s.childNum = o["child_num"];
  if (o["ext_prv_enc"] != undefined)
    s.extendedPrivateEncrypted = o["ext_prv_enc"];
  if (o["ext_pub_b58"] != undefined)
    s.extendedPublicBase58 = o["ext_pub_b58"];
  if (o["fp"] != undefined)
    s.fingerprint = o["fp"];
  if (o["next_change_addr"] != undefined)
    s.nextChangeAddressIndex = o["next_change_addr"];
  if (o["next_pub_addr"] != undefined)
    s.nextPublicAddressIndex = o["next_pub_addr"];
  if (o["path"] != undefined)
    s.path = o["path"];
  if (o["pfp"] != undefined)
    s.parentFingerprint = o["pfp"];

  return s;
};
