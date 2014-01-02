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

var bitcoinWalletApp = angular.module('bitcoinWalletApp', []);

var callbacks = {};
var callbackId = 1;

var postMessageWithCallback = function(message, callback) {
  message.id = callbackId++;
  callbacks[message.id] = callback;
  common.naclModule.postMessage(JSON.stringify(message));
  console.log(message);
};

function handleMessage(message) {
  var message_object = JSON.parse(message.data);
  console.log(message_object);
  var id = message_object.id;
  if (callbacks[id]) {
    callbacks[id].call(this, message_object);
    delete callbacks[id];
  }
}

var hideLoading = function() {
  $("#loading-container").fadeOut(250, function() {
    $("#main-container").fadeIn(500, function() {
      $('#main-tabs a:first').tab('show');
      $('#settings-tabs a:first').tab('show');
    });
  });
};

var showLoading = function() {
  $("#loading-container").show();
  $("#main-container").hide();
};

function moduleDidLoad() {
  common.hideModule();
  hideLoading();
}

window.onload = function() {
  var onTabClick = function(e) {
    e.preventDefault()
    $(this).tab('show')
  };

  // Add click handlers.
  $('#main-tabs a').click(onTabClick);
  $('#settings-tabs a').click(onTabClick);

  if (chrome && chrome.runtime) {
    showLoading();
  } else {
    // For development in a normal browser tab.
    hideLoading();
  }
};
