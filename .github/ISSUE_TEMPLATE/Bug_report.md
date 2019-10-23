---
name: Bug report
about: If you're having an issue with installing node-sass or the compiled results
  look incorrect

---

<!--
Before opening an Install issue:

- If you're running Node 13, you must be running node-sass 4.13.
- Check that the version of node-sass you're trying to install supports your version of Node by looking at the release page for that version https://github.com/sass/node-sass/releases
- If you're running the latest versions of Node, you'll likely need the latest node-sass, we don't backport support to old versions of node-sass
- Read the common workarounds in https://github.com/sass/node-sass/blob/master/TROUBLESHOOTING.md
- Common issues we're tracking can be found by looking at the `Pinned` label https://github.com/sass/node-sass/issues?utf8=%E2%9C%93&q=is%3Aissue+is%3Aopen+label%3APinned+

**When reporting any bug, YOU MUST PROVIDE THE FOLLOWING INFORMATION
or your issue will be closed without discussion**
-->

- NPM version (`npm -v`):
- Node version (`node -v`):
- Node Process (`node -p process.versions`):
- Node Platform (`node -p process.platform`):
- Node architecture (`node -p process.arch`):
- node-sass version (`node -p "require('node-sass').info"`):
- npm node-sass versions (`npm ls node-sass`):

<!--

When encountering a syntax, or compilation issue:

- Please note that we cannot backport fixes to old versions, so ensure that you are running the latest release https://github.com/sass/node-sass/releases
- Search for duplicate or closed issues https://github.com/sass/node-sass/issues?utf8=%E2%9C%93&q=is%3Aissue
- Validate with http://sassmeister.com/ that the code works with Ruby Sass, then open an issue on `LibSass` https://github.com/sass/LibSass/issues/new

Sorry you didn't have the experience you expected.

-->
