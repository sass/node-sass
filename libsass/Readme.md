Libsass
=======

by Aaron Leung and Hampton Catlin (@hcatlin)

http://github.com/hcatlin/libsass

Libsass is just a library, but if you want to RUN libsass,
then go to http://github.com/hcatlin/sassc or
http://github.com/hcatlin/sassruby or find your local 
implementer.

About
-----

Libsass is a C/C++ port of the Sass CSS precompiler. The original version was written in Ruby, but this version is meant for efficiency and portability.

This library strives to be light, simple, and easy to build and integrate with a variety of platforms and languages.

Usage
-----

While libsass is primarily implemented in C++, it provides a simple
C interface that is defined in [sass_inteface.h]. Its usage is pretty
straight forward.

First, you create a sass context struct. We use these objects to define
different execution parameters for the library. There are three 
different context types. 

    sass_context        //string-in-string-out compilation
    sass_file_context   //file-based compilation
    sass_folder_context //full-folder multi-file 

Each of the context's have slightly different behavior and are
implemented seperately. This does add extra work to implementing
a wrapper library, but we felt that a mixed-use context object
provides for too much implicit logic. What if you set "input_string"
AND "input_file"... what do we do? This would introduce bugs into
wrapper libraries that would be difficult to debug. 

We anticipate that most adapters in most languages will define
their own logic for how to separate these use cases based on the
language. For instance, the original Ruby interface has a combined
interface, but is backed by three different processes.

To generate a context, use one of the following methods.

    new_sass_context()
    new_sass_file_context()
    new_sass_folder_context()

Again, please see the sass_interface.h for more information.

And, to get even more information, then please see the implementations
in SassC and SassC-Ruby.

About Sass
----------

Sass is a CSS pre-processor language to add on exciting, new, 
awesome features to CSS. Sass was the first language of its kind
and by far the most mature and up to date codebase.

Sass was originally created by the co-creator of this library, 
Hampton Catlin (@hcatlin). The extension and continuing evolution
of the language has all been the result of years of work by Nathan
Weizenbaum (@nex3) and Chris Eppstein (@chriseppstein). 

For more information about Sass itself, please visit http://sass-lang.com

Contribution Agreement
----------------------

Any contribution to the project are seen as copyright assigned to Hampton Catlin. Your contribution warrants that you have the right to assign copyright on your work. This is to ensure that the project remains free and open -- similar to the Apache Foundation.

Our license is designed to be as simple as possible.


