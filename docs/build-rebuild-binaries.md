# Rebuilding binaries

Node-sass includes pre-compiled binaries for popular platforms, to add a binary for your platform follow these steps:

Check out the project:

```bash
git clone --recursive https://github.com/sass/node-sass.git
cd node-sass
git submodule update --init --recursive
npm install
node scripts/build -f  # use -d switch for debug release
# if succeeded, it will generate and move
# the binary in vendor directory.
```
