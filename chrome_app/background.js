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
