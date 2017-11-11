'use strict';

var types = require('../').types;
var iterateAndMeasure = require('./_measure');

iterateAndMeasure(function() { return new types.String('hi'); });
