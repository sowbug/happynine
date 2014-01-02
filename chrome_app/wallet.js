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

function Account($scope, number) {
  this.$scope = $scope;
  this.masterKey = $scope.masterKey;
  this.number = number;
  this.balance = number * 7 + 0.12345678;
  this.addresses = [];
  this.transactions = [
    { 'date': '3 Jan 2009', 'address': '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa', 'amount': '2.34' },
    { 'date': '4 Jan 2009', 'address': '1JKMcZibd5MBn3sn4u3WVnFvHWMFiny59', 'amount': '50.00' },
    { 'date': '5 Jan 2009', 'address': '17sz256snXYak5VMX8EdE4p4Pab8X8iMGn', 'amount': '(50.00)' }
  ];

  var account = this;
  var message = { 'command': 'get-addresses', 'seed_hex': this.masterKey.xprv,
                  'path': "m/" + number + "'/0",
                  'start': 0, 'count': 5 };
  postMessageWithCallback(message, function(response) {
    for (var i in response.addresses) {
      var address = response.addresses[i];
      account.addresses.push(
        { 'index': address.index, 'address': address.address,
          'balance': '123.45', 'tx_count': 3 });
    }
    account.$scope.$apply();
  });

  this.hasTransactions = function() {
    return this.transactions.length > 0;
  }
}

function Settings() {
  var settings = this;
  this.units = "mBTC";
  this.passphraseHash = null;

  this.isPassphraseSet = function() {
    return settings.passphraseHash;
  }

  this.setPassphrase = function(passphrase, confirm) {
    settings.passphraseHash = 'foobarbaz';
  }
}

function WalletController($scope) {
  $scope.masterKey = null;
  $scope.account = null;
  $scope.settings = new Settings();

  $scope.newMasterKey = function() {
    var message = {
      'command': 'create-node'
    };
    postMessageWithCallback(message, function(response) {
      var masterKey = new MasterKey();
      masterKey.xprv = response.ext_prv_b58;
      masterKey.showXprv = false;
      masterKey.xpub = response.ext_pub_b58;
      masterKey.setFingerprint(response.fingerprint);

      $scope.masterKey = masterKey;
      $scope.nextAccount();
      $scope.$apply();
    });
  };

  $scope.removeMasterKey = function() {
    // TODO(miket): ideally we'll track whether this key was backed
    // up, and make this button available only if yes. Then we'll
    // confirm up the wazoo before actually deleting.
    //
    // Less of a big deal if the master key is public.
    $scope.account = null;
    $scope.masterKey = null;
  };

  $scope.importMasterKey = function() {
    console.log("not implemented");
  };

  $scope.nextAccount = function() {
    if ($scope.account) {
      $scope.account =
        new Account($scope, $scope.account.number + 1);
    } else {
      $scope.account = new Account($scope, 0);
    }
  };

  $scope.prevAccount = function() {
    if ($scope.account) {
      if ($scope.account.number > 0) {
        $scope.account =
          new Account($scope, $scope.account.number - 1);
      }
    } else {
      $scope.account = new Account($scope, 0);
    }
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
