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
  var nodeType = node.isMaster() ? "Master" : "Child";
  var text =
    "Happynine Export for BIP 0038 " + nodeType +" Node " +
    node.fingerprint + "\r\n" +
    " PUBLIC KEY: " + node.extendedPublicBase58 + "\r\n" +
    "PRIVATE KEY: " + privateKey + "\r\n" +
    "Print with a small monospace (fixed-width) typeface. Scan at angle.\r\n\r\n";
  var qr = $('<div></div>');
  qr.qrcode({'text': privateKey,
             'render': 'text',
             'correctLevel': QRErrorCorrectLevel.L});
  text += qr.text();
  return text;
}

Exporter.prototype.exportToEntry = function(node, privateKey, entry) {
  var errorHandler = function(e) {
    logFatal("file export", e);
  };

  var text = this.generateText(node, privateKey);
  entry.createWriter(function(writer) {
    writer.onerror = errorHandler;
    var blob = new Blob([text], {type: 'text/plain; charset=utf-8'});
    writer.write(blob);
  }, errorHandler);
};

Exporter.prototype.exportNode = function(node, privateKey) {
  chrome.fileSystem.chooseEntry(
    {'type': 'saveFile',
     'suggestedName': 'Happynine-Export-' +
     node.fingerprint + '.txt'
    },
    this.exportToEntry.bind(this, node, privateKey));
};

