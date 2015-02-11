# node-sass

<table>
  <tr>
    <td>
      <img width="100%" alt="Sass logo" src="https://rawgit.com/sass/node-sass/master/media/logo.svg" />
    </td>
    <td valign="bottom" align="right">
      <a href="https://nodei.co/npm/node-sass/">
        <img width="100%" src="https://nodei.co/npm/node-sass.png?downloads=true&downloadRank=true&stars=true">
      </a>
    </td>
  </tr>
</table>

[![Build Status](https://travis-ci.org/sass/node-sass.svg?branch=master&style=flat)](https://travis-ci.org/sass/node-sass)
[![Build status](https://ci.appveyor.com/api/projects/status/22mjbk59kvd55m9y/branch/master)](https://ci.appveyor.com/project/sass/node-sass/branch/master)
[![npm version](https://badge.fury.io/js/node-sass.svg)](http://badge.fury.io/js/node-sass)
[![Dependency Status](https://david-dm.org/sass/node-sass.svg?theme=shields.io)](https://david-dm.org/sass/node-sass)
[![devDependency Status](https://david-dm.org/sass/node-sass/dev-status.svg?theme=shields.io)](https://david-dm.org/sass/node-sass#info=devDependencies)
[![Coverage Status](https://coveralls.io/repos/sass/node-sass/badge.svg?branch=master)](https://coveralls.io/r/sass/node-sass?branch=master)
[![Inline docs](http://inch-ci.org/github/sass/node-sass.svg?branch=master)](http://inch-ci.org/github/sass/node-sass)
[![Gitter chat](http://img.shields.io/badge/gitter-sass/node--sass-brightgreen.svg)](https://gitter.im/sass/node-sass)

Node-sass is a library that provides binding for Node.js to [libsass], the C version of the popular stylesheet preprocessor, Sass.

It allows you to natively compile .scss files to css at incredible speed and automatically via a connect middleware.

Find it on npm: <https://npmjs.org/package/node-sass>

Follow @nodesass on twitter for release updates: https://twitter.com/nodesass

## Reporting Sass compilation and syntax issues

The [libsass] library is not currently at feature parity with the 3.2 [Ruby Gem](https://github.com/nex3/sass) that most Sass users will use, and has little-to-no support for 3.3 syntax. While we try our best to maintain feature parity with [libsass], we can not enable features that have not been implemented in [libsass] yet.

If you'd like to see what features are still upcoming in [libsass], [Jo Liss](http://twitter.com/jo_liss) has written [a blog post on the subject](http://www.solitr.com/blog/2014/01/state-of-libsass/).

Please check for [issues on the libsass repo](https://github.com/hcatlin/libsass/issues) (as there is a good chance that it may already be an issue there for it), and otherwise [create a new issue there](https://github.com/hcatlin/libsass/issues/new).

If this project is missing an API or command line flag that has been added to [libsass], then please open an issue here. We will then look at updating our [libsass] submodule and create a new release. You can help us create the new release by rebuilding binaries, and then creating a pull request to the [node-sass-binaries](https://github.com/sass/node-sass-binaries) repo.

## Install

    npm install node-sass

Some users have reported issues installing on Ubuntu due to `node` being registered to another package. [Follow the official NodeJS docs](https://github.com/joyent/node/wiki/Installing-Node.js-via-package-manager) to install NodeJS so that `#!/usr/bin/env node` correctly resolved.

Compiling versions 0.9.4 and above on Windows machines requires [Visual Studio 2013 WD](http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop). If you have multiple VS versions, use ```npm install``` with the ```--msvs_version=2013``` flag also use this flag when rebuilding the module with node-gyp or nw-gyp.

## Usage

```javascript
var sass = require('node-sass');
sass.render({
	file: scss_filename,
	success: callback
	[, options..]
	});
// OR
var result = sass.renderSync({
	data: scss_content
	[, options..]
});
```

### Options

The API for using node-sass has changed, so that now there is only one variable - an options hash. Some of these options are optional, and in some circumstances some are mandatory.

#### file
`file` is a `String` of the path to an `scss` file for [libsass] to render. One of this or `data` options are required, for both render and renderSync.

#### data
`data` is a `String` containing the scss to be rendered by [libsass]. One of this or `file` options are required, for both render and renderSync. It is recommended that you use the `includePaths` option in conjunction with this, as otherwise [libsass] may have trouble finding files imported via the `@import` directive.

#### success
`success` is a `Function` to be called upon successful rendering of the scss to css. This option is required but only for the render function. If provided to renderSync it will be ignored.

The callback function is passed a results object, containing the following keys:

* `css` - The compiled CSS. Write this to a file, or serve it out as needed.
* `map` - The source map
* `stats` - An object containing information about the compile. It contains the following keys:
    * `entry` - The path to the scss file, or `data` if the source was not a file
    * `start` - Date.now() before the compilation
    * `end` - Date.now() after the compilation
    * `duration` - *end* - *start*
    * `includedFiles` - Absolute paths to all related scss files in no particular order.

#### error
`error` is a `Function` to be called upon occurrence of an error when rendering the scss to css. This option is optional, and only applies to the render function. If provided to renderSync it will be ignored.

#### importer (starting from v2)
`importer` is a `Function` to be called when libsass parser encounters the import directive. If present, libsass will call node-sass and let the user change file, data or both during the compilation. This option is optional, and applies to both render and renderSync functions. Also, it can either return object of form `{file:'..', contents: '..'}` or send it back via `done({})`. Note in renderSync or render, there is no restriction imposed on using `done()` callback or `return` statement (dispite of the asnchrony difference).

#### includePaths
`includePaths` is an `Array` of path `String`s to look for any `@import`ed files. It is recommended that you use this option if you are using the `data` option and have **any** `@import` directives, as otherwise [libsass] may not find your depended-on files.

#### imagePath
`imagePath` is a `String` that represents the public image path. When using the `image-url()` function in a stylesheet, this path will be prepended to the path you supply. eg. Given an `imagePath` of `/path/to/images`, `background-image: image-url('image.png')` will compile to `background-image: url("/path/to/images/image.png")`

#### indentedSyntax
`indentedSyntax` is a `Boolean` flag to determine if [Sass Indented Syntax](http://sass-lang.com/documentation/file.INDENTED_SYNTAX.html) should be used to parse provided string or a file.

#### omitSourceMapUrl
`omitSourceMapUrl` is a `Boolean` flag to determine whether to include `sourceMappingURL` comment in the output file.

#### outFile
`outFile` specifies where the CSS will be saved. This option does not actually output a file, but is used as input for generating a source map.

#### outputStyle
`outputStyle` is a `String` to determine how the final CSS should be rendered. Its value should be one of `'nested'` or `'compressed'`.  Node-sass defaults to 'nested'.
[`'expanded'` and `'compact'` are not currently supported by [libsass]]

#### precision
`precision` is a `Number` that will be used to determine how many digits after the decimal will be allowed. For instance, if you had a decimal number of `1.23456789` and a precision of `5`, the result will be `1.23457` in the final CSS.

#### sourceComments
`sourceComments` is a `Boolean` flag to determine what debug information is included in the output file.

#### sourceMap
You must define this option as well as `outFile` in order to generate a source map. If it is set to `true`, the source map will be generated at the path provided in the `outFile` option. If set to a path (`String`), the source map will be generated at the provided path.

#### sourceMapEmbed
`sourceMapEmbed` is a `Boolean` flag to determine whether to embed `sourceMappingUrl` as data URI.

#### sourceMapContents
`sourceMapContents` is a `Boolean` flag to determine whether to include `contents` in maps.

### Examples

```javascript
var sass = require('node-sass');
sass.render({
	file: '/path/to/myFile.scss',
	data: 'body{background:blue; a{color:black;}}',
	success: function(result) {
		// result is an object: v2 change
        console.log(result.css);
        console.log(result.stats);
        console.log(result.map)
	},
	error: function(error) {
		// error is an object: v2 change
		console.log(error.message);
		console.log(error.code);
		console.log(error.line);
		console.log(error.column); // new in v2
	},
	importer: function(url, prev, done) {
		// url is the path in import as is, which libsass encountered.
		// prev is the previously resolved path.
		// done is an optional callback, either consume it or return value synchronously.
		someAsyncFunction(url, prev, function(result){
			done({
				file: result.path, // only one of them is required, see section Sepcial Behaviours.
				contents: result.data
			});
		});
		// OR
		var result = someSyncFunction(url, prev);
		return {file: result.path, contents: result.data};
	},
	includePaths: [ 'lib/', 'mod/' ],
	outputStyle: 'compressed'
});
// OR
var result = sass.renderSync({
	file: '/path/to/file.scss',
	data: 'body{background:blue; a{color:black;}}',
	outputStyle: 'compressed',
	outFile: '/to/my/output.css',
	sourceMap: true, // or an absolute or relative (to outFile) path
	importer: function(url, prev, done) {
		// url is the path in import as is, which libsass encountered.
		// prev is the previously resolved path.
		// done is an optional callback, either consume it or return value synchronously.
		someAsyncFunction(url, prev, function(result){
			done({
				file: result.path, // only one of them is required, see section Sepcial Behaviours.
				contents: result.data
			});
		});
		// OR
		var result = someSyncFunction(url, prev);
		return {file: result.path, contents: result.data};
	},
}));

console.log(result.css);
console.log(result.map);
console.log(result.stats);
```

### Special behaviours

* In the case that both `file` and `data` options are set, node-sass will give precedence to `data` and use `file` to calculate paths in sourcemaps.

### Version information (v2 change)

Both `node-sass` and `libsass` version info is now present in `package.json` and is exposed via `info()` method:

```javascript
require('node-sass').info();

// outputs something like:
// node-sass version: 2.0.0-beta
// libsass version: 3.1.0-beta
```

## Integrations

Listing of community uses of node-sass in build tools and frameworks.

### Brackets extension

[@jasonsanjose](https://github.com/jasonsanjose) has created a [Brackets](http://brackets.io) extension based on node-sass: <https://github.com/jasonsanjose/brackets-sass>. When editing Sass files, the extension compiles changes on save. The extension also integrates with Live Preview to show Sass changes in the browser without saving or compiling.

### Brunch plugin

[Brunch](http://brunch.io)'s official sass plugin uses node-sass by default, and automatically falls back to ruby if use of Compass is detected: <https://github.com/brunch/sass-brunch>

### Connect/Express middleware

Recompile `.scss` files automatically for connect and express based http servers.

This functionality has been moved to [`node-sass-middleware`](https://github.com/sass/node-sass-middleware) in node-sass v1.0.0

### DocPad Plugin

[@jking90](https://github.com/jking90) wrote a [DocPad](http://docpad.org/) plugin that compiles `.scss` files using node-sass: <https://github.com/jking90/docpad-plugin-nodesass>

### Duo.js extension

[@stephenway](https://github.com/stephenway) has created an extension that transpiles Sass to CSS using node-sass with [duo.js](http://duojs.org/)
<https://github.com/duojs/sass>

### Grunt extension

[@sindresorhus](https://github.com/sindresorhus/) has created a set of grunt tasks based on node-sass: <https://github.com/sindresorhus/grunt-sass>

### Gulp extension

[@dlmanning](https://github.com/dlmanning/) has created a gulp sass plugin based on node-sass: <https://github.com/dlmanning/gulp-sass>

### Harp

[@sintaxi](https://github.com/sintaxi)’s Harp web server implicitly compiles `.scss` files using node-sass: <https://github.com/sintaxi/harp>

### Metalsmith plugin

[@stevenschobert](https://github.com/stevenschobert/) has created a metalsmith plugin based on node-sass: <https://github.com/stevenschobert/metalsmith-sass>

### Meteor plugin

[@fourseven](https://github.com/fourseven) has created a meteor plugin based on node-sass: <https://github.com/fourseven/meteor-scss>

### Mimosa module

[@dbashford](https://github.com/dbashford) has created a Mimosa module for sass which includes node-sass: <https://github.com/dbashford/mimosa-sass>

## Example App

There is also an example connect app here: <https://github.com/andrew/node-sass-example>

## Rebuilding binaries

Node-sass includes pre-compiled binaries for popular platforms, to add a binary for your platform follow these steps:

Check out the project:

```bash
git clone --recursive https://github.com/sass/node-sass.git
cd node-sass
git submodule update --init --recursive
npm install
npm install -g node-gyp
node-gyp rebuild  # to make debug release, use -d switch
```

### Workaround for node `v0.11.13` `v0.11.14`
Follow the steps above, but comment out this [line](https://github.com/sass/node-sass/blob/e01497c4d4b8a7a7f4dbf9d607920ac10ad64445/lib/index.js#L181) in `lib/index.js` before the `npm install` step. Then uncomment it back again, and continue with the rest of the steps (see issue [#563](https://github.com/sass/node-sass/issues/563)).

## Command Line Interface

The interface for command-line usage is fairly simplistic at this stage, as seen in the following usage section.

Output will be saved with the same name as input SASS file into the current working directory if it's omitted.

### Usage
 `node-sass [options] <input.scss> [<output.css>]`

 **Options:**

```bash
    -w, --watch                Watch a directory or file
    -r, --recursive            Recursively watch directories or files
    -o, --output               Output directory
    -x, --omit-source-map-url  Omit source map URL comment from output
    -i, --indented-syntax      Treat data from stdin as sass code (versus scss)
    --output-style             CSS output style (nested|expanded|compact|compressed)
    --source-comments          Include debug info in output
    --source-map               Emit source map
    --source-map-embed         Embed sourceMappingUrl as data URI
    --source-map-contents      Embed include contents in map
    --include-path             Path to look for imported files
    --image-path               Path to prepend when using the `image-url()` helper
    --precision                The amount of precision allowed in decimal numbers
    --stdout                   Print the resulting CSS to stdout
    --importer                 Path to custom importer
    --help                     Print usage info
```

Note `--importer` takes the (absolute or relative to pwd) path to a js file, which needs to have a default `module.exports` set to the importer function. See our test [fixtures](https://github.com/sass/node-sass/tree/974f93e76ddd08ea850e3e663cfe64bb6a059dd3/test/fixtures/extras) for example.

## Post-install Build

Install runs only two Mocha tests to see if your machine can use the pre-built [libsass] which will save some time during install. If any tests fail it will build from source.

## Maintainers

This module is brought to you and maintained by the following people:

* Adeel Mujahid - Project Lead ([Github](https://github.com/am11) / [Twitter](https://twitter.com/adeelbm))
* Andrew Nesbitt ([Github](https://github.com/andrew) / [Twitter](https://twitter.com/teabass))
* Dean Mao ([Github](https://github.com/deanmao) / [Twitter](https://twitter.com/deanmao))
* Brett Wilkins ([Github](https://github.com/bwilkins) / [Twitter](https://twitter.com/bjmaz))
* Keith Cirkel ([Github](https://github.com/keithamus) / [Twitter](https://twitter.com/Keithamus))
* Laurent Goderre ([Github](https://github.com/laurentgoderre) / [Twitter](https://twitter.com/laurentgoderre))
* Nick Schonning ([Github](https://github.com/nschonni) / [Twitter](https://twitter.com/nschonni))
* Adam Yeats ([Github](https://github.com/adamyeats) / [Twitter](https://twitter.com/adamyeats))

## Contributors

We <3 our contributors! A special thanks to all those who have clocked in some dev time on this project, we really appreciate your hard work. You can find [a full list of those people here.](https://github.com/sass/node-sass/graphs/contributors)

### Note on Patches/Pull Requests

 * Fork the project.
 * Make your feature addition or bug fix.
 * Add documentation if necessary.
 * Add tests for it. This is important so I don't break it in a future version unintentionally.
 * Send a pull request. Bonus points for topic branches.

## Copyright

Copyright (c) 2015 Andrew Nesbitt. See [LICENSE](https://github.com/sass/node-sass/blob/master/LICENSE) for details.

[libsass]: https://github.com/hcatlin/libsass
