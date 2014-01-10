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

function loadStorage(saved_name, object, storables, callback) {
  if (!chrome || !chrome.storage) {
    console.log("!chrome; skipping load of " + saved_name);
    callback.call(this);
    return;
  }
  chrome.storage.sync.get(saved_name, function(result) {
    var items = result[saved_name];
    if (items) {
      console.log("loaded " + saved_name + " from storage", items);
      for (var key_index in storables) {
        var key = storables[key_index];
        object[key] = items[key];
      }
    } else {
      console.log("storage: " + saved_name + " is empty");
    }
    callback.call(this);
  });
}

function loadStorage2(saved_name, callback) {
  if (!chrome || !chrome.storage) {
    console.log("!chrome; skipping load of " + saved_name);
    callback.call(this, null);
    return;
  }
  chrome.storage.sync.get(saved_name, function(result) {
    var items = result[saved_name];
    if (items) {
      console.log("loaded " + saved_name + " from storage", items);
    } else {
      console.log("storage: " + saved_name + " is empty");
    }
    callback.call(this, items);
  });
}

function saveStorage(saved_name, object, storables) {
  if (!chrome || !chrome.storage) {
    console.log("!chrome; skipping save of " + saved_name);
    return;
  }
  var items = {};
  for (var key_index in storables) {
    var key = storables[key_index];
    items[key] = object[key];
  }
  var toSave = {};
  toSave[saved_name] = items;
  chrome.storage.sync.set(toSave, function() {
    console.log("saved " + saved_name + " to storage");
  });
}

function saveStorage2(saved_name, object) {
  if (!chrome || !chrome.storage) {
    console.log("!chrome; skipping save of " + saved_name);
    return;
  }
  var toSave = {};
  toSave[saved_name] = object;
  chrome.storage.sync.set(toSave, function() {
    console.log("saved " + saved_name + " to storage");
  });
}

function clearAllStorage() {
  if (!chrome || !chrome.storage) {
    return;
  }
  chrome.storage.sync.clear(function() {
    console.log("storage cleared");
  });
}
