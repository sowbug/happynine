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

function Account($scope, $http, index) {
  this.$scope = $scope;
  this.index = index;
  this.balance = 0;
  this.addresses = [];
  this.addressMap = {};
  this.transactions = [];
  this.nextAddress = 0;
  this.batchCount = 20;

  var account = this;
  var message = {
    'command': 'get-addresses',
    'seed': $scope.credentials.accountXprvIfAvailable(index),
    'path': "m/0",  // external addresses
    'start': this.nextAddress, 'count': this.batchCount };
  postMessageWithCallback(message, function(response) {
    var balanceURL = ['https://blockchain.info/multiaddr?active='];

    for (var i in response.addresses) {
      var address = response.addresses[i];
      account.addressMap[address.address] = { 'index': address.index,
                                              'address': address.address,
                                              'key': address.key,
                                              'balance': 0,
                                              'tx_count': 0 };
      if (balanceURL.length > 1) {
        balanceURL.push('|');
      }
      balanceURL.push(address.address);
    }
    balanceURL.push('&format=json');
    balanceURL = balanceURL.join('');

    $http({method: 'GET', url: balanceURL}).
      success(function(data, status, headers, config) {
        for (var i in data.addresses) {
          var addr = data.addresses[i];
          account.addressMap[addr.address].balance = addr.final_balance;
          account.addressMap[addr.address].tx_count = addr.n_tx;
        }
        for (var i in data.txs) {
          var tx = data.txs[i];
          account.transactions.push({
            'date': tx.time,
            'address': tx.inputs[0].prev_out.addr,
            'hash': tx.hash,
            'amount': Math.floor(Math.random() * 1000000000)
          });
        }
        account.calculateBalance();
       }).
      error(function(data, status, headers, config) {
        console.log("error", status, data);
        account.calculateBalance();
      });
  });

  this.calculateBalance = function() {
    this.balance = 0;
    this.addresses = [];
    for (var i in this.addressMap) {
      var address = this.addressMap[i];
      this.balance += address.balance;
      this.addresses.push(address);
    }
  };

  this.hasTransactions = function() {
    return this.transactions.length > 0;
  };

  this.name = function() {
    if (this.index > 0) {
      return "Account " + this.index;
    } else {
      return "Default Account";
    }
  };

  this.setFingerprint = function(fingerprint) {
    this.fingerprint = fingerprint;
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'http://robohash.org/' + fingerprint +
             '.png?set=set3&bgset=any&size=64x64', true);
    xhr.responseType = 'blob';
    xhr.onload = function(e) {
      $("#account-fingerprint-img").attr(
        "src",
        window.webkitURL.createObjectURL(this.response));
    };
    xhr.send();
  };
  this.setFingerprint($scope.credentials.accounts[index].fingerprint);
}
