##node-sass

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

* sass compression options
* publish npm
* file context?
* folder context?
