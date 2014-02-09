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

var WIDTH = 1000;
var HEIGHT = 640;

var onCreate = function(window) {
  // There seems to be a bug where resizeTo includes the title bar
  // height and isn't the same as the width/height parameters passed
  // to app.window.create().
  //    window.resizeTo(WIDTH, HEIGHT + UNKNOWN_TITLE_BAR_HEIGHT);
};

chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create('index.html', {
    'frame': 'chrome',
    'width': WIDTH,
    'height': HEIGHT,
    'resizable': false
  }, onCreate);
});
