# Command Line Interface

The interface for command-line usage is fairly simplistic at this stage, as seen in the following usage section.

Output will be sent to stdout if the `--output` flag is omitted.

## Usage
 `node-sass [options] <input> [output]`
 `cat <input> | node-sass > output`

Example:

`node-sass src/style.scss dest/style.css`

 **Options:**

```bash
    -w, --watch                Watch a directory or file
    -r, --recursive            Recursively watch directories or files
    -o, --output               Output directory
    -x, --omit-source-map-url  Omit source map URL comment from output
    -i, --indented-syntax      Treat data from stdin as sass code (versus scss)
    -q, --quiet                Suppress log output except on error
    -v, --version              Prints version info
    --output-style             CSS output style (nested | expanded | compact | compressed)
    --indent-type              Indent type for output CSS (space | tab)
    --indent-width             Indent width; number of spaces or tabs (maximum value: 10)
    --linefeed                 Linefeed style (cr | crlf | lf | lfcr)
    --source-comments          Include debug info in output
    --source-map               Emit source map
    --source-map-contents      Embed include contents in map
    --source-map-embed         Embed sourceMappingUrl as data URI
    --source-map-root          Base path, will be emitted in source-map as is
    --include-path             Path to look for imported files
    --follow                   Follow symlinked directories
    --precision                The amount of precision allowed in decimal numbers
    --importer                 Path to .js file containing custom importer
    --functions                Path to .js file containing custom functions
    --help                     Print usage info
```

The `input` can be either a single `.scss` or `.sass`, or a directory. If the input is a directory the `--output` flag must also be supplied.

Also, note `--importer` takes the (absolute or relative to pwd) path to a js file, which needs to have a default `module.exports` set to the importer function. See our test [fixtures](https://github.com/sass/node-sass/tree/974f93e76ddd08ea850e3e663cfe64bb6a059dd3/test/fixtures/extras) for example.

The `--source-map` option accepts a boolean value, in which case it replaces destination extension with `.css.map`. It also accepts path to `.map` file and even path to the desired directory.
When compiling a directory `--source-map` can either be a boolean value or a directory.
