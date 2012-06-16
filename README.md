##node-sass

Node-sass is a library that provides binding for Node.js to libsass, the C version of the popular stylesheet preprocessor, Sass.

It allows you to natively compile .scss files to css at incredible speed and automatically via a connect middleware.

Find it on npm: <http://search.npmjs.org/#/node-sass>

## Install

    npm install

## Usage

    var sass = require('node-sass');
    sass.render('body{background:blue; a{color:black;}}', function(err, css){
      console.log(css)
    });

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
