import { PathLike } from 'fs'
declare module "node-sass" {
  type CssTimeUnit = "s" | "ms"
  type CssDistanceUnit = "cm" | "em" | "ex" | "in" | "mm" | "pc" | "pt" | "px"
  type CssNumberUnit = "%" | " " | CssDistanceUnit | CssTimeUnit
  class SassNumber {
    constructor(value: number, unit?: CssNumberUnit)
    getValue(): number
    getUnit(): CssNumberUnit
    setValue(value: number): void
    setUnit(unit: CssNumberUnit): void
  }
  class SassString {
    constructor(value: string)
    getValue(): string
    setValue(value: string): void
  }
  class SassColor {
    constructor(argb: number)
    constructor(r: number, g: number, b: number, a?: number)
    getR(): number
    getG(): number
    getB(): number
    getA(): number
    setR(value: number): void
    setG(value: number): void
    setB(value: number): void
    setA(value: number): void
  }
  class SassBoolean<T extends boolean = boolean> {
    static TRUE: SassBoolean<true>
    static FALSE: SassBoolean<false>
    constructor(value: T)
    getValue(): T
  }
  class SassList<Length extends number = number> {
    constructor(length: Length, commaSeparator?: boolean)
    getValue(index: number): SassValue
    getLength(): Length
    getSeparator(): boolean
    setValue(index: number, value: SassValue): void
    setSeparator(isComma: boolean): void
  }
  class SassMap<Length extends number = number> {
    constructor(length: Length)
    getLength(): Length
    getValue(index: number): SassValue
    setValue(index: number, value: SassValue): void
    getKey(index: number): SassValue
    setKey(index: number, value: SassValue): void
  }
  class SassNull {
    static NULL: SassNull
    constructor()
  }
  type SassValue = types[keyof types]
  type types = {
    Number: SassNumber,
    String: SassString,
    Color: SassColor,
    Boolean: SassBoolean,
    List: SassList,
    Map: SassMap,
    Null: SassNull
  }
  
  
  
  /**
   * Define a handler for `import` calls in sass
   * @param url the path in import as-is, which [LibSass](https://github.com/sass/libsass) encountered
   * @param prev the previously resolved path
   * @param done a callback function to invoke on async completion, takes an object literal containing 
   * 
   */
  type Importer = (url: string, prev: string, done: (data: ImporterReturn) => void) => ImporterReturn | null | void
  type ImporterReturn = { file: string } | { contents: string } | Error | null
  
