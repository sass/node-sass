'use strict';

var types = require('../').types;
var iterateAndMeasure = require('./_measure');

iterateAndMeasure(function() {
  var key = new types.String('the-key');
  var value = new types.String('the-value');

  var map = new types.Map(1);

  map.setKey(0, key);
  map.setValue(0, value);

  map.getKey(0);
}, 100000);

