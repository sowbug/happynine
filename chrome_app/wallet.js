function moduleDidLoad() {
    common.hideModule();
}

function handleMessage(message) {
    var message_object = JSON.parse(message.data);
    console.log(message);
    switch (message_object.command) {
    case "sha256":
        document.querySelector("#sha256-output").value = message_object.hash;
        break;
    case "get-wallet":
        document.querySelector("#secret-key").value =
            message_object.secret_key;
        document.querySelector("#chain-code").value =
            message_object.chain_code;
        document.querySelector("#public-key").value =
            message_object.public_key;
        document.querySelector("#fingerprint").value =
            message_object.fingerprint;
        break;
    }
}

window.onload = function() {
    document.querySelector('#get-wallet').onclick = function() {
        var message = {
            'command': 'get-wallet',
            'seed_hex': document.querySelector("#seed").value,
            'wallet': document.querySelector("#wallet").value
        };
        common.naclModule.postMessage(JSON.stringify(message));
    };
};
