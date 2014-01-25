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

var walletAppController = function($scope,
                                   $http,
                                   settings,
                                   credentials,
                                   wallet,
                                   electrum) {
  $scope.settings = settings;
  $scope.credentials = credentials;
  $scope.wallet = wallet;
  $scope.electrum = electrum;

  // For some crazy reason, angularjs won't reflect view changes in
  // the model's scope-level objects, so I have to create another
  // object and hang stuff off it. I picked w for wallet.
  $scope.w = {};
  $scope.w.showPrivateKey = false;

  if (chrome && chrome.runtime) {
    var manifest = chrome.runtime.getManifest();
    $scope.app_name = manifest.name;
    $scope.app_version = "v" + manifest.version;
  } else {
    $scope.app_name = "[name]";
    $scope.app_version = "v0.0.0.0";
  }

  $scope.addPostLoadWatchers = function() {
    // It's important not to add these watchers before initial load.
    $scope.$watch("settings", function(newVal, oldVal) {
      if (newVal != oldVal) {
        $scope.settings.save();
      }
    }, true);

    $scope.$watch("credentials", function(newVal, oldVal) {
      if (newVal != oldVal) {
        $scope.credentials.save();
      }
    }, true);

    $scope.$watch("wallet", function(newVal, oldVal) {
      if (newVal != oldVal) {
        // A small hack: if the wallet ever changes, it's a good
        // time to hide this checkbox.
        $scope.w.showPrivateKey = false;
        $scope.wallet.save();
      }
    }, true);

    $scope.$watchCollection('getAccounts()', function(newItems, oldItems) {
      if (newItems != oldItems) {
        $scope.wallet.save();
      }
    });
  };

  $scope.startLoading = function() {
    $scope.settings.load().then(function() {
      return $scope.credentials.load();
    }).then(function() {
      return $scope.wallet.load();
    }).then(function() {
      $scope.$apply();
      $scope.addPostLoadWatchers();
    }).catch(function(err) {
      console.log("error", err);
    });
  };

  $scope.$watch('getAccountCount()', function(newVal, oldVal) {
    if (newVal > 0 && oldVal == 0) {
      $scope.selectFirstAccount();
    }
  });

  $scope.$watch(
    'getWalletKeyFingerprint()',
    function(newVal, oldVal) {
      if (newVal != oldVal) {
        if (newVal) {
          var xhr = new XMLHttpRequest();
          xhr.open('GET', 'http://robohash.org/' + newVal +
                   '.png?set=set3&bgset=any&size=64x64', true);
          xhr.responseType = 'blob';
          xhr.onload = function(e) {
            $("#master-key-fingerprint-img").attr(
              "src",
              window.webkitURL.createObjectURL(this.response));
          };
          xhr.send();
        } else {
          $("#master-key-fingerprint-img").attr("src", "");
        }
      }
    }
  );

  $scope.$watch(
    'getCurrentAccount().isDerivationComplete',
    function(newVal, oldVal) {
      if (newVal != oldVal) {
        if (newVal) {
          var account = $scope.getCurrentAccount();
          account.retrieveAllTransactions($scope.electrum);
        }
      }
    }
  );

  $scope.$watch(
    'getCurrentAccountFingerprint()',
    function(newVal, oldVal) {
      if (newVal != oldVal) {
        if (newVal) {
          var xhr = new XMLHttpRequest();
          xhr.open('GET', 'http://robohash.org/' + newVal +
                   '.png?set=set3&bgset=any&size=64x64', true);
          xhr.responseType = 'blob';
          xhr.onload = function(e) {
            $("#account-fingerprint-img").attr(
              "src",
              window.webkitURL.createObjectURL(this.response));
          };
          xhr.send();
        } else {
          $("#account-fingerprint-img").attr("src", "");
        }
      }
    });

  $scope.newMasterKey = function() {
    $scope.wallet.addRandomMasterKey(function() {
      $scope.$apply();
    });
  };

  $scope.importMasterKey = function() {
    $scope.wallet.importMasterKey(
      $scope.w.importMasterKey,
      function(succeeded) {
        if (succeeded) {
          $scope.w.importMasterKey = null;
          $("#import-master-key-modal").modal('hide');
        }
        $scope.$apply();
      });
  };

  $scope.removeMasterKey = function() {
    // TODO(miket): ideally we'll track whether this key was backed
    // up, and make this button available only if yes. Then we'll
    // confirm up the wazoo before actually deleting.
    //
    // Less of a big deal if the master key is public.
    $scope.wallet.removeMasterKey();
  };

  $scope.nextAccountName = function() {
    return "Account " + $scope.wallet.getNextAccountNumber();
  };

  $scope.generateNextAccount = function() {
    $scope.wallet.deriveNextAccount(false, function(succeeded) {
      if (succeeded) {
        $scope.$apply();
      }
    });
  };

  $scope.watchNextAccount = function() {
    $scope.wallet.deriveNextAccount(true, function(succeeded) {
      if (succeeded) {
        $scope.$apply();
      }
    });
  };

  $scope.unlockWallet = function() {
    var unlockCallback = function(succeeded) {
      if (succeeded) {
        $("#unlock-wallet-modal").modal('hide');
      }
      $scope.w.passphraseNew = null;
      $scope.$apply();
    };

    var relockCallback = function() {
      //$scope.w.showPrivateKey = false;
      $scope.$apply();
    };

    $scope.credentials.unlock(
      $scope.w.passphraseNew,
      relockCallback.bind(this),
      unlockCallback.bind(this));
  };

  $scope.lockWallet = function() {
    $scope.credentials.lock(function() {
      $scope.$apply();
    });
  };

  $scope.setPassphrase = function() {
    // TODO: angularjs can probably do this check for us
    if (!$scope.w.passphraseNew || $scope.w.passphraseNew.length == 0) {
      console.log("missing new passphrase");
      return;
    }
    if ($scope.w.passphraseNew != $scope.w.passphraseConfirm) {
      console.log("new didn't match confirm:" + $scope.w.passphraseNew);
      return;
    }

    $scope.credentials.setPassphrase($scope.w.passphraseNew,
                                     function() { $scope.$apply(); },
                                     function(succeeded) {
                                       $scope.$apply();
                                     });

    // We don't want these items lurking in the DOM.
    $scope.w.passphraseNew = null;
    $scope.w.passphraseConfirm = null;
  };

  $scope.reinitializeEverything = function() {
    $scope.settings.init();
    $scope.credentials.init();
    $scope.wallet.init();
  };

  $scope.clearEverything = function() {
    // TODO(miket): confirmation
    $scope.reinitializeEverything();
    clearAllStorage();
    chrome.runtime.reload();
  };

  $scope.isPassphraseSet = function() {
    return $scope.credentials.isPassphraseSet();
  };

  $scope.isWalletUnlocked = function() {
    return $scope.isPassphraseSet() && !$scope.credentials.isWalletLocked();
  };

  $scope.isWalletKeySet = function() {
    return $scope.wallet.isKeySet();
  };

  $scope.isWalletKeyPrivateSet = function() {
    return $scope.wallet.isExtendedPrivateSet();
  };

  $scope.getWalletKeyPrivate = function() {
    return $scope.wallet.getExtendedPrivateBase58();
  };

  $scope.getWalletKeyPublic = function() {
    return $scope.wallet.getExtendedPublicBase58();
  };

  $scope.getWalletKeyFingerprint = function() {
    return $scope.wallet.getFingerprint();
  };

  $scope.getAccountCount = function() {
    return $scope.wallet.getAccountCount();
  };

  $scope.getAccounts = function() {
    return $scope.wallet.getAccounts();
  };

  $scope.getCurrentAccount = function() {
    return $scope.w.currentAccount;
  };

  $scope.getCurrentAccountFingerprint = function() {
    if (!$scope.getCurrentAccount()) {
      return null;
    }
    return $scope.getCurrentAccount().fingerprint;
  };

  $scope.areRequestsPending = function() {
    return $scope.electrum.areRequestsPending();
  };

  $scope.______refreshAccount = function() {
    $scope.w.currentAccount.fetchAddresses(function() {
      $scope.w.currentAccount.fetchBalances($http,
                                            function(succeeded) {});
      $scope.w.currentAccount.fetchUnspent($http,
                                           function(succeeded) {});
    });
  };

  $scope.setCurrentAccountByIndex = function(index) {
    if ($scope.w.currentAccount == $scope.getAccounts()[index]) {
      return;
    }
    $scope.w.currentAccount = $scope.getAccounts()[index];
   // $scope.refreshAccount();
  };

  $scope.selectFirstAccount = function() {
    if ($scope.getAccountCount() > 0) {
      $scope.setCurrentAccountByIndex(0);
    }
  };

  $scope.send = function() {
    var sendTo = $scope.w.sendTo;
    var sendValue = $scope.unitToSatoshi($scope.w.sendValue);
    var sendFee = $scope.unitToSatoshi($scope.w.sendFee);

    if (sendValue > 0 && sendFee > 0 && sendTo.length > 25) {
      console.log("send", sendTo, sendValue, $scope.unitLabel(),
                  "for", sendFee);
      $scope.getCurrentAccount().sendFunds($http,
                                           sendTo,
                                           sendValue,
                                           sendFee,
                                           function() {
                                             $scope.$apply();
                                           });
    }
  };

  $scope.satoshiToUnit = function(satoshis) {
    return $scope.settings.satoshiToUnit(satoshis);
  };

  $scope.unitToSatoshi = function(units) {
    return $scope.settings.unitToSatoshi(units);
  };

  $scope.unitLabel = function() {
    return $scope.settings.unitLabel();
  };

  // TODO(miket): this might be a race with moduleDidLoad.
  var listenerDiv = document.getElementById('listener');
  listenerDiv.addEventListener('load', function() {
    $scope.startLoading();
  }, true);
};
