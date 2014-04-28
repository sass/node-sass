sass2scss
=========

It may just work or is probably horribly broken!

C++ port of https://github.com/mgreter/OCBNET-CSS3/blob/master/bin/sass2scss.  
This implementation is currently far ahead of the previous perl implementation!  

Converts old indented sass syntax to newer scss syntax. My C++ is very rusty so it may
contain obvious errors and memory leaks. I only tested some basic examples from
http://sass-lang.com/documentation/file.INDENTED_SYNTAX.html. The scripts reads from STDIN
and writes to STDOUT, so you should be able to pipe sass files to scss processors.

Added some unit tests for sass2scss to https://github.com/mgreter/CSS-Sass.  
This may hopefully will get integrated to https://github.com/hcatlin/libsass.  

-> https://github.com/hcatlin/libsass/pull/181  
-> https://github.com/hcatlin/libsass/issues/16  


Options
=======

```
sass2scss [options] < file.sass
```

```
-p, --pretty       pretty print output
```
