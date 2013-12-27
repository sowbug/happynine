function moduleDidLoad() {
    common.hideModule();
}

function handleMessage(message) {
    document.querySelector("#sha256-output").value = message.data;
}

window.onload = function() {
    document.querySelector('#sha256-button').onclick = function() {
        var message = {
            'command': 'sha256',
            'value': document.querySelector("#sha256-input").value
        };
        common.naclModule.postMessage(JSON.stringify(message));
    };
};
