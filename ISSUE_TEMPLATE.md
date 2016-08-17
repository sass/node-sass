## Reporting an installation issue
1. I'm running on a [supported platform](https://github.com/sass/node-sass-binaries/blob/master/README.md#compatibility)
1. Have you recently installed a new version of Node? Native bindings are downloading for the specific version of Node when you install `node-sass`. Delete your `node_modules` and then re-install.
1. Create [Gist](https://gist.github.com/) with the npm.log file and add it to this issue
1. I've [searched for existing issues](https://github.com/sass/node-sass/search?type=Issues)
1. I've followed the [Troubleshooting Guide](TROUBLESHOOTING.md)

## My Sass isn't compiling
1. My code compiles on [SassMeister](http://www.sassmeister.com/) when using the Ruby Sass complier
1. Try the Beta version of `node-sass` to see if the latest `libsass` fixes this
1. Create a reproducible sample, and [open an issue on `libsass`](https://github.com/sass/libsass/issues/new). Compiler issues need to be fixed there, not here.
