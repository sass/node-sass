
## Contributing
 * Fork the project.
 * Make your feature addition or bug fix.
 * Add documentation if necessary. 
 * Add tests for it. This is important so I don't break it in a future version unintentionally.
 * Send a pull request. Bonus points for topic branches.

## Bug reports

A bug is a _demonstrable problem_ that is caused by the code in the repository. Good bug reports are extremely helpful, so thanks!

Guidelines for bug reports:

1. **Use the GitHub issue search** &mdash; check if the issue has already been
 reported.
2. **Check if the issue has been fixed** &mdash; try to reproduce it using the
 latest `master` branch in the repository.
3. **Isolate the problem** &mdash; ideally create an
 [SSCCE](http://www.sscce.org/) and a live example.
4. Always consult the [Troubleshooting guide](/TROUBLESHOOTING.md) before opening a new issue.


A good bug report shouldn't leave others needing to chase you up for more information. Please try to be as detailed as possible in your report. What is your environment? What steps will reproduce the issue? What browser(s) and OS experience the problem? Do other browsers show the bug differently? What would you expect to be the outcome? All these details will help people to fix any potential bugs.

If you are facing binary related issues, please create a gist (see [Creating gists on GitHub](https://help.github.com/articles/creating-gists/)) with the output of this set of commands:

  * For *nix: https://gist.github.com/am11/9f429c211822a9b15aee.
  * For win: https://gist.github.com/am11/e5de3c49c219f0811e1d.

If this project is missing an API or command line flag that has been added to [libsass], then please open an issue here. We will then look at updating our [libsass] submodule and create a new release. You can help us create the new release by rebuilding binaries, and then creating a pull request to the [node-sass-binaries](https://github.com/sass/node-sass-binaries) repo.

## Reporting Sass compilation and syntax issues

Please check for [issues on the libsass repo](https://github.com/hcatlin/libsass/issues) (as there is a good chance that it may already be an issue there for it), and otherwise [create a new issue there](https://github.com/sass/libsass/issues/new).

[libsass]: https://github.com/sass/libsass
