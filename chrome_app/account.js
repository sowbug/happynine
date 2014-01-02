// Copyright 2014 Mike Tsao <mike@sowbug.com>

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

function Account($scope, index) {
  this.$scope = $scope;
  this.masterKey = $scope.masterKey;
  this.index = index;
  this.balance = 0;
  this.addresses = [];
  this.transactions = [
    { 'date': '3 Jan 2009', 'address': '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa', 'amount': 234000000 },
    { 'date': '4 Jan 2009', 'address': '1JKMcZibd5MBn3sn4u3WVnFvHWMFiny59', 'amount': 500000000 },
    { 'date': '5 Jan 2009', 'address': '17sz256snXYak5VMX8EdE4p4Pab8X8iMGn', 'amount': -500000000 }
  ];

  var account = this;
  var message = { 'command': 'get-addresses',
                  'seed_hex': this.masterKey.xprv,
                  'path': "m/" + index + "'/0",
                  'start': 0, 'count': 5 };
  postMessageWithCallback(message, function(response) {
    for (var i in response.addresses) {
      var address = response.addresses[i];
      account.addresses.push(
        { 'index': address.index, 'address': address.address,
          'balance': 0, 'tx_count': 0 });
    }
    account.calculateBalance();
    account.$scope.$apply();
  });

  this.calculateBalance = function() {
    account.balance = 0;
    for (var i in account.addresses) {
      var address = account.addresses[i];
      account.balance += address.balance;
    }
  };

  this.hasTransactions = function() {
    return this.transactions.length > 0;
  };
}
