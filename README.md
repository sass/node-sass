sass2scss
=========

[![Build Status](https://travis-ci.org/mgreter/sass2scss.svg?branch=master)](https://travis-ci.org/mgreter/sass2scss)

C++ tool/library to convert indented sass syntax to the newer scss syntax. This
was initially a port of a previous perl implementation. Its primary intention
was to bring indented sass syntax support to [`libsass`](https://github.com/sass/libsass).
`sass2scss` is included in `libsass` since [version 2.0](https://github.com/sass/libsass/releases/tag/v2.0).

Unit Tests
==========

I added around 60 unit tests for `sass2sccs` to the `libsass` perl binding
[`CSS-Sass`](https://github.com/sass/perl-libsass/blob/master/t/06_sass_to_scss.t).

Command Line Utility
====================

```
sass2scss [options] < file.sass
```

```
-p, --pretty       pretty print output
-c, --convert      convert src comments
-s, --strip        strip all comments
-k, --keep         keep all comments
-h, --help         help text
-v, --version      version information
```

`--pretty` can be repeated up to 3 times to add even more linefeeds (lf).

- 0: Write everything on one line (`minimized`)
- 1: Add lf after opening bracket (`lisp style`)
- 2: Add lf after opening and before closing bracket (`1TBS style`)
- 3: Add lf before/after opening and before closing (`allman style`)

The `lisp style` is the only output style that should not alter the line count
of the input file. This is the best option if you still want to use `source-maps`,
since it should only change the source by a few inserted chars. So far
`sass2sccs` does not produce source-maps and `libsass` will not be able to
produce 100% accurate `source-maps` for indented sass syntax input files!