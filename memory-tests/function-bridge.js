"use strict";

var sass = require("../");
var iterateAndMeasure = require('./_measure');

iterateAndMeasure(function() {
  sass.renderSync({
    data: '#{headings()} { color: #08c; }',
    functions: {
      'headings()': function() {
        return new sass.types.String('hi');
      }
    }
  });
}, 10000);
