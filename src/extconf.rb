require 'mkmf'
# .. more stuff
$LIBPATH.push(Config::CONFIG['libdir'])
create_makefile("libsass")