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

function WalletController($scope) {
  // For some crazy reason, angularjs won't reflect view changes in
  // the model's scope-level objects, so I have to create another
  // object and hang stuff off it. I picked w for wallet.
  $scope.w = {};

  $scope.startLoading = function() {
    $scope.settings.load(function() {
      $scope.credentials.load(function() {
        $scope.$apply();

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
      });
    });
  };

  $scope.$watch('credentials.extendedPrivateBase58',
                function(newVal, oldVal) {
                  if (newVal != oldVal) {
                    $scope.updateMasterKey();
                  }
                });
  $scope.$watch('credentials.extendedPublicBase58',
                function(newVal, oldVal) {
                  if (newVal != oldVal) {
                    $scope.updateMasterKey();
                  }
                });

  $scope.newMasterKey = function() {
    var message = {
      'command': 'create-node'
    };
    postMessageWithCallback(message, function(response) {
      $scope.credentials.setMasterKey(response.ext_pub_b58,
                                      response.ext_prv_b58,
                                      function() {
                                        $scope.$apply();
                                      });
    });
  };

  $scope.updateMasterKey = function() {
    if (!$scope.credentials.extendedPublicBase58) {
      console.log("updated master key to null");
      masterKey = null;
      return;
    }
    $scope.w.showPrivateKeys = false;
    var b58 = $scope.credentials.extendedPublicBase58;
    if ($scope.credentials.extendedPrivateBase58) {
      b58 = $scope.credentials.extendedPrivateBase58;
    }
    var message = {
      'command': 'get-node',
      'seed': b58
    };
    postMessageWithCallback(message, function(response) {
      var masterKey = new MasterKey(response.ext_pub_b58,
                                    response.ext_prv_b58,
                                    response.fingerprint);
      $scope.masterKey = masterKey;
      $scope.firstAccount();
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

  $scope.firstAccount = function() {
    $scope.account = new Account($scope, 0, $scope.masterKey);
  };

  $scope.nextAccount = function() {
    if ($scope.account) {
      $scope.account =
        new Account($scope, $scope.account.index + 1, $scope.masterKey);
    } else {
      $scope.account = new Account($scope, 0, $scope.masterKey);
    }
  };

  $scope.prevAccount = function() {
    if ($scope.account) {
      if ($scope.account.index > 0) {
        $scope.account =
          new Account($scope, $scope.account.index - 1, $scope.masterKey);
      }
    } else {
      $scope.account = new Account($scope, 0, $scope.masterKey);
    }
  };

  $scope.unlockWallet = function() {
    var message = {};
    message.command = 'unlock-wallet';
    message.salt = $scope.credentials.salt;
    message.check = $scope.credentials.check;
    message.internal_key_encrypted = $scope.credentials.internalKeyEncrypted;
    message.passphrase = $scope.passphraseNew;

    postMessageWithCallback(message, function(response) {
      if (response.key) {
        $("#unlock-wallet-modal").modal('hide');

        // This deserves some explanation. We are asking the
        // credentials object to cache the key and internal key
        // (asynchronously, because it needs to message the NaCl
        // module), after which it should call us to zero out the
        // passphrase and update the UI. And when the cache expires,
        // update the UI once again.
        $scope.credentials.cacheKeys(response.key,
                                     response.internal_key,
                                     function() {
                                       $scope.$apply();
                                     },
                                     function() {
                                       $scope.$apply(function() {
                                         $scope.passphraseNew = null;
                                       });
                                     });
      }
    });
  };

  $scope.lockWallet = function() {
    $scope.credentials.clearCachedKeys();
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
    $scope.masterKey = null;
    $scope.account = null;
    $scope.settings = new Settings();
    $scope.credentials = new Credentials($scope.settings);
  };

  $scope.clearEverything = function() {
    // TODO(miket): confirmation
    $scope.initializeEverything();
    clearAllStorage();
  };

  // TODO(miket): this might be a race with moduleDidLoad.
  var listenerDiv = document.getElementById('listener');
  listenerDiv.addEventListener('load', function() {
    $scope.startLoading();
  }, true);

  $scope.initializeEverything();
}
