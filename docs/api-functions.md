# functions (>= v3.0.0) - _experimental_

**This is an experimental LibSass feature. Use with caution.**

`functions` is an `Object` that holds a collection of custom functions that may be invoked by the sass files being compiled. They may take zero or more input parameters and must return a value either synchronously (`return ...;`) or asynchronously (`done();`). Those parameters will be instances of one of the constructors contained in the `require('node-sass').types` hash. The return value must be of one of these types as well. See the list of available types below:

## types.Number(value [, unit = ""])
* `getValue()`/ `setValue(value)` : gets / sets the numerical portion of the number
* `getUnit()` / `setUnit(unit)` : gets / sets the unit portion of the number

## types.String(value)
* `getValue()` / `setValue(value)` : gets / sets the enclosed string

## types.Color(r, g, b [, a = 1.0]) or types.Color(argb)
* `getR()` / `setR(value)` : red component (integer from `0` to `255`)
* `getG()` / `setG(value)` : green component (integer from `0` to `255`)
* `getB()` / `setB(value)` : blue component (integer from `0` to `255`)
* `getA()` / `setA(value)` : alpha component (number from `0` to `1.0`)

Example:

```javascript
var Color = require('node-sass').types.Color,
  c1 = new Color(255, 0, 0),
  c2 = new Color(0xff0088cc);
```

## types.Boolean(value)
* `getValue()` : gets the enclosed boolean
* `types.Boolean.TRUE` : Singleton instance of `types.Boolean` that holds "true"
* `types.Boolean.FALSE` : Singleton instance of `types.Boolean` that holds "false"

## types.List(length [, commaSeparator = true])
* `getValue(index)` / `setValue(index, value)` : `value` must itself be an instance of one of the constructors in `sass.types`.
* `getSeparator()` / `setSeparator(isComma)` : whether to use commas as a separator
* `getLength()`

## types.Map(length)
* `getKey(index)` / `setKey(index, value)`
* `getValue(index)` / `setValue(index, value)`
* `getLength()`

## types.Null()
* `types.Null.NULL` : Singleton instance of `types.Null`.

## Example

```javascript
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
