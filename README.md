##node-sass

[![Build Status](https://secure.travis-ci.org/andrew/node-sass.png?branch=master)](https://travis-ci.org/andrew/node-sass)

Node-sass is a library that provides binding for Node.js to libsass, the C version of the popular stylesheet preprocessor, Sass.

It allows you to natively compile .scss files to css at incredible speed and automatically via a connect middleware.

Find it on npm: <https://npmjs.org/package/node-sass>

## Install

    npm install node-sass

## Usage

```javascript
var sass = require('node-sass');
sass.render(scss_content, callback [, options]);
// OR
var css = sass.renderSync(scss_content);
```

Especially, the options argument is optional. It support two attributes: `includePaths` and `outputStyle`, both of which are optional.

`includePaths` is an `Array`, you can add a sass import path.

`outputStyle` is a `String`, its value should be one of `'nested', 'expanded', 'compact', 'compressed'`.
[Important: currently the argument `outputStyle` has some problem which may cause the output css becomes nothing because of the libsass, so you should not use it now!]

Here is an example:

```javascript
var sass = require('node-sass');
sass.render('body{background:blue; a{color:black;}}', function(err, css){
  console.log(css)
}/*, { includePaths: [ 'lib/', 'mod/' ], outputStyle: 'compressed' }*/);
// OR
console.log(sass.renderSync('body{background:blue; a{color:black;}}'));
```

## Connect/Express middleware

Recompile `.scss` files automatically for connect and express based http servers

```javascript
var server = connect.createServer(
  sass.middleware({
      src: __dirname
    , dest: __dirname + '/public'
    , debug: true
  }),
  connect.static(__dirname + '/public')
);
```

Heavily inspired by <https://github.com/LearnBoost/stylus>

## Rebuilding binaries

Node-sass includes pre-compiled binaries for popular platforms, to add a binary for your platform follow these steps:

Check out the project:

    git clone https://github.com/andrew/node-sass.git
    cd node-sass
    npm install
    npm install -g node-gyp
    git submodule init
    git submodule update
    node-gyp rebuild

Replace the prebuild binary with your newly generated one

    cp build/Release/binding.node precompiled/*your-platform*/binding.node

## TODO

* better error handling
* file context
* folder context

### Note on Patches/Pull Requests

 * Fork the project.
 * Make your feature addition or bug fix.
 * Add documentation if necessary.
 * Add tests for it. This is important so I don't break it in a future version unintentionally.
 * Send a pull request. Bonus points for topic branches.

## Copyright

Copyright (c) 2013 Andrew Nesbitt. See [LICENSE](https://github.com/andrew/node-sass/blob/master/LICENSE) for details.
