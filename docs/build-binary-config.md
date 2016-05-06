# Binary configuration parameters

node-sass supports different configuration parameters to change settings related to the sass binary such as binary name, binary path or alternative download path. Following parameters are supported by node-sass:

Variable name    | .npmrc parameter | Process argument   | Value
-----------------|------------------|--------------------|------
SASS_BINARY_NAME | sass_binary_name | --sass-binary-name | path
SASS_BINARY_SITE | sass_binary_site | --sass-binary-site | URL
SASS_BINARY_PATH | sass_binary_path | --sass-binary-path | path

These parameters can be used as environment variable:

* E.g. `export SASS_BINARY_SITE=http://example.com/`

As local or global [.npmrc](https://docs.npmjs.com/misc/config) configuration file:

* E.g. `sass_binary_site=http://example.com/`

As a process argument:

* E.g. `npm install node-sass --SASS_BINARY_SITE=http://example.com/`
