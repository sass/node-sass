### Get the sources
```bash
# using git is preferred
git clone https://github.com/sass/libsass.git
# only needed for sassc and/or testsuite
git clone https://github.com/sass/sassc.git libsass/sassc
git clone https://github.com/sass/sass-spec.git libsass/sass-spec
```

### Decide for static or shared library

`libsass` can be built and linked as a `static` or as a `shared` library. The default is `static`. To change it you can set the `BUILD` environment variable:

```bash
export BUILD="shared"
```

Alternatively you can also define it directly when calling make:

```bash
BUILD="shared" make ...
```

### Compile the library
```bash
make -C libsass -j5
```

### Results can be found in
```bash
$ ls libsass/lib
libsass.a libsass.so
```

### Compling sassc

```bash
# Let build know library location
export SASS_LIBSASS_PATH="`pwd`/libsass"
# Invokes the sassc makefile
make -C libsass -j5 sassc
```

### Run the spec test-suite

```bash
# needs ruby available
# also gem install minitest
make -C libsass -j5 test_build
```
