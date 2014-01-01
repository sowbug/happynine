var callbacks = {};
var callbackId = 1;

var bitcoinWalletApp = angular.module('bitcoinWalletApp', []);

var postMessageWithCallback = function(message, callback) {
  message.id = callbackId++;
  callbacks[message.id] = callback;
  common.naclModule.postMessage(JSON.stringify(message));
};

function handleMessage(message) {
  var message_object = JSON.parse(message.data);
  console.log(message_object);
  var id = message_object.id;
  if (callbacks[id]) {
    callbacks[id].call(this, message_object);
    delete callbacks[id];
  }
}

function MasterKey() {
  this.showXprv = false;
  this.xprv = "";
  this.xpub = "";
  this.fingerprint = "";

  var mk = this;
  this.setFingerprint = function(fingerprint) {
    mk.fingerprint = fingerprint;
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'http://robohash.org/' + fingerprint +
             '.png?set=set3&bgset=any&size=64x64', true);
    xhr.responseType = 'blob';
    xhr.onload = function(e) {
      $("#fingerprint-img").attr(
        "src",
        window.webkitURL.createObjectURL(this.response));
      $("#fingerprint-img-big").attr(
        "src",
        window.webkitURL.createObjectURL(this.response));
    };
    xhr.send();
  };
}

function Account(number) {
  this.number = number;
  this.balance = 9999.12345678;
  this.addresses = [
  { 'address': '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa', 'balance': '123.45', 'tx_count': 3 },
  { 'address': '1JKMcZibd5MBn3sn4u3WVnFvHWMFiny59', 'balance': '3.50', 'tx_count': 1 },
  { 'address': '1ADc4zojfRp9794ZX3ozDSGF4W2meSfkzr', 'balance': '0.0001', 'tx_count': 2 },
  { 'address': '18gaduiJeXLcLJVcdhRAiMLge6YmCyAnQz', 'balance': '5.55', 'tx_count': 3 },
  { 'address': '1CSWk7EioDwpqUDr4gftsjnzewPUSRPs9p', 'balance': '0.00', 'tx_count': 0 },
];

  this.transactions = [
  { 'date': '3 Jan 2009', 'address': '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa', 'amount': '2.34' },
  { 'date': '4 Jan 2009', 'address': '1JKMcZibd5MBn3sn4u3WVnFvHWMFiny59', 'amount': '50.00' },
  { 'date': '5 Jan 2009', 'address': '17sz256snXYak5VMX8EdE4p4Pab8X8iMGn', 'amount': '(50.00)' }
];

  this.hasTransactions = function() {
    return this.transactions.length > 0;
  }
}

function Settings() {
  this.units = "mBTC";
}

function WalletController($scope) {
  $scope.masterKey = null;
  $scope.account = null;
  $scope.settings = new Settings();

  $scope.newMasterKey = function() {
    var message = {
      'command': 'create-random-node'
    };
    postMessageWithCallback(message, function(response) {
      var masterKey = new MasterKey();
      masterKey.xprv = response.ext_prv_b58;
      masterKey.showXprv = false;
      masterKey.xpub = response.ext_pub_b58;
      masterKey.setFingerprint(response.fingerprint);

      $scope.masterKey = masterKey;
      $scope.account = new Account(7);
      $scope.$apply();
    });
  };

  $scope.removeMasterKey = function() {
    // TODO(miket): ideally we'll track whether this key was backed
    // up, and make this button available only if yes. Then we'll
    // confirm up the wazoo before actually deleting.
    //
    // Less of a big deal if the master key is public.
    $scope.masterKey = null;
    $scope.$apply();
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

var onTabClick = function(e) {
  e.preventDefault()
  $(this).tab('show')
};

var setAccount = function(account) {
  if (!account || account == 0) {
    $("#account").text("Default Account");
  } else {
    $("#account").text("Account " + account);
  }
};

window.onload = function() {
  // Add click handlers.
  $('#main-tabs a').click(onTabClick);

  // For development in a normal browser tab.
  if (chrome && chrome.runtime)
    showLoading();
  else
    hideLoading();
};
