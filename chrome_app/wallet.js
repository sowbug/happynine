function moduleDidLoad() {
    common.hideModule();
}

function handleMessage(message) {
    var message_object = JSON.parse(message.data);
    switch (message_object.command) {
    case "sha256":
        document.querySelector("#sha256-output").value = message_object.hash;
        break;
    case "create-master-key":
        document.querySelector("#master-key-output").value =
            message_object.secret_key + " " +
            message_object.chain_code + " " +
            message_object.public_key;
        break;
    }
}

window.onload = function() {
    document.querySelector('#sha256-button').onclick = function() {
        var message = {
            'command': 'sha256',
            'value': document.querySelector("#sha256-input").value
        };
        common.naclModule.postMessage(JSON.stringify(message));
    };

    document.querySelector('#master-key-button').onclick = function() {
        var message = {
            'command': 'create-master-key',
            'seed': document.querySelector("#master-key-input").value
        };
        common.naclModule.postMessage(JSON.stringify(message));
    };
};
