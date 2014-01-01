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

function updateFingerprintImage() {
  var fingerprint = $("#fingerprint-readonly").val();
  var xhr = new XMLHttpRequest();
  xhr.open('GET', 'http://robohash.org/' + fingerprint +
           '.png?set=set3&bgset=any&size=64x64', true);
  xhr.responseType = 'blob';
  xhr.onload = function(e) {
    $("#fingerprint-img-big").attr(
      'src',
      window.webkitURL.createObjectURL(this.response));
    $("#fingerprint-img").attr(
      'src',
      window.webkitURL.createObjectURL(this.response));
  };
  xhr.send();
}

function handleMessage(message) {
  var message_object = JSON.parse(message.data);
  console.log(message);
  switch (message_object.command) {
  case "get-wallet-node":
    document.querySelector("#ext-prv-b58").value =
      message_object.ext_prv_b58;
    document.querySelector("#chain-code").value =
      message_object.chain_code;
    document.querySelector("#public-key").value =
      message_object.public_key;
    document.querySelector("#fingerprint").value =
      message_object.fingerprint;
    updateFingerprintImage(message_object.fingerprint);
    break;
  case "create-random-node":
    document.querySelector("#xprv-readonly").value =
      message_object.ext_prv_b58;
    document.querySelector("#xpub-readonly").value =
      message_object.ext_pub_b58;
    document.querySelector("#fingerprint-readonly").value =
      message_object.fingerprint;
    document.querySelector("#fingerprint-navbar").innerText =
      message_object.fingerprint;
    updateFingerprintImage(message_object.fingerprint);
    break;
  }
}

var onTabClick = function(e) {
  e.preventDefault()
  $(this).tab('show')
};

var onNewMasterKeyClick = function(e) {
  e.preventDefault();
  var message = {
    'command': 'create-random-node'
  };
  common.naclModule.postMessage(JSON.stringify(message));
};

var onShowPrivateKeyClick = function(e) {
  e.preventDefault();
  $("#xprv-wrapper").slideToggle();
};

window.onload = function() {
  // Add click handlers.
  $('#main-tabs a').click(onTabClick);
  $('#new-master-key').click(onNewMasterKeyClick);
  $('#xprv-show').click(onShowPrivateKeyClick);

  // Set up initial state.
  $("#xprv-wrapper").hide();

  // document.querySelector('#get-wallet-node').onclick = function() {
  //   var message = {
  //     'command': 'get-wallet-node',
  //     'seed_hex': document.querySelector("#seed").value,
  //     'path': document.querySelector("#path").value
  //   };
  //   common.naclModule.postMessage(JSON.stringify(message));
  // };

  // For development in a normal browser tab.
  if (chrome && chrome.runtime)
    showLoading();
  else
    hideLoading();
};
