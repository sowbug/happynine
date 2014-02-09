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

function loadStorage(saved_name) {
  return new Promise(function(resolve, reject) {
    if (!chrome || !chrome.storage) {
      reject("!chrome; skipping load of", saved_name);
      return;
    }
    chrome.storage.sync.get(saved_name, function(result) {
      var items = result[saved_name];
      if (items) {
        logInfo("loaded", saved_name, "from storage", items);
      } else {
        logInfo("empty:", saved_name);
      }
      resolve(items);
    });
  });
}

function saveStorage(saved_name, object) {
  if (!chrome || !chrome.storage) {
    logInfo("!chrome; skipping save of", saved_name);
    return;
  }
  var toSave = {};
  toSave[saved_name] = object;
  chrome.storage.sync.set(toSave, function() {
    logInfo("saved", saved_name, "to storage", toSave);
  });
}

function clearAllStorage() {
  if (!chrome || !chrome.storage) {
    return;
  }
  chrome.storage.sync.clear(function() {
    logInfo("storage cleared");
  });
}
