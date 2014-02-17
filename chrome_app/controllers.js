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

function AppController($scope,
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
  $scope.w.selectedAddress = undefined;
  $scope.w.isCheckingPassphrase = false;
  $scope.w.enableLogging = SHOULD_LOG;  // First time through, pick it up

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
    $scope.settings.load()
      .then(function() {
        return $scope.credentials.load();
      }).then(function() {
        return $scope.wallet.load();
      }).then(function() {
        $scope.$apply();
        $scope.addPostLoadWatchers();
      }).then(function() {
        $scope.wallet.startElectrum();
      }).catch(function(err) {
        logFatal("startLoading", err);
      });
  };

  $scope.$watch('getChildNodeCount()', function(newVal, oldVal) {
    if (newVal > 0 && oldVal == 0) {
      $scope.selectFirstChildNode();
    }
  });

  $scope.$watch('isMasterKeyInstalled()', function(newVal, oldVal) {
    if (newVal && !oldVal) {
      $scope.selectFirstMasterNode(0);
    }
  });

  $scope.$watch(
    'getCurrentMasterNodeFingerprint()',
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

  $scope.$watch('w.enableLogging', function(newVal, oldVal) {
    if (newVal !== oldVal) {
      SHOULD_LOG = newVal;
    }
  });

  $scope.newMasterKey = function() {
    $scope.wallet.addRandomMasterKey()
      .then(function() {
        $scope.$apply();
      });
  };

  $scope.importMasterKey = function() {
    var success = function() {
      $scope.w.didImportFail = false;
      $scope.w.importMasterKey = null;
      $("#import-master-key-modal").modal('hide');
      $scope.$apply();
    };
    var failure = function(err) {
      $scope.w.didImportFail = true;
      logFatal("importMasterKey", err);
      $scope.$apply();
    };
    $scope.w.didImportFail = false;
    $scope.wallet.importMasterKey($scope.w.importMasterKey)
      .then(success.bind(this),
            failure.bind(this));
  };

  $scope.removeMasterNode = function() {
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

  $scope.resetIsCheckingPassphrase = function() {
    $scope.w.isCheckingPassphrase = false;
  };

  $scope.unlockWallet = function(secondsUntilRelock) {
    var unlockSuccessful = function() {
      $scope.w.passphraseNew = null;
      $scope.resetIsCheckingPassphrase();
      $scope.w.didPassphraseCheckFail = false;
      $("#unlock-wallet-modal").modal('hide');
      $scope.$apply();
    };
    var unlockFailed = function() {
      $scope.w.passphraseNew = null;
      $scope.resetIsCheckingPassphrase();
      $scope.w.didPassphraseCheckFail = true;
      $scope.$apply();
    };
    var relockCallback = function() {
      $scope.$apply();
    };

    $scope.w.isCheckingPassphrase = true;
    $scope.w.didPassphraseCheckFail = false;
    $scope.credentials.unlock($scope.w.passphraseNew,
                              secondsUntilRelock,
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
      logFatal("missing new passphrase");
      return;
    }
    if ($scope.w.passphraseNew != $scope.w.passphraseConfirm) {
      logInfo("new didn't match confirm:" + $scope.w.passphraseNew);
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

  $scope.isChildNodeInstalled = function() {
    return $scope.wallet.nodes.length != 0;
  };

  $scope.isWalletKeySet = function() {
    return $scope.wallet.isKeySet();
  };

  $scope.isMasterKeyInstalled = function() {
    return $scope.wallet.masterNodes.length != 0;
  };

  $scope.getWalletKeyPublic = function() {
    return $scope.wallet.activeMasterNode.extendedPublicBase58;
  };

  $scope.getWalletBalance = function() {
    return $scope.wallet.getBalance();
  };

  $scope.getCurrentMasterNodeFingerprint = function() {
    if ($scope.getCurrentMasterNode()) {
      return $scope.getCurrentMasterNode().fingerprint;
    }
    return undefined;
  };

  $scope.getCurrentChildNodeFingerprint = function() {
    if ($scope.getCurrentChildNode()) {
      return $scope.getCurrentChildNode().fingerprint;
    }
    return undefined;
  };

  $scope.getCurrentMasterNodePublicKey = function() {
    if ($scope.getCurrentMasterNode()) {
      return $scope.getCurrentMasterNode().extendedPublicBase58;
    }
    return undefined;
  };

  $scope.getChildNodeCount = function() {
    return $scope.wallet.getChildNodeCount();
  };

  $scope.getChildNodes = function() {
    return $scope.wallet.getChildNodes();
  };

  $scope.removeChildNode = function(node) {
    var needNewSelection = node == $scope.getCurrentChildNode();
    $scope.wallet.removeNode(node)
      .then(function() {
        if (needNewSelection) {
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
    return $scope.wallet.getAddressBalance(addr_b58);
  }

  $scope.getAddressTxCount = function(addr_b58) {
    return $scope.wallet.getAddressTxCount(addr_b58);
  }

  $scope.isAddressExpended = function(addr_b58) {
    if ($scope.getAddressBalance(addr_b58) != 0) {
      return false;
    }
    if ($scope.getAddressTxCount(addr_b58) == 0) {
      return false;
    }
    return true;
  };


  $scope.getCurrentMasterNode = function() {
    return $scope.wallet.activeMasterNode;
  };

  $scope.getCurrentChildNode = function() {
    return $scope.wallet.activeChildNode;
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

  $scope.areRequestsPending = function() {
    return $scope.electrum.areRequestsPending();
  };

  $scope.selectFirstMasterNode = function() {
    $scope.wallet.setActiveMasterNodeByIndex(0);
  };

  $scope.selectFirstChildNode = function() {
    $scope.wallet.setActiveChildNodeByIndex(0);
  };

  $scope.selectChildNode = function(node) {
    $scope.wallet.setActiveChildNode(node);
  };

  $scope.selectChildNodeByIndex = function(index) {
    $scope.wallet.setActiveChildNodeByIndex(index);
  };

  $scope.exportMasterNode = function() {
    $scope.exportNode($scope.getCurrentMasterNode());
  };

  $scope.exportNode = function(node) {
    var doFail = function(err) {
      logFatal("exporting", err);
    };
    var exporter = new Exporter($scope.wallet);
    $scope.wallet.retrievePrivateKey(node)
      .then(function(key) {
        exporter.exportNode(node, key);
      },
            doFail.bind(this));
  };

  $scope.send = function() {
    var sendTo = $scope.w.sendTo;
    var sendValue = $scope.unitToSatoshi($scope.w.sendValue);
    var sendFee = $scope.unitToSatoshi($scope.w.sendFee);

    if (sendValue > 0 && sendFee > 0 && sendTo.length > 25) {
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

  $(document).on("apply", function(evt) {
    $scope.$apply();
  }.bind(this));
}
