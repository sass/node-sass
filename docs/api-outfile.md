# outFile
Type: `String | null`
Default: `null`
**Special:** Required when `sourceMap` is a truthy value

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
