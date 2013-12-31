function moduleDidLoad() {
    common.hideModule();
}

function updateFingerprintImage(fingerprint) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'http://robohash.org/' + fingerprint +
             '.png?set=set3&bgset=any&size=64x64', true);
    xhr.responseType = 'blob';
    xhr.onload = function(e) {
        var img = document.querySelector("#fingerprint-img");
        img.src = window.webkitURL.createObjectURL(this.response);
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
        document.querySelector("#path").value = "m";
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
    }
}

window.onload = function() {
    document.querySelector('#get-wallet-node').onclick = function() {
        var message = {
            'command': 'get-wallet-node',
            'seed_hex': document.querySelector("#seed").value,
            'path': document.querySelector("#path").value
        };
        common.naclModule.postMessage(JSON.stringify(message));
    };
    document.querySelector('#create-random-node').onclick = function() {
        var message = {
            'command': 'create-random-node'
        };
        common.naclModule.postMessage(JSON.stringify(message));
    };
};
