# Troubleshooting

This document covers some common node-sass issues and how to resolve them. You should always follow these steps before opening a new issue.

## TOC

- [Installation problems](#installation-problems)
  - [Assertion failed: (handle->flags & UV_CLOSING), function uv__finish_close](#assertion-failed-handle-flags-&-uv_closing-function-uv__finish_close)
  - [Cannot find module '/root/<...>/install.js'](#cannot-find-module-rootinstalljs)
    - [Linux](#linux)
- [Glossary](#glossary)
  - [Which node runtime am I using?](#which-node-runtime-am-i-using)
  - [Which version of node am I using?](#which-version-of-node-am-i-using)
  - [Debugging installation issues.](#debugging-installation-issues)
    - [Windows](#windows)
    - [Linux/OSX](#linuxosx)
- [Using node-sass with Visual Studio 2015 Task Runner.](#using-node-sass-with-visual-studio-2015-task-runner)

## Installation problems

### Assertion failed: (handle->flags & UV_CLOSING), function uv__finish_close

This issue primarily affected early node-sass@3.0.0 alpha and beta releases, although it did occassionally happen in node-sass@2.x.

The only fix for this issue is to update to node-sass >= 3.0.0.

If this didn't solve your problem please open an issue with the output from [our debugging script](#debugging-installation-issues).


### Cannot find module '/root/<...>/install.js'

#### Linux

This can happen if you are install node-sass as `root`, or globally with `sudo`. This is a security feature of `npm`. You should always avoid running `npm` as `sudo` because install scripts can be unintentionally malicious.
Please check [npm documentation on fixing permissions](https://docs.npmjs.com/getting-started/fixing-npm-permissions).

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

Node sass runs some install scripts to make it as easy to use as possible, but some times there can be issues. Before opening a new issue please follow the instructions for [Windows](#windows) or [Linux/OSX](#linuxosx) and provide their output in you [GitHub issue](https://github.com/sass/node-sass/issues).

**Remember to always search before opening a new issue**.

#### Windows

Firstly create a clean work space.

```sh
mkdir \temp1
cd \temp1
```

Check your `COMSPEC` environment variable.

```sh
node -p process.env.comspec
```

Please make sure the variable points to `C:\WINDOWS\System32\cmd.exe`

Gather some basic diagnostic information.

```sh
npm -v
node -v
node -p process.versions
node -p process.platform
node -p process.arch
```

Clean npm cache

```sh
npm cache clean
```

Install the latest node-sass

```sh
npm install -ddd node-sass > npm.log 2> npm.err
```

Note which version was installed by running

```sh
npm ls node-sass
```
```sh
y@1.0.0 /tmp
└── node-sass@3.8.0
```

If node-sass could not be installed successfully, please publish your `npm.log`
and `npm.err` files for analysis.

You can [download reference known-good logfiles](https://gist.github.com/saper/62b6e5ea41695c1883e3)
to compare your log against.

If node-sass install successfully lets gather some basic installation infomation.

```sh
node -p "require('node-sass').info"
```
```sh
node-sass       3.8.0   (Wrapper)       [JavaScript]
libsass         3.3.6   (Sass Compiler) [C/C++]
```

If the node-sass installation process produced an error, open the vendor folder.

```sh
cd node_modules\node-sass\vendor
```

Then, using the version number we gather at the beginning, go to https://github.com/sass/node-sass/releases/tag/v<your-version>.

There you should see a folder with same name as the one in the `vendor` folder. Download the `binding.node` file from that folder and replace your own with it.

Test if that worked by gathering some basic installation infomation.

```sh
node -p "require('node-sass').info"
```
```sh
node-sass       3.8.0   (Wrapper)       [JavaScript]
libsass         3.3.6   (Sass Compiler) [C/C++]
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

Note which version was installed by running

```sh
npm ls node-sass
```
```sh
y@1.0.0 /tmp
└── node-sass@3.8.0
```

If node-sass install successfully lets gather some basic installation infomation.

```sh
node -p "require('node-sass').info"
```
```sh
node-sass       3.8.0   (Wrapper)       [JavaScript]
libsass         3.3.6   (Sass Compiler) [C/C++]
```

If the node-sass installation process produced an error, open the vendor folder.

```sh
cd node_modules/node-sass/vendor
```

Then, using the version number we gather at the beginning, go to https://github.com/sass/node-sass/releases/tag/v<your-version>.

There you should see a folder with same name as the one in the `vendor` folder. Download the `binding.node` file from that folder and replace your own with it.

Test if that worked by gathering some basic installation infomation.

```sh
node -p "require('node-sass').info"
```
```sh
node-sass       3.8.0   (Wrapper)       [JavaScript]
libsass         3.3.6   (Sass Compiler) [C/C++]
```

If this still produces an error please open an issue with the output from these steps.

### Using node-sass with Visual Studio 2015 Task Runner.

If you are using node-sass with VS2015 Task Runner Explorer, you need to make sure that the version of node.js (or io.js) is same as the one you installed node-sass with. This is because for each node.js runtime modules version (`node -p process.versions.modules`), we have a separate build of native binary. See [#532](https://github.com/sass/node-sass/issues/532).

Alternatively, if you prefer using system-installed node.js (supposedly higher version than one bundles with VS2015), you may want to point Visual Studio 2015 to use it for task runner jobs by following the guidelines available at: http://blogs.msdn.com/b/webdev/archive/2015/03/19/customize-external-web-tools-in-visual-studio-2015.aspx.
