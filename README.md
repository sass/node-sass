##node-sass

Node bindings to libsass

*work in progress*

## Install

    node-waf configure && node-waf build

## Usage

    var sass = require('./sass');
    sass.render('body{background:blue; a{color:black;}}');

## TODO

* error handling
* express middleware
* file context
* folder context
