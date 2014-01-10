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
                                   wallet) {
  console.log("here I am", settings, credentials, wallet);
  $scope.settings = settings;
  $scope.credentials = credentials;
  $scope.wallet = wallet;

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
    $scope.$watchCollection("settings", function(newVal, oldVal) {
      if (newVal != oldVal) {
        $scope.settings.save();
      }
    });

    $scope.$watchCollection("credentials", function(newVal, oldVal) {
      if (newVal != oldVal) {
        $scope.credentials.save();
      }
    });

    $scope.$watchCollection("wallet.storable", function(newVal, oldVal) {
      if (newVal != oldVal) {
        // A small hack: if the wallet ever changes, it's a good
        // time to hide this checkbox.
        $scope.showPrivateKey = false;
        $scope.wallet.save();
      }
    });

    $scope.$watchCollection('getAccounts()', function(newItems, oldItems) {
      if (newItems != oldItems) {
        $scope.wallet.save();
      }
    });

    $scope.$watch('isWalletUnlocked()', function(newVal, oldVal) {
      if (newVal == oldVal) {
        return;
      }
      if (newVal) {
        this.wallet.decryptSecrets(function(succeeded) {
          if (succeeded) {
            $scope.$apply();
          }
        });
      }
    }.bind(this));
  };

  $scope.startLoading = function() {
    $scope.settings.load(function() {
      $scope.credentials.load(function() {
        $scope.wallet.load(function() {

          // TODO(miket): I hate this, but I can't get
          // $scope.credentials.accounts to be watchable.
          if ($scope.credentials.accounts.length > 0) {
            $scope.credentials.accountChangeCounter++;
          }

          $scope.$apply();
          $scope.addPostLoadWatchers();
        });
      });
    });
  };

  $scope.$watch('getAccountCount()', function(newVal, oldVal) {
    if (newVal > 0 && oldVal == 0) {
      $scope.selectFirstAccount();
    }
  });


  // Something about credentials.extended changed. We should generate
  // a MasterKey.
  $scope.$watch('credentials.digestMasterKey()',
                function(newVal, oldVal) {
                  if (newVal != oldVal) {
                    $scope.generateMasterKey();
                  }
                });

  $scope.$watch('credentials.needsAccountsRetrieval',
                function(newVal, oldVal) {
                  if (newVal != oldVal && newVal) {
                    $scope.credentials.retrieveAccounts(function() {
                      $scope.$apply();
                    });
                  }
                });

  $scope.$watch('credentials.accountChangeCounter',
                function(newVal, oldVal) {
                  if (newVal != oldVal) {
                    // TODO(miket): will probably change to
                    // lastAccount() when we have > 1.
                    $scope.firstAccount();
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
    });

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

  $scope.generateMasterKey = function() {
    if (!$scope.credentials.extendedPublicBase58) {
      console.log("updated master key to null");
      $scope.masterKey = null;
      return;
    }
    var b58 = $scope.credentials.extendedPublicBase58;
    if ($scope.credentials.extendedPrivateBase58) {
      b58 = $scope.credentials.extendedPrivateBase58;
    }
    var message = {
      'command': 'get-node',
      'seed': b58
    };
    postMessageWithCallback(message, function(response) {
      $scope.masterKey = new MasterKey(response.ext_pub_b58,
                                       response.ext_prv_b58,
                                       response.fingerprint);
      $scope.$apply();
    });
  };

  $scope.newMasterKey = function() {
    $scope.wallet.createRandomMasterKey(function() {
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

  $scope._nextAccount = function() {
    if ($scope.account) {
      $scope.account =
        new Account($scope,
                    $http,
                    $scope.account.index + 1,
                    $scope.masterKey);
    } else {
      $scope.account = new Account($scope, $http, 0, $scope.masterKey);
    }
  };

  $scope._prevAccount = function() {
    if ($scope.account) {
      if ($scope.account.index > 0) {
        $scope.account =
          new Account($scope,
                      $http,
                      $scope.account.index - 1,
                      $scope.masterKey);
      }
    } else {
      $scope.account = new Account($scope, $http, 0, $scope.masterKey);
    }
  };

  $scope.nextAccountName = function() {
    return "Account " + $scope.wallet.getNextAccountNumber();
  };

  $scope.generateNextAccount = function() {
    $scope.wallet.deriveNextAccount(function(succeeded) {
      if (succeeded) {
        console.log("succeeded, account count is " + $scope.getAccountCount());
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
      $scope.showPrivateKey = false;
      $scope.$apply();
    };

    $scope.wallet.unlock(
      $scope.w.passphraseNew,
      relockCallback.bind(this),
      unlockCallback.bind(this));
  };

  $scope.lockWallet = function() {
    $scope.wallet.lock();
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
                                     function() {
                                       $scope.$apply();
                                     },
                                     function(succeeded) {
                                       if (succeeded) {
                                         $scope.$apply();
                                       }
                                     });

    // We don't want these items lurking in the DOM.
    $scope.w.passphraseNew = null;
    $scope.w.passphraseConfirm = null;
  };

  $scope.initializeEverything = function() {

    // TODO(miket): figure out how to re-get injected parameters
    if (false) {
      $scope.masterKey = null;
      $scope.account = null;
      $scope.accounts = [
        { 'parent': '0x11112222',
          'index': 0,
          'fingerprint': '0x22223333'},
      ];
//      $scope.settings = new Settings();
//    $scope.credentials = new Credentials($scope.settings);
    }
  };

  $scope.clearEverything = function() {
    // TODO(miket): confirmation
    $scope.initializeEverything();
    clearAllStorage();
    chrome.runtime.reload();
  };

  $scope.isPassphraseSet = function() {
    return $scope.credentials.isPassphraseSet();
  };

  $scope.isWalletUnlocked = function() {
    return $scope.credentials.isKeyAvailable();
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
  }

  $scope.getCurrentAccountFingerprint = function() {
    if (!$scope.getCurrentAccount()) {
      return null;
    }
    return $scope.getCurrentAccount().fingerprint;
  }

  $scope.setCurrentAccountByIndex = function(index) {
    $scope.w.currentAccount = $scope.getAccounts()[index];
  }

  $scope.selectFirstAccount = function() {
    if ($scope.getAccountCount() > 0) {
      $scope.setCurrentAccountByIndex(0);
    }
  };

  $scope.satoshiToUnit = function(satoshis) {
    return $scope.settings.satoshiToUnit(satoshis);
  };

  $scope.unitLabel = function() {
    return $scope.settings.unitLabel();
  };

  // TODO(miket): this might be a race with moduleDidLoad.
  var listenerDiv = document.getElementById('listener');
  listenerDiv.addEventListener('load', function() {
    $scope.startLoading();
  }, true);

  $scope.initializeEverything();
};
