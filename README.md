##node-sass

Node bindings to libsass

*work in progress*

## Install

    cd libsass && make && cd ..
    node-waf configure && node-waf build

## Usage

    var sass = require('./sass');
    sass.render('body{background:blue; a{color:black;}}', function(css){
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

* async cpp
* error handling
* file context
* folder context
* sass compression options