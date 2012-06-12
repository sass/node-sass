##node-sass

Node bindings to libsass

Find it on npm: <http://search.npmjs.org/#/node-sass>

## Install

    npm install

## Usage

    var sass = require('sass');
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
