exports.input = '#navbar {\
  width: 80%;\
  height: 23px; }\
  #navbar ul {\
    list-style-type: none; }\
  #navbar li {\
    float: left;\
    a {\
      font-weight: bold; }}\
  @mixin keyAnimation($name, $attr, $value) {\
    @-webkit-keyframes #{$name} {\
      0%   { #{$attr}: $value; }\
    }\
  }';

// Note that the bad
exports.badInput = '#navbar \n\
  width: 80%';

exports.expectedRender = '#navbar {\n\
  width: 80%;\n\
  height: 23px; }\n\
\n\
#navbar ul {\n\
  list-style-type: none; }\n\
\n\
#navbar li {\n\
  float: left; }\n\
  #navbar li a {\n\
    font-weight: bold; }\n';