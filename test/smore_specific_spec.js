var sass = require('../sass');
var assert = require('assert');
var path = require('path');

var SCSS_TEST_DIR = path.resolve(__dirname, 'smore_specific');

var includedFilesFile = path.resolve(SCSS_TEST_DIR, 'test_included_files.scss');
var importOnceFile = path.resolve(SCSS_TEST_DIR, 'test_import_once.scss');
var relativeAbsolutesFile = path.resolve(SCSS_TEST_DIR, 'relative_absolutes', 'test_relative_absolutes.scss');


describe('Smore specific Scss addons', function () {
    it('Should return the list of included files', function (done) {

        // A list of file names we expect to be included
        var expectedIncludedFiles = [
            path.resolve(SCSS_TEST_DIR, 'included_a.scss'),
            path.resolve(SCSS_TEST_DIR, 'included_b.scss'),
            includedFilesFile
        ];

        var stats = {};

        sass.render({
            file: includedFilesFile,
            stats: stats,
            success: function () {

                assert.deepEqual(stats.includedFiles, expectedIncludedFiles);

                done();
            },
            error: function (error) {
                done(error);
            }
        });


    });

    it('Should include files only once if the importOnce flag was specified', function (done) {

        var expected = 'body {\n  color: #aaa; }\n';

        sass.render({
            file: importOnceFile,
            importOnce: true,
            success: function (css) {


                assert.equal(css, expected);

                done();
            },
            error: function (error) {
                done(error);
            }
        });


    });

    it('Should use absolute paths relatively to the included folders', function (done) {

        var expected = 'body {\n  color: #aaa; }\n\nbody {\n  font-size: 12px; }\n';

        sass.render({
            file: relativeAbsolutesFile,
            includePaths: [SCSS_TEST_DIR],
            success: function (css) {


                assert.equal(css, expected);

                done();
            },
            error: function (error) {
                done(error);
            }
        });
    });
});
