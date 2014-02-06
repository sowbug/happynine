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
  $scope.w.selectedAddress = undefined;

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

    $scope.$watchCollection('getChildNodes()', function(newItems, oldItems) {
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

  $scope.$watch('getChildNodeCount()', function(newVal, oldVal) {
    if (newVal > 0 && oldVal == 0) {
      $scope.selectFirstChildNode();
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
    'getCurrentChildNodeFingerprint()',
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
    $scope.wallet.addRandomMasterKey()
      .then(function() {
        $scope.$apply();
      });
  };

  $scope.importMasterKey = function() {
    $scope.wallet.importMasterKey($scope.w.importMasterKey)
      .then(function() {
        $scope.w.importMasterKey = null;
        $("#import-master-key-modal").modal('hide');
        $scope.$apply();
      }.bind(this),
            function() { console.log("nope"); }.bind(this));
  };

  $scope.removeMasterKey = function() {
    // TODO(miket): ideally we'll track whether this key was backed
    // up, and make this button available only if yes. Then we'll
    // confirm up the wazoo before actually deleting.
    //
    // Less of a big deal if the master key is public.
    $scope.wallet.removeMasterKey();
  };

  $scope.nextChildNodeName = function() {
    return "Account " + $scope.wallet.getNextChildNodeNumber();
  };

  $scope.deriveNextChildNode = function() {
    $scope.wallet.deriveChildNode($scope.wallet.getNextChildNodeNumber(),
                                  false)
      .then(function() {
        $scope.$apply();
      });
  };

  $scope.watchNextChildNode = function() {
    $scope.wallet.deriveChildNode($scope.wallet.getNextChildNodeNumber(),
                                  true, function(succeeded) {
      if (succeeded) {
        $scope.$apply();
      }
    });
  };

  $scope.unlockWallet = function() {
    var unlockSuccessful = function() {
      $("#unlock-wallet-modal").modal('hide');
      $scope.w.passphraseNew = null;
      $scope.$apply();
    };
    var unlockFailed = function() {
      $scope.w.passphraseNew = null;
      $scope.$apply();
    };
    var relockCallback = function() {
      //$scope.w.showPrivateKey = false;
      $scope.$apply();
    };

    $scope.credentials.unlock($scope.w.passphraseNew,
                              relockCallback.bind(this))
      .then(unlockSuccessful, unlockFailed);
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
                                     function() {
                                       $scope.$apply();
                                     }).then(function() {
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

  $scope.isMasterKeyInstalled = function() {
    return $scope.wallet.rootNodes.length != 0;
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

  $scope.getChildNodeCount = function() {
    return $scope.wallet.getChildNodeCount();
  };

  $scope.getChildNodes = function() {
    return $scope.wallet.getChildNodes();
  };

  $scope.removeChildNode = function(node) {
    $scope.wallet.removeNode(node)
      .then(function() {
        if (node == $scope.getCurrentChildNode()) {
          $scope.selectFirstChildNode();
        }
        $scope.$apply();
      });
  };

  $scope.getPublicAddresses = function() {
    return $scope.wallet.publicAddresses;
  }

  $scope.getChangeAddresses = function() {
    return $scope.wallet.changeAddresses;
  }

  $scope.getAddressBalance = function(addr_b58) {
    return $scope.wallet.watchedAddresses[addr_b58].value;
  }

  $scope.getCurrentChildNode = function() {
    return $scope.w.currentChildNode;
  };

  $scope.getRecentTransactions = function() {
    return $scope.wallet.recentTransactions;
  };

  $scope.friendlyTimestamp = function(t) {
    if (t) {
      var d = new Date(t * 1000);
      return d.toLocaleString();
    }
    return "unconfirmed";
  };

  $scope.getCurrentChildNodeFingerprint = function() {
    if (!$scope.getCurrentChildNode()) {
      return null;
    }
    return $scope.getCurrentChildNode().fingerprint;
  };

  $scope.areRequestsPending = function() {
    return $scope.electrum.areRequestsPending();
  };

  $scope.setCurrentChildNodeByIndex = function(index) {
    if ($scope.w.currentChildNode == $scope.getChildNodes()[index]) {
      return;
    }
    $scope.w.currentChildNode = $scope.getChildNodes()[index];
   // $scope.refreshChildNode();
  };

  $scope.selectFirstChildNode = function() {
    if ($scope.getChildNodeCount() > 0) {
      $scope.setCurrentChildNodeByIndex(0);
    }
  };

  $scope.exportMasterKey = function() {
    chrome.fileSystem.chooseEntry(
      {'type': 'saveFile',
       'suggestedName': 'Happynine-Export-' +
       $scope.getWalletKeyFingerprint() + '.txt',
      },
      function(entry) {
        var errorHandler = function(e) {
          console.log(e);
        };
        entry.createWriter(function(writer) {
          writer.onerror = errorHandler;
          writer.onwriteend = function(e) {
            console.log('write complete');
          };
          var message =
            "Happynine: BIP 0038 Master Key Export\r\n" +
            $scope.getWalletKeyFingerprint() + ": " +
            $scope.getWalletKeyPublic() + "\r\n" +
            "Private Key [NOT YET IMPLEMENTED]\r\n\r\n" +
            "THIS IS NOT A BACKUP OF YOUR KEY! THAT FEATURE IS COMING.\r\n";
          message += "Meanwhile, here is a proof of concept text " +
            "QR code (NOT YOURS). \r\n\r\n";
          var qr = $('<div></div>');
          qr.qrcode({'text': $scope.getWalletKeyPublic(),
                     'render': 'text',
                     'correctLevel': QRErrorCorrectLevel.L});
          message += qr.text();
          var blob = new Blob([message], {type: 'text/plain; charset=utf-8'});
          writer.write(blob);
        }, errorHandler);
      });
  };

  $scope.send = function() {
    var sendTo = $scope.w.sendTo;
    var sendValue = $scope.unitToSatoshi($scope.w.sendValue);
    var sendFee = $scope.unitToSatoshi($scope.w.sendFee);

    if (sendValue > 0 && sendFee > 0 && sendTo.length > 25) {
      console.log("send", sendTo, sendValue, $scope.unitLabel(),
                  "for", sendFee);
      $scope.wallet.sendFunds(sendTo, sendValue, sendFee)
        .then(function() {
          $scope.$apply();
        });
    }
  };

  $scope.setAddress = function(a) {
    $scope.w.selectedAddress = a;
    $('#qrcode').empty();
    $('#qrcode').qrcode({
      'width': 128,
      'height': 128,
      'correctLevel': QRErrorCorrectLevel.L,
      'text': a});
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
