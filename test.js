var sass = require('./sass');

var scssStr = '#navbar {\
  width: 80%;\
  height: 23px; }\
  #navbar ul {\
    list-style-type: none; }\
  #navbar li {\
    float: left;\
    a {\
      font-weight: bold; }}';

//sass.render(scssStr, function(err, css){
  //console.log(css)
//})

//console.log(sass.renderSync(scssStr));
console.log(sass.renderSync(scssStr));

//console.log(sass.renderSync({
      //file: './test/sample.scss'
//}));


sass.render({
  file: './test/sample.scss'
}, function(output) {
  console.log(output);
});
