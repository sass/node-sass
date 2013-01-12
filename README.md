##node-sass

[![Build Status](https://secure.travis-ci.org/andrew/node-sass.png?branch=master)](https://travis-ci.org/andrew/node-sass)

Node-sass is a library that provides binding for Node.js to libsass, the C version of the popular stylesheet preprocessor, Sass.

It allows you to natively compile .scss files to css at incredible speed and automatically via a connect middleware.

Find it on npm: <http://search.npmjs.org/#/node-sass>

## Install

    npm install

## Usage

    var sass = require('node-sass');
    sass.render(scss_content, callback [, options]);

Especially, the options argument is optional. It support two attribute: `include_paths` and `output_style`, both of them are optional.

`include_paths` is an `Array`, you can add a sass import path.

`output_style` is a `String`, its value should be one of `'nested', 'expanded', 'compact', 'compressed'`.
[Important: currently the argument `output_style` has some problem which may cause the output css becomes nothing because of the libsass, so you should not use it now!]

Here is an example:

    var sass = require('node-sass');
    sass.render('body{background:blue; a{color:black;}}', function(err, css){
      console.log(css)
    }/*, { include_paths: [ 'lib/', 'mod/' ], output_style: 'compressed' }*/);

## Connect/Express middleware

Recompile `.scss` files automatically for connect and express based http servers

    var server = connect.createServer(
      sass.middleware({
          src: __dirname
        , dest: __dirname + '/public'
        , debug: true
      }),
      connect.static(__dirname + '/public')
    );

Heavily inspired by <https://github.com/LearnBoost/stylus>

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
