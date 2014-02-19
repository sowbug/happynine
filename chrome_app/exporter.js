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

/**
 * @constructor
 */
function Exporter(wallet) {
  this.wallet = wallet;
}

Exporter.prototype.generateText = function(node, privateKey) {
  return new Promise(function(resolve, reject) {
    var nodeType = node.isMaster() ? "Master" : "Child";
    var div = document.createElement("div");
    var qr = $(div).qrcode({
      'width': 256,
      'height': 256,
      'correctLevel': QRErrorCorrectLevel.L,
      'text': privateKey});
    var canvas = div.children[0];
    var dataURL = canvas.toDataURL();
    console.log(canvas, dataURL);

    var xhr = new XMLHttpRequest();
    xhr.open('GET', chrome.runtime.getURL("export_template.html"), true);
    xhr.onload = function(e) {
      var text = this.response
        .replace('{{NODE_TYPE}}', nodeType)
        .replace('{{FINGERPRINT}}', node.fingerprint)
        .replace('{{PUBLIC_KEY}}', node.extendedPublicBase58)
        .replace('{{PRIVATE_KEY}}', privateKey)
        .replace('{{QR_CODE_DATA_URL}}', dataURL);
      resolve(text);
  };
  xhr.send();
  });
}

Exporter.prototype.exportToEntry = function(node, privateKey, entry) {
  var errorHandler = function(e) {
    logFatal("file export", e);
  };

  this.generateText(node, privateKey)
    .then(function(text) {
      entry.createWriter(function(writer) {
        writer.onerror = errorHandler;
        var blob = new Blob([text], {type: 'text/plain; charset=utf-8'});
        writer.write(blob);
      }, errorHandler);
    });
};

Exporter.prototype.exportNode = function(node, privateKey) {
  chrome.fileSystem.chooseEntry(
    {'type': 'saveFile',
     'suggestedName': 'Happynine-Export-' + node.fingerprint + '.html'
    },
    this.exportToEntry.bind(this, node, privateKey));
};

