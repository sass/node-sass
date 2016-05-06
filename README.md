# <img width="77px" alt="Sass logo" src="https://rawgit.com/sass/node-sass/master/media/logo.svg" /> node-sass

#### Supported Node.js versions 0.10, 0.12, 1, 2, 3, 4, 5, and 6.

[![NPM Stats](https://nodei.co/npm/node-sass.png?downloads=true&downloadRank=true&stars=true)](https://www.npmjs.com/package/node-sass)

[![Build Status](https://travis-ci.org/sass/node-sass.svg?branch=master&style=flat)](https://travis-ci.org/sass/node-sass)
[![Build status](https://ci.appveyor.com/api/projects/status/22mjbk59kvd55m9y/branch/master)](https://ci.appveyor.com/project/sass/node-sass/branch/master)
[![Dependency Status](https://david-dm.org/sass/node-sass.svg?theme=shields.io)](https://david-dm.org/sass/node-sass)
[![Coverage Status](https://coveralls.io/repos/sass/node-sass/badge.svg?branch=master)](https://coveralls.io/r/sass/node-sass?branch=master)
[![Inline docs](http://inch-ci.org/github/sass/node-sass.svg?branch=master)](http://inch-ci.org/github/sass/node-sass)

Node-sass is a library that provides binding for Node.js to [LibSass], the C version of the popular stylesheet preprocessor, Sass. LibSass allows you to natively compile `.scss` files to css at incredible speed.

## Install

```
npm install node-sass
```

Some users have reported issues installing on Ubuntu due to `node` being registered to another package. [Follow the official NodeJS docs](https://github.com/nodejs/node-v0.x-archive/wiki/Installing-Node.js-via-package-manager) to install NodeJS so that `#!/usr/bin/env node` correctly resolved.

Compiling versions 0.9.4 and above on Windows machines requires [Visual Studio 2013 WD](https://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop). If you have multiple VS versions, use ```npm install``` with the ```--msvs_version=2013``` flag also use this flag when rebuilding the module with node-gyp or nw-gyp.

**Having installation troubles? Check out our [Troubleshooting guide](/TROUBLESHOOTING.md).**

## Documentation

See the [docs](docs) folder for specific build and API topics. The JSDocs can be found on [Inch-CI](http://inch-ci.org/github/sass/node-sass))

## Bugs and feature requests

Have a bug or a feature request? Please first read the [issue guidelines](https://github.com/sass/node-sass/blob/master/CONTRIBUTING.md#bug-reports) and search for existing and closed issues. If your problem or idea is not addressed yet, [please open a new issue](https://github.com/sass/node-sass/issues/new).

## Usage

```javascript
var sass = require('node-sass');
sass.render({
  file: scss_filename,
  [, options..]
}, function(err, result) { /*...*/ });
// OR
var result = sass.renderSync({
  data: scss_content
  [, options..]
});
```

## Maintainers

This module is brought to you and maintained by the following people:

* Michael Mifsud - Project Lead ([Github](https://github.com/xzyfer) / [Twitter](https://twitter.com/xzyfer))
* Andrew Nesbitt ([Github](https://github.com/andrew) / [Twitter](https://twitter.com/teabass))
* Dean Mao ([Github](https://github.com/deanmao) / [Twitter](https://twitter.com/deanmao))
* Brett Wilkins ([Github](https://github.com/bwilkins) / [Twitter](https://twitter.com/bjmaz))
* Keith Cirkel ([Github](https://github.com/keithamus) / [Twitter](https://twitter.com/Keithamus))
* Laurent Goderre ([Github](https://github.com/laurentgoderre) / [Twitter](https://twitter.com/laurentgoderre))
* Nick Schonning ([Github](https://github.com/nschonni) / [Twitter](https://twitter.com/nschonni))
* Adam Yeats ([Github](https://github.com/adamyeats) / [Twitter](https://twitter.com/adamyeats))
* Adeel Mujahid ([Github](https://github.com/am11) / [Twitter](https://twitter.com/adeelbm))

## Contributors

We :heart: our contributors! A special thanks to all those who have clocked in some dev time on this project, we really appreciate your hard work. You can find [a full list of those people here.](https://github.com/sass/node-sass/graphs/contributors)

Please read through our [contributing guidelines](https://github.com/sass/node-sass/blob/master/CONTRIBUTING.md). Included are directions for opening issues, coding standards, and notes on development.

Moreover, if your pull request contains JavaScript patches or features, you must include [relevant unit tests](https://github.com/sass/node-sass/tree/master/test).

Editor preferences are available in the [editor config](https://github.com/sass/node-sass/blob/master/.editorconfig) for easy use in common text editors. Read more and download plugins at http://editorconfig.org.

## Community

Follow @nodesass on twitter for release updates: https://twitter.com/nodesass

Ask questions in our Slack channel [![Join us in Slack](https://libsass-slack.herokuapp.com/badge.svg)](https://libsass-slack.herokuapp.com/)

## Copyright

Copyright (c) 2015 Andrew Nesbitt. See [LICENSE](https://github.com/sass/node-sass/blob/master/LICENSE) for details.

[LibSass]: https://github.com/sass/libsass