  type RenderOptions = Partial<{
    file: PathLike;
    data: string;
    /**
     * Handles when [LibSass](https://github.com/sass/libsass) encounters the @import directive. A custom importer allows extension of the [LibSass](https://github.com/sass/libsass) engine in both a synchronous and asynchronous manner. In both cases, the goal is to either return or call `done()` with an object literal. Depending on the value of the object literal, one of two things will happen.
     * - importers can return error and LibSass will emit that error in response. For instance:
  ```js
  done(new Error('doesn\'t exist!'));
  // or return synchronously
  return new Error('nothing to do here');
  ```
     */
    importer: Importer | Importer[];
    /**
     * **This is an experimental LibSass feature. Use with caution.**
     * 
     * `functions` is an `Object` that holds a collection of custom functions that may be invoked by the sass files being compiled. They may take zero or more input parameters and must return a value either synchronously (`return ...;`) or asynchronously (`done();`). Those parameters will be instances of one of the constructors contained in the `require('node-sass').types` hash. The return value must be of one of these types as well. See the list of available types below:
     * 
     * @example
     * ```js
  sass.renderSync({
    data: '#{headings(2,5)} { color: #08c; }',
    functions: {
      'headings($from: 0, $to: 6)': function(from, to) {
        var i, f = from.getValue(), t = to.getValue(),
            list = new sass.types.List(t - f + 1);
  
        for (i = f; i <= t; i++) {
          list.setValue(i - f, new sass.types.String('h' + i));
        }
  
        return list;
      }
    }
  });
  ```
     */
    functions: { [key: string]: Function }
    /** An array of paths that LibSass can look in to attempt to resolve your `@import` declarations. When using `data`, it is recommended that you use this. */
    includePaths: string[];
    /**
     * `true` values enable [Sass Indented Syntax](https://sass-lang.com/documentation/file.INDENTED_SYNTAX.html) for parsing the data string or file.
     * __Note:__ node-sass/libsass will compile a mixed library of scss and indented syntax (.sass) files with the Default setting (false) as long as .sass and .scss extensions are used in filenames.
     */
    indentedSyntax: boolean
    /** Used to determine whether to use space or tab character for indentation. */
    indentType: "space" | "tab"
    /**
     * Used to determine the number of spaces or tabs to be used for indentation.
     * 
     * Maximum: `10`
     */
    indentWidth: number;
    /** Used to determine whether to use `cr`, `crlf`, `lf` or `lfcr` sequence for line break. */
    linefeed: "cr" | "crlf" | "lf" | "lfcr";
    /**
     * **Special:** When using this, you should also specify `outFile` to avoid unexpected behavior.
     * 
     * `true` values disable the inclusion of source map information in the output file.
     */
    omitSourceMapUrl: boolean;
    /**
     * **Special:** Required when `sourceMap` is a truthy value
  
  Specify the intended location of the output file. Strongly recommended when outputting source maps so that they can properly refer back to their intended files.
  
  **Attention** enabling this option will **not** write the file on disk for you, it's for internal reference purpose only (to generate the map for example).
  
  Example on how to write it on the disk
  
  ```javascript
  sass.render({
      ...
      outFile: yourPathTotheFile,
    }, function(error, result) { // node-style callback from v3.0.0 onwards
      if(!error){
        // No errors during the compilation, write this result on the disk
        fs.writeFile(yourPathTotheFile, result.css, function(err){
          if(!err){
            //file written on disk
          }
        });
      }
    });
  });
  ```
  
     */
    outFile: string;
    /** Determines the output format of the final CSS style. */
    outputStyle: "nested" | "expanded" | "compact" | "compressed";
    /** Used to determine how many digits after the decimal will be allowed. For instance, if you had a decimal number of `1.23456789` and a precision of `5`, the result will be `1.23457` in the final CSS. */
    precision: number;
    /** `true` Enables the line number and file where a selector is defined to be emitted into the compiled CSS as a comment. Useful for debugging, especially when using imports and mixins. */
    sourceComments: boolean;
    /**
     * **Special:** Setting the `sourceMap` option requires also setting the `outFile` option
  
  Enables the outputting of a source map during `render` and `renderSync`. When `sourceMap === true`, the value of `outFile` is used as the target output location for the source map. When `typeof sourceMap === "string"`, the value of `sourceMap` will be used as the writing location for the file.
     */
    sourceMap: boolean;
    /** `true` includes the `contents` in the source map information */
    sourceMapContents: boolean;
    /** `true` embeds the source map as a data URI */
    sourceMapEmbed: boolean;
    /** the value will be emitted as `sourceRoot` in the source map information */
    sourceMapRoot: boolean;
  }>;
  class RenderError extends Error {
    /** The error message. */
    message: string
    /** The line number of error. */
    line: number
    /** The column number of error. */
    column: number
    /** The status code. */
    status: number
    /** The filename of error. In case `file` option was not set (in favour of `data`), this will reflect the value `stdin`. */
    file: string
  }
  type RenderResult = {
    /** The compiled CSS. Write this to a file, or serve it out as needed. */
    css: Uint8Array;
    map: Buffer | undefined;
    stats: {
      entry: string;
      start: number;
      end: number;
      duration: number;
      includedFiles: string[];
    };
  };
  export function render(options: RenderOptions, callback: (err: RenderError | undefined, result: RenderResult) => void): void
  export function renderSync(options: RenderOptions): RenderResult
}
