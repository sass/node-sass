var parseImports = require('../../lib/sass-graph/parse-imports');
var assert = require('assert').strict;
var path = require('path');

describe('parse-imports', function () {
  it('should parse single import with single quotes', function () {
    var scss = "@import 'app';";
    var result = parseImports(scss);
    assert.deepEqual(['app'], result);
  });

  it('should parse single import with double quotes', function () {
    var scss = '@import "app";';
    var result = parseImports(scss);
    assert.deepEqual(['app'], result);
  });

  it('should parse single import without quotes', function () {
    var scss = '@import app;';
    var result = parseImports(scss);
    assert.deepEqual(['app'], result);
  });

  it('should parse single import without quotes in indented syntax', function () {
    var scss = '@import app';
    var result = parseImports(scss, true);
    assert.deepEqual(['app'], result);
  });

  it('should parse unquoted import', function () {
    var scss = '@import include/app;\n' +
               '@import include/foo,\n' +
                  'include/bar;';
    var result = parseImports(scss);
    assert.deepEqual(['include/app', 'include/foo', 'include/bar'], result);
  });

  it('should parse single import with extra spaces after import', function () {
    var scss = '@import  "app";';
    var result = parseImports(scss);
    assert.deepEqual(['app'], result);
  });

  it('should parse single import with extra spaces before ;', function () {
    var scss = '@import "app" ;';
    var result = parseImports(scss);
    assert.deepEqual(['app'], result);
  });

  it('should parse two individual imports', function () {
    var scss = '@import "app"; \n' +
               '@import "foo"; \n';
    var result = parseImports(scss);
    assert.deepEqual(['app', 'foo'], result);
  });

  it('should parse two imports on same line', function () {
    var scss = '@import "app", "foo";';
    var result = parseImports(scss);
    assert.deepEqual(['app', 'foo'], result);
  });

  it('should parse two imports continued on multiple lines ', function () {
    var scss = '@import "app", \n' +
                   '"foo"; \n';
    var result = parseImports(scss);
    assert.deepEqual(['app', 'foo'], result);
  });

  it('should parse three imports with mixed style ', function () {
    var scss = '@import "app", \n' +
                        '"foo"; \n' +
               '@import "bar";';
    var result = parseImports(scss);
    assert.deepEqual(['app', 'foo', 'bar'], result);
  });

  it('should not parse import in CSS comment', function () {
    var scss = '@import "app"; \n' +
               '/*@import "foo";*/ \n' +
               '/*@import "nav"; */ \n' +
               '@import /*"bar"; */ "baz"; \n' +
               '@import /*"bar";*/ "bam"';
    var result = parseImports(scss);
    assert.deepEqual(['app', 'baz', 'bam'], result);
  });

  it('should not parse import in Sass comment', function () {
    var scss = '@import "app"; \n' +
               '//@import "foo"; \n' +
               '@import //"bar"; \n'+
                        '"baz"';
    var result = parseImports(scss);
    assert.deepEqual(['app', 'baz'], result);
  });

  it('should not parse import in any comment', function () {
    var scss = '@import \n' +
      '// app imports foo\n' +
      '"app",\n' +
      '\n' +
      '/** do not import bar;\n' +
      ' "bar"\n' +
      '*/\n' +
      '\n' +
      '// do not import nav: "d",\n' +
      '\n' +
      '// footer imports nothing else\n' +
      '"baz"';
    var result = parseImports(scss);
    assert.deepEqual(['app', 'baz'], result);
  });

  it('should throw error when invalid @import syntax is encountered', function () {
    var scss = '@import "a"\n' +
        '@import "b";';
    assert.throws(function() {
      parseImports(scss);
    });
  });

  it('should not throw error when invalid @import syntax is encountered using indented syntax', function () {
    var scss = '@import a\n' +
        '@import b';
    assert.doesNotThrow(function() {
      parseImports(scss, true);
    });
  });

  it('should parse a full css file', function () {
    var scss = '@import url("a.css");\n' +
        '@import url("b.scss");\n' +
        '@import "c.scss";\n' +
        '@import "d";\n' +
        '@import "app1", "foo1";\n' +
        '@import "app2",\n' +
        '  "foo2";\n' +
        '/********\n' +
        'reset\n' +
        '*********/\n' +
        '/*\n' +
        'table{\n' +
        '  border-collapse: collapse;\n' +
        '  width: 100%;\n' +
        '}\n' +
        '\n' +
        '  [class*="jimu"],\n' +
        '  [class*="jimu"] * {\n' +
        '  -moz-box-sizing: border-box;\n' +
        '  box-sizing: border-box;\n' +
        '}';
    var result = parseImports(scss);
    assert.deepEqual(["c.scss", "d", "app1", "foo1", "app2", "foo2"], result);
  });
});
