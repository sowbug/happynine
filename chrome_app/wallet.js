var callbacks = {};
var callbackId = 1;

var bitcoinWalletApp = angular.module('bitcoinWalletApp', []);
var settings = new Settings();
settings.load();

var postMessageWithCallback = function(message, callback) {
  message.id = callbackId++;
  callbacks[message.id] = callback;
  common.naclModule.postMessage(JSON.stringify(message));
  console.log(message);
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
  this.balance = number * 700000000 + 12345678;
  this.addresses = [];
  this.transactions = [
    { 'date': '3 Jan 2009', 'address': '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa', 'amount': 234000000 },
    { 'date': '4 Jan 2009', 'address': '1JKMcZibd5MBn3sn4u3WVnFvHWMFiny59', 'amount': 500000000 },
    { 'date': '5 Jan 2009', 'address': '17sz256snXYak5VMX8EdE4p4Pab8X8iMGn', 'amount': -500000000 }
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
          'balance': 12345678, 'tx_count': 3 });
    }
    account.$scope.$apply();
  });

  this.hasTransactions = function() {
    return this.transactions.length > 0;
  }
}

function Settings() {
  var thisSettings = this;
  this.units = "mbtc";
  this.availableUnits = {
    "btc": "BTC",
    "mbtc":"mBTC",
    "ubtc": "uBTC",
    "sats": "Satoshis"
  };

  // These are here only for data bindings. Never serialize!
  this.currentPassphrase = null;
  this.newPassphrase = null;
  this.confirmPassphrase = null;

  this.load = function() {
    if (chrome && chrome.storage) {
      chrome.storage.local.get("settings", function(items) {
        var loadedSettings = items.settings;
        if (loadedSettings) {
          thisSettings.units = loadedSettings.units;
          thisSettings.passphraseSalt = loadedSettings.passphraseSalt;
          thisSettings.$scope.$apply();
        }
      });
    }
  };

  this.save = function() {
    if (chrome && chrome.storage) {
      var settingsToSave = {};
      settingsToSave.units = thisSettings.units;
      settingsToSave.passphraseSalt = thisSettings.passphraseSalt;
      chrome.storage.local.set({'settings': settingsToSave}, function() {
      });
    }
  };

  this.isPassphraseSet = function() {
    return thisSettings.passphraseSalt;
  };

  this.setPassphrase = function() {
    console.log(thisSettings.newPassphrase);
    console.log(thisSettings.confirmPassphrase);

    if (thisSettings.newPassphrase &&
        thisSettings.newPassphrase.length > 0 &&
        thisSettings.newPassphrase == thisSettings.confirmPassphrase) {
      console.log("here");
      var message = {
        'command': 'derive-key',
        'passphrase': thisSettings.newPassphrase
      };

      // We don't want these items lurking in the DOM.
      thisSettings.currentPassphrase = null;
      thisSettings.newPassphrase = null;
      thisSettings.confirmPassphrase = null;

      postMessageWithCallback(message, function(response) {
        // TODO(miket): key should be cleared after something like 5 min.
        thisSettings.passphraseKey = response.key;
        thisSettings.passphraseSalt = response.salt;
        thisSettings.$scope.$apply();
        thisSettings.save();
      });
    }
  };

  this.satoshiToUnit = function(satoshis) {
    switch (thisSettings.units) {
    case "btc":
      return satoshis / 100000000;
    case "mbtc":
       return satoshis / 100000;
    case "ubtc":
       return satoshis / 100;
    default:
      return satoshis;
    }
  };

  this.unitLabel = function() {
    return thisSettings.availableUnits[thisSettings.units];
  };
}

function WalletController($scope) {
  $scope.masterKey = null;
  $scope.account = null;
  $scope.settings = settings;
  settings.$scope = $scope;

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
      $('#settings-tabs a:first').tab('show');
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
  $('#settings-tabs a').click(onTabClick);

  // For development in a normal browser tab.
  if (chrome && chrome.runtime)
    showLoading();
  else
    hideLoading();
};
