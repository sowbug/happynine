// Copyright 2014 Mike Tsao <mike@sowbug.com>

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

function MasterKey(xprv, xpub, fingerprint) {
  this.showXprv = false;
  this.xprv = xprv;
  this.xpub = xpub;

  var mk = this;
  this.setFingerprint = function(fingerprint) {
    mk.fingerprint = fingerprint;
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'http://robohash.org/' + fingerprint +
             '.png?set=set3&bgset=any&size=64x64', true);
    xhr.responseType = 'blob';
    xhr.onload = function(e) {
      $("#fingerprint-img").attr(
        "src",
        window.webkitURL.createObjectURL(this.response));
      $("#fingerprint-img-big").attr(
        "src",
        window.webkitURL.createObjectURL(this.response));
    };
    xhr.send();
  };
  this.setFingerprint(fingerprint);
}
