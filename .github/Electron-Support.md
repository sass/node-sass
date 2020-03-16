## Supporting Electron (5-8)

In order to bring Electron support to all platforms (not just Windows), we need people to follow the below steps

### Steps

1) Create a project and run `` and 
    ```console
    npm install node-sass
    npm install --save-dev electron-rebuild
    ```

2) Once those have complete run the below *(resolve any errors - usually missing programmes i.e. python)*

    ```console
    # Target different architectures by appending 
    # --arch=x64 or --arch=ia32   etc. etc.

    ./node_modules/.bin/electron-rebuild -v=8 -w node-sass
    ./node_modules/.bin/electron-rebuild -v=7 -w node-sass
    ./node_modules/.bin/electron-rebuild -v=6 -w node-sass
    ./node_modules/.bin/electron-rebuild -v=5 -w node-sass
    ```

3) New folders would have been placed in the `/node-modules/node-sass/bin/` folder, rename the `.node` files to `binding.node`

4) Create a fork of this repository

**IN THE NEW FORK**

5) Add the newly created folders (from step 3) to the `/bin/` folder of your new fork

5) Update the platform support in this document

6) In file `\lib\extensions.js` on line 175 add your platform to the array (copy the id from below)

### Platform support
- [X] Windows x32 (win32-x64)
- [X] Windows x64 (win32-ia32)
- [ ] Darwin (darwin-x64) => 'darwin'
- [ ] FreeBSD (freebsd-ia32) => 'freebsd'
- [ ] Linux x32 (linux-ia32) => 'linux'
- [ ] Linux x64 (linux-x64) => 'linux'
- [ ] Linux/musl (linux_musl) => 'linux_musl'


#### References

[Electron - Using Native Node Modules](https://www.electronjs.org/docs/tutorial/using-native-node-modules)