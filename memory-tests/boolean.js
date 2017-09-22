'use strict';

var types = require('../').types;
var iterateAndMeasure = require('./_measure');

iterateAndMeasure(function() { return types.Boolean(true).getValue(); });
