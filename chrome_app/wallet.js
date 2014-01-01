var callbacks = {};
var callbackId = 1;

var bitcoinWalletApp = angular.module('bitcoinWalletApp', []);

var postMessageWithCallback = function(message, callback) {
  message.id = callbackId++;
  callbacks[message.id] = callback;
  common.naclModule.postMessage(JSON.stringify(message));
};

function WalletController($scope) {
  $scope.addresses = [
  { 'address': '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa', 'balance': '123.45', 'tx_count': 3 },
  { 'address': '1JKMcZibd5MBn3sn4u3WVnFvHWMFiny59', 'balance': '3.50', 'tx_count': 1 },
  { 'address': '1ADc4zojfRp9794ZX3ozDSGF4W2meSfkzr', 'balance': '0.0001', 'tx_count': 2 },
  { 'address': '18gaduiJeXLcLJVcdhRAiMLge6YmCyAnQz', 'balance': '5.55', 'tx_count': 3 },
  { 'address': '1CSWk7EioDwpqUDr4gftsjnzewPUSRPs9p', 'balance': '0.00', 'tx_count': 0 },
  ];
  $scope.transactions = [
  { 'date': '3 Jan 2009', 'address': '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa', 'amount': '2.34' },
  { 'date': '4 Jan 2009', 'address': '1JKMcZibd5MBn3sn4u3WVnFvHWMFiny59', 'amount': '50.00' },
  { 'date': '5 Jan 2009', 'address': '17sz256snXYak5VMX8EdE4p4Pab8X8iMGn', 'amount': '(50.00)' }
  ];
  $scope.masterKey = {};
  $scope.masterKey.xprv = "xprvfoobarbaz";
  $scope.masterKey.showXprv = false;
  $scope.masterKey.xpub = "xpubfoobarbaz";
  $scope.masterKey.fingerprint = "0x12345678";

  $scope.newMasterKey = function() {
    var message = {
      'command': 'create-random-node'
    };
    postMessageWithCallback(message, function(response) {
      $scope.masterKey = {};
      $scope.masterKey.xprv = response.ext_prv_b58;
      $scope.masterKey.showXprv = false;
      $scope.masterKey.xpub = response.ext_pub_b58;
      $scope.masterKey.fingerprint = response.fingerprint;
      //    updateFingerprintImage(message_object.fingerprint);
    });
    common.naclModule.postMessage(JSON.stringify(message));
  };
}

function moduleDidLoad() {
  common.hideModule();
  hideLoading();
}

function hideLoading() {
  $("#loading-container").fadeOut(250, function() {
    $("#main-container").fadeIn(500, function() {
      $('#main-tabs a:first').tab('show');
    });
  });
}

function showLoading() {
  $("#loading-container").show();
  $("#main-container").hide();
}

function updateFingerprintImage() {
  var fingerprint = $("#fingerprint-readonly").val();
  var xhr = new XMLHttpRequest();
  xhr.open('GET', 'http://robohash.org/' + fingerprint +
   '.png?set=set3&bgset=any&size=64x64', true);
  xhr.responseType = 'blob';
  xhr.onload = function(e) {
    $("#fingerprint-img-big").attr(
      'src',
      window.webkitURL.createObjectURL(this.response));
    $("#fingerprint-img").attr(
      'src',
      window.webkitURL.createObjectURL(this.response));
  };
  xhr.send();
}

var refreshTransactionTable = function(transactions) {
  var table = $("#transaction-table");
  var noTransactions = $("#no-transactions");
  table.empty();

  if (transactions.length == 0) {
    table.hide();
    noTransactions.show();
  } else {
    table.show();
    noTransactions.hide();
    table.append($("<thead><tr><th>Date</th><th>Address</th><th>Amount</th></tr></thead>"));
    var tbody = $('<tbody></tbody>');
    for (var i in transactions) {
      var transaction = transactions[i];
      tbody.append($("<tr>" +
       "<td>" + transaction.date + "</td>" + 
       "<td>" + transaction.address + "</td>" + 
       "<td>" + transaction.amount + "</td>" +
       "</tr>"));
    }
    table.append(tbody);
  }
};

function handleMessage(message) {
  var message_object = JSON.parse(message.data);
  console.log(message);
  var id = message_object.id;
  if (callbacks[id]) {
    callbacks[id].call(message_object);
    delete callbacks[id];
  }
}

var onTabClick = function(e) {
  e.preventDefault()
  $(this).tab('show')
};

var onNewMasterKeyClick = function(e) {
  e.preventDefault();
  var message = {
    'command': 'create-random-node'
  };
  common.naclModule.postMessage(JSON.stringify(message));
};

var onShowPrivateKeyClick = function(e) {
  e.preventDefault();
  $("#xprv-wrapper").slideToggle();
};

var setAccount = function(account) {
  if (!account || account == 0) {
    $("#account").text("Default Account");
  } else {
    $("#account").text("Account " + account);
  }
};

var setMasterKey = function(key) {
  if (!key) {
 //   refreshTransactionTable([]);
} else {
  refreshTransactionTable([
    { 'date': '3 Jan 2009', 'address': '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa', 'amount': '2.34' },
    { 'date': '4 Jan 2009', 'address': '1JKMcZibd5MBn3sn4u3WVnFvHWMFiny59', 'amount': '50.00' },
    { 'date': '5 Jan 2009', 'address': '17sz256snXYak5VMX8EdE4p4Pab8X8iMGn', 'amount': '(50.00)' }
    ]);
}
setAccount();
};

window.onload = function() {
  // Add click handlers.
  $('#main-tabs a').click(onTabClick);
  $('#new-master-key').click(onNewMasterKeyClick);
  $('#xprv-show').click(onShowPrivateKeyClick);

  // Set up initial state.
  $("#xprv-wrapper").hide();
  setMasterKey();

  // document.querySelector('#get-wallet-node').onclick = function() {
  //   var message = {
  //     'command': 'get-wallet-node',
  //     'seed_hex': document.querySelector("#seed").value,
  //     'path': document.querySelector("#path").value
  //   };
  //   common.naclModule.postMessage(JSON.stringify(message));
  // };

  // For development in a normal browser tab.
  if (chrome && chrome.runtime)
    showLoading();
  else
    hideLoading();
};
