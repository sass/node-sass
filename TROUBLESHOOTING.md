# Troubleshooting

This document covers some common node-sass issues and how to resolve them. You should always follow these steps before opening a new issue.

## TOC

- [Installation problems](#installation-problems)
  - ["Module did not self-register"](#module-did-not-self-register)
    - [Windows](#windows)
  - [Assertion failed: (handle->flags & UV_CLOSING), function uv__finish_close](#assertion-failed-handle-flags-&-uv_closing-function-uv__finish_close)
  - [Cannot find module '/root/<...>/install.js'](#cannot-find-module-rootinstalljs)
    - [Linux](#linux)
- [Glossary](#glossary)
  - [Which node runtime am I using?](#which-node-runtime-am-i-using)
  - [Which version of node am I using?](#which-version-of-node-am-i-using)
  - [Debugging installation issues.](#debugging-installation-issues)
    - [Windows](#windows-1)
    - [Linux/OSX](#linuxosx)

## Installation problems

### "Module did not self-register"

#### Windows

This can happen if you are using io.js rather than node. Unfortunately this issue is due to an [issue with a library](https://github.com/iojs/io.js/issues/751) we depend on and such is currently out of our control. The problem is a side effect of iojs aliasing the `node.exe` binary to itself.

To work around this now be sure to execute your node commands using `iojs` instead of `node` i.e.

```sh
$ iojs ./node_modules/.bin/node-sass --version
```

If this didn't solve your problem please open an issue with the output from [our debugging script](#debugging-installation-issues).


### Assertion failed: (handle->flags & UV_CLOSING), function uv__finish_close

This issue primarily affected early node-sass@3.0.0 alpha and beta releases, although it did occassionally happen in node-sass@2.x.

The only fix for this issue is to update to node-sass >= 3.0.0.

If this didn't solve your problem please open an issue with the output from [our debugging script](#debugging-installation-issues).


### Cannot find module '/root/<...>/install.js'

#### Linux

This can happen if you are install node-sass as `root`, or globally with `sudo`. This is a security feature of `npm`. You should always avoid running `npm` as `sudo` because install scripts can be unintentionally malicious.

If you must however, you can work around this error by using the `--unsafe-perm` flag with npm install i.e.

```sh
$ sudo npm install --unsafe-perm -g node-sass
```

If this didn't solve your problem please open an issue with the output from [our debugging script](#debugging-installation-issues).


## Glossary


### Which node runtime am I using?

There are two primary node runtimes, Node.js and io.js, both of which are supported by node-sass. To determine which you are currenty using you first need to determine [which node runtime](#which-node-runtime-am-i-using-glossaryruntime) you are running.

```
node -v
```

If the version reported begins with a `0`, you are running Node.js, otherwise you are running io.js.


### Which version of node am I using?

To determine which version of Node.js or io.js you are currenty using run the following command in a terminal.

```
node -v
```

The resulting value the version you are running.


### Debugging installation issues.

Node sass runs some install scripts to make it as easy to use as possible, but some times there can be issues. Before opening a new issue please follow the instructions for [Windows](#windows-1) or [Linux/OSX](#linuxosx) and provide their output in you [GitHub issue](https://github.com/sass/node-sass/issues).

**Remember to always search before opening a new issue**.

#### Windows

Firstly create a clean work space.

```sh
mkdir \temp1
cd \temp1
```

Gather some basic diagnostic information.

```sh
npm -v              
node -v
node -p process.versions
node -p process.platform
node -p process.arch
```

Install the latest node-sass

```sh
npm install node-sass
```

Note which version was installed by opening the `package.json` file with a text editor.

```json
{
  "name": "node-sass",
  "version": "3.0.0",
  "libsass": "3.2.0",
}
```

If node-sass install successfully lets gather some basic installation infomation.

```sh
.\node_modules\.bin\node-sass --version
node -p "console.log(require('node-sass').info)"
```

If the node-sass installation process produced an error, open the vendor folder.

```sh
cd node_modules\node-sass\vendor
```

Then, using the version number we gather at the beginning, go to https://github.com/sass/node-sass-binaries/tree/v<your-version>.

There you should see a folder with same name as the one in the `vendor` folder. Download the `binding.node` file from that folder and replace your own with it.

Test if that worked by gathering some basic installation infomation.

```sh
.\node_modules\.bin\node-sass --version
node -p "console.log(require('node-sass').info)"
```

If this still produces an error please open an issue with the output from these steps.


#### Linux/OSX

Firstly create a clean work space.

```sh
mkdir ~/temp1
cd ~/temp1
```

Gather some basic diagnostic information.

```sh
npm -v              
node -v
node -p process.versions
node -p process.platform
node -p process.arch
```

Install the latest node-sass

```sh
npm install node-sass
```

Note which version was installed by opening the `package.json` file with a text editor.

```json
{
  "name": "node-sass",
  "version": "3.0.0",
  "libsass": "3.2.0",
}
```

If node-sass install successfully lets gather some basic installation infomation.

```sh
./node_modules/.bin/node-sass --version
node -p "console.log(require('node-sass').info)"
```

If the node-sass installation process produced an error, open the vendor folder.

```sh
cd node_modules/node-sass/vendor
```

Then, using the version number we gather at the beginning, go to https://github.com/sass/node-sass-binaries/tree/v<your-version>.

There you should see a folder with same name as the one in the `vendor` folder. Download the `binding.node` file from that folder and replace your own with it.

Test if that worked by gathering some basic installation infomation.

```sh
.\node_modules\.bin\node-sass --version
node -p "console.log(require('node-sass').info)"
```

If this still produces an error please open an issue with the output from these steps.
