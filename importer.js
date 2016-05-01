console.log('importer.js:', __dirname);

require('./');

module.exports = function() {
	return { contents: 'a { b: c; }' };
};