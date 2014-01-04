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
  $scope.masterKey = null;
  $scope.account = null;
  $scope.settings = new Settings();
  $scope.credentials = new Credentials($scope.settings);

  // Passphrase UI elements
  $scope.passphraseNew = "foo";
  $scope.passphraseConfirm = "bar";

  $scope.settings.load(function() {
    console.log("settings are loaded. do something!");
    $scope.$apply();
  });

  $scope.newMasterKey = function() {
    var message = {
      'command': 'create-node'
    };
    thisScope = $scope;
    postMessageWithCallback(message, function(response) {
      thisScope.setMasterKey(response.ext_prv_b58);
    });
  };

  $scope.setMasterKey = function(extended_b58) {
    if (!extended_b58 ||
        ($scope.masterKey && $scope.masterKey.xpub == extended_b58)) {
      return;
    }
    var message = {
      'command': 'get-node',
      'seed': extended_b58
    };
    thisScope = $scope;
    postMessageWithCallback(message, function(response) {
      $scope.$apply(function() {
        var masterKey = new MasterKey(response.ext_prv_b58,
                                      response.ext_pub_b58,
                                      response.fingerprint);
        $scope.masterKey = masterKey;
        $scope.nextAccount();
      });
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
        new Account($scope, $scope.account.index + 1);
    } else {
      $scope.account = new Account($scope, 0);
    }
  };

  $scope.prevAccount = function() {
    if ($scope.account) {
      if ($scope.account.index > 0) {
        $scope.account =
          new Account($scope, $scope.account.index - 1);
      }
    } else {
      $scope.account = new Account($scope, 0);
    }
  };

  $scope.unlockWallet = function() {
    var message = {};
    message.command = 'unlock-wallet';
    message.salt = $scope.credentials.salt();
    message.check = $scope.credentials.check();
    message.internal_key_encrypted = $scope.credentials.internalKeyEncrypted();
    message.passphrase = $scope.passphraseNew;

    postMessageWithCallback(message, function(response) {
      if (response.key) {
        $("#unlock-wallet-modal").modal('hide');
        $scope.credentials.cacheKeys(response.key,
                                     response.internal_key,
                                     function() {
                                       $scope.$apply();
                                     });
        $scope.$apply(function() {
          $scope.passphraseNew = null;
        });
      }
    });
  };

  $scope.lockWallet = function() {
    $scope.credentials.clearCachedKeys();
  };

  $scope.setPassphrase = function() {
    console.log($scope.setPassphrase);
    console.log($scope.passphraseNew);
    console.log($scope.passphraseConfirm);
    // TODO: angularjs can probably do this check for us
    if (!$scope.passphraseNew || $scope.passphraseNew.length == 0) {
      console.log("missing new passphrase");
      return;
    }
    if ($scope.passphraseNew != $scope.passphraseConfirm) {
      console.log("new didn't match confirm:" + $scope.passphraseNew);
      return;
    }
    $scope.setPassphrase($scope.passphraseNew);

    // We don't want these items lurking in the DOM.
    $scope.passphraseNew = null;
    $scope.passphraseConfirm = null;
  };

  $scope.clearEverything = function() {
    // TODO(miket): confirmation
    clearAllStorage();
  };

  $scope.$watch(function() {
    return $scope.settings.storable.units;
  }, function(newVal, oldVal) {
    console.log('settings', newVal, oldVal);
  });

  $scope.$watch('settings.storable.units', function(newVal, oldVal) {
    console.log('settings 2', newVal, oldVal);
  });

  $scope.$watch(function() {
    return $scope.passphraseNew;
  }, function(newVal, oldVal) {
    console.log('passphrase', newVal, oldVal);
  });
}
