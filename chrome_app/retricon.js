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

// Given a string, returns a string suitable for inserting as a data
// URL into an <img> tag.
//
// Heavily based on https://github.com/sehrgut/node-retricon.

/**
 * @constructor
 */
function Retricon() {
}

Retricon.DEFAULTS = {
  pixelSize: 10,
  bgColor: undefined,
  pixelPadding: 0,
  imagePadding: 0,
  tiles: 5,
  minFill: 0.3,
  maxFill: 0.90,
  pixelColor: 0
};

Retricon.STYLE = {
  github: {
    pixelSize: 70,
    bgColor: '#F0F0F0',
    pixelPadding: -1,
    imagePadding: 35,
    tiles: 5
  },
  gravatar: {
    tiles: 8,
    bgColor: 1
  },
  mono: {
    bgColor: '#F0F0F0',
    pixelColor: '#000000',
    tiles: 6,
    pixelSize: 12,
    pixelPadding: -1,
    imagePadding: 6
  },
  mosaic: {
    imagePadding: 2,
    pixelPadding: 1,
    pixelSize: 16,
    bgColor: '#F0F0F0'
  },
  mini: {
    pixelSize: 10,
    pixelPadding: 1,
    tiles: 3,
    bgColor: 0,
    pixelColor: 1
  },
  window: {
    pixelColor: undefined,
    bgColor: 0,
    imagePadding: 2,
    pixelPadding: 1,
    pixelSize: 16
  }
};

function brightness(r, g, b) {
  // http://www.nbdtech.com/Blog/archive/2008/04/27/Calculating-the-Perceived-Brightness-of-a-Color.aspx
  return Math.sqrt( .241 * r * r + .691 * g * g + .068 * b * b )
}

function cmp_brightness(a, b) {
  return brightness(a[0], a[1], a[2]) - brightness(b[0], b[1], b[2]);
}

function rcmp_brightness(a, b) {
  return cmp_brightness(b, a);
}

Retricon.prototype.reflect = function(id, dimension) {
  var mid = Math.ceil(dimension / 2);
  var odd = Boolean(dimension % 2);

  var pic = [];
  for (var row = 0; row < dimension; ++row) {
    pic[row] = [];
    for (var col = 0; col < dimension; ++col) {
      var p = (row * mid) + col;
      if (col >= mid) {
        var d = mid - (odd ? 1 : 0) - col;
        var ad = Math.abs(d);
        p = (row * mid) + mid - 1 - ad;
      }
      pic[row][col] = id.pixels[p];
    }
  }
  return pic;
};

Retricon.prototype.idhash = function(str, n, minFill, maxFill) {
  var hash = CryptoJS.SHA512(str);
  var hex_hash = hash + '';
  var colors = [ hex_hash.substring(0, 6), hex_hash.substring(6, 12) ];
  colors.sort(cmp_brightness);
  var a = new ArrayBuffer(4 * 16);
  var a4 = new Uint32Array(a);
  for (var i = 0; i < hash.words.length; ++i) {
    a4[i] = hash.words[i];
  }
  var a1 = new Uint8Array(a);
  var pixels = [];
  for (var i = 0; i < hash.words.length * 4; ++i) {
    pixels.push(a1[i] & 1);
  }
  return {colors: colors, pixels: pixels};
};

Retricon.prototype.create = function(str, opts) {
  if (opts == undefined) {
    opts = {};
  }
  for (var i in Retricon.DEFAULTS) {
    if (!opts.hasOwnProperty(i)) {
      opts[i] = Retricon.DEFAULTS[i];
    }
  }

  var dimension = opts.tiles;
  var pixelSize = opts.pixelSize;
  var border = opts.pixelPadding;

  var mid = Math.ceil(dimension / 2);

  var id = this.idhash(str, mid * dimension, opts.minFill, opts.maxFill);
  var pic = this.reflect(id, dimension);
  var csize = (pixelSize * dimension) + (opts.imagePadding * 2);
  var c = document.createElement('canvas');
  c.width = csize;
  c.height = csize;
  var ctx = c.getContext('2d');

  if (opts.bgColor != undefined) {
    if (opts.bgColor[0] == '#') {
      ctx.fillStyle = opts.bgColor;
    } else {
      var col = '#' + id.colors[opts.bgColor];
      ctx.fillStyle = col;
    }
    ctx.fillRect(0, 0, csize, csize);
  }

  var drawOp = ctx.fillRect.bind(ctx);
  if (opts.pixelColor != undefined) {
    if (opts.pixelColor[0] == '#') {
      ctx.fillStyle = opts.pixelColor;
    } else {
      ctx.fillStyle = '#' + id.colors[opts.pixelColor];
    }
  } else {
    drawOp = ctx.clearRect.bind(ctx);
  }

  for (var x = 0; x < dimension; ++x) {
    for (var y = 0; y < dimension; ++y) {
      if (pic[y][x]) {
        drawOp((x * pixelSize) + border + opts.imagePadding,
               (y * pixelSize) + border + opts.imagePadding,
               pixelSize - (border * 2),
               pixelSize - (border * 2));
      }
    }
  }
  return c;
}
