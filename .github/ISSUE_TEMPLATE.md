Before opening an issue:
- Read the common workarounds in the [TROUBLESHOOTING.md](https://github.com/sass/node-sass/blob/master/TROUBLESHOOTING.md)
- [Search for duplicate or closed issues](https://github.com/sass/node-sass/issues?utf8=%E2%9C%93&q=is%3Aissue)
- [Validate](http://sassmeister.com/) that it runs with both Ruby Sass and LibSass
- Prepare a [reduced test case](https://css-tricks.com/reduced-test-cases/) for any bugs
- Read the [contributing guidelines](https://github.com/sass/node-sass/blob/master/CONTRIBUTING.md)

When reporting an bug, **you must provide this information**:

- NPM version (`npm -v`):
- Node version (`node -v`):
- Node Process (`node -p process.versions`):
- Node Platform (`node -p process.platform`):
- Node architecture (`node -p process.arch`):
- node-sass version (`node -p "require('node-sass').info"`):
- npm node-sass versions (`npm ls node-sass`):

When encountering a syntax, or compilation issue:

- [Open an issue on `LibSass`](https://github.com/sass/LibSass/issues/new). You
may link it back here, but any change will be required there, not here

*If you delete this text without following it, your issue will be closed.*
