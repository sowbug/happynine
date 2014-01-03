function WalletController($scope) {
  $scope.masterKey = null;
  $scope.account = null;
  $scope.settings = settings;
  settings.$scope = $scope;

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
      var masterKey = new MasterKey(response.ext_prv_b58,
                                    response.ext_pub_b58,
                                    response.fingerprint);
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
}
