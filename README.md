sass2scss
=========

It may just work or is probably horribly broken!

C++ port of https://github.com/mgreter/OCBNET-CSS3/blob/master/bin/sass2scss.

Converts old indented sass syntax to newer scss syntax. My C++ is very rusty so it may
contain obvious errors and memory leaks. I only tested some basic examples from
http://sass-lang.com/documentation/file.INDENTED_SYNTAX.html. The scripts reads from STDIN
and writes to STDOUT, so you should be able to pipe sass files to scss processors.


Options
=======

```
sass2scss [options] < file.sass
```

```
-p, --pretty       pretty print output
```
