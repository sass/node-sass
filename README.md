# node-sass

![logo](https://rawgit.com/sass/node-sass/master/media/logo.svg)

[![Build Status](https://travis-ci.org/sass/node-sass.svg?branch=master&style=flat)](https://travis-ci.org/sass/node-sass)
[![Build status](https://ci.appveyor.com/api/projects/status/22mjbk59kvd55m9y/branch/master)](https://ci.appveyor.com/project/sass/node-sass/branch/master)
[![npm version](https://badge.fury.io/js/node-sass.svg)](http://badge.fury.io/js/node-sass)
[![Dependency Status](https://david-dm.org/sass/node-sass.svg?theme=shields.io)](https://david-dm.org/sass/node-sass)
[![devDependency Status](https://david-dm.org/sass/node-sass/dev-status.svg?theme=shields.io)](https://david-dm.org/sass/node-sass#info=devDependencies)
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

Compiling versions 0.9.4 and above on Windows machines requires [Visual Studio 2013 WD](http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop). If you have multiple VS versions, use ```npm install``` with the ```--msvs_version=2013``` flag.

## Usage

```javascript
var sass = require('node-sass');
sass.render({
	file: scss_filename,
	success: callback
	[, options..]
	});
// OR
var css = sass.renderSync({
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

#### error
`error` is a `Function` to be called upon occurance of an error when rendering the scss to css. This option is optional, and only applies to the render function. If provided to renderSync it will be ignored.

#### includePaths
`includePaths` is an `Array` of path `String`s to look for any `@import`ed files. It is recommended that you use this option if you are using the `data` option and have **any** `@import` directives, as otherwise [libsass] may not find your depended-on files.

#### imagePath
`imagePath` is a `String` that represents the public image path. When using the `image-url()` function in a stylesheet, this path will be prepended to the path you supply. eg. Given an `imagePath` of `/path/to/images`, `background-image: image-url('image.png')` will compile to `background-image: url("/path/to/images/image.png")`

#### outputStyle
`outputStyle` is a `String` to determine how the final CSS should be rendered. Its value should be one of `'nested'` or `'compressed'`.
[`'expanded'` and `'compact'` are not currently supported by [libsass]]

#### precision
`precision` is a `Number` that will be used to determine how many digits after the decimal will be allowed. For instance, if you had a decimal number of `1.23456789` and a precision of `5`, the result will be `1.23457` in the final CSS.

#### sourceComments
`sourceComments` is a `Boolean` flag to determine what debug information is included in the output file.

#### omitSourceMapUrl
`omitSourceMapUrl` is a `Boolean` flag to determine whether to include `sourceMappingURL` comment in the output file.

#### sourceMap
If your `sourceComments` option is set to `map`, `sourceMap` allows setting a new path context for the referenced Sass files.
The source map describes a path from your CSS file location, into the the folder where the Sass files are located. In most occasions this will work out-of-the-box but, in some cases, you may need to set a different output.

#### stats
`stats` is an empty `Object` that will be filled with stats from the compilation:

```javascript
{
    entry: "path/to/entry.scss",    // or just "data" if the source was not a file
    start: 10000000,                // Date.now() before the compilation
    end:   10000001,                // Date.now() after the compilation
    duration: 1,                    // end - start
    includedFiles: [ ... ],         // absolute paths to all related scss files
    sourceMap: "..."                // the source map string or null
}
```

`includedFiles` isn't sorted in any meaningful way, it's just a list of all imported scss files including the entry.

### renderFile()

Same as `render()` but writes the CSS and sourceMap (if requested) to the filesystem.

#### outFile

`outFile` specifies where to save the CSS.

#### sourceMap

`sourceMap` specifies that the source map should be saved.

- If falsy the source map will not be saved
- If `sourceMap === true` the source map will be saved to the
standard location of `path.basename(options.outFile) + '.map'`
- Otherwise specifies the path (relative to the `outFile`)
where the source map should be saved


### Examples

```javascript
var sass = require('node-sass');
var stats = {};
sass.render({
	data: 'body{background:blue; a{color:black;}}',
	success: function(css) {
        console.log(css);
        console.log(stats);
	},
	error: function(error) {
		console.log(error);
	},
	includePaths: [ 'lib/', 'mod/' ],
	outputStyle: 'compressed',
    stats: stats
});
// OR
console.log(sass.renderSync({
	data: 'body{background:blue; a{color:black;}}',
	outputStyle: 'compressed',
    stats: stats
}));
console.log(stats);
```

### Edge-case behaviours

* In the case that both `file` and `data` options are set, node-sass will only attempt to honour the `file` directive.


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

    git clone --recursive https://github.com/sass/node-sass.git
    cd node-sass
    git submodule update --init --recursive
    npm install
    npm install -g node-gyp
    node-gyp rebuild

## Command Line Interface

The interface for command-line usage is fairly simplistic at this stage, as seen in the following usage section.

Output will be saved with the same name as input SASS file into the current working directory if it's omitted.

### Usage
 `node-sass [options] <input.scss> [<output.css>]`

 **Options:**

      --output-style         CSS output style (nested|expanded|compact|compressed)  [default: "nested"]
      --source-comments      Include debug info in output                           [default: false]
      --omit-source-map-url  Omit source map URL comment from output                [default: false]
      --include-path         Path to look for @import-ed files                      [default: cwd]
      --help, -h             Print usage info

## Post-install Build

Install runs a series of Mocha tests to see if your machine can use the pre-built [libsass] which will save some time during install. If any tests fail it will build from source.

If you know the pre-built version will work and do not want to wait for the tests to run you can skip the tests by setting the environment variable `SKIP_NODE_SASS_TESTS` to true.

      SKIP_NODE_SASS_TESTS=true npm install

## Maintainers

This module is brought to you and maintained by the following people:

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

Copyright (c) 2013 Andrew Nesbitt. See [LICENSE](https://github.com/sass/node-sass/blob/master/LICENSE) for details.

[libsass]: https://github.com/hcatlin/libsass
