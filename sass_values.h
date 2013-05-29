enum Sass_Tag { NUMBER, PERCENTAGE, DIMENSION, COLOR, STRING, LIST };

enum Sass_Separator { COMMA, SPACE };

union Sass_Value;

struct Sass_Number {
  enum Sass_Tag tag;
  double value;
};

struct Sass_Percentage {
  enum Sass_Tag tag;
  double value;
};

struct Sass_Dimension {
  enum Sass_Tag tag;
  double value;
  char* unit;
};

struct Sass_Color {
  enum Sass_Tag tag;
  double r;
  double g;
  double b;
  double a;
};

struct Sass_String {
  enum Sass_Tag tag;
  char* contents;
};

struct Sass_List {
  enum Sass_Tag tag;
  enum Sass_Separator separator;
  unsigned int length;
  union Sass_Value* values;
};

union Sass_Value {
  struct Sass_Number    number;
  struct Sass_Dimension dimension;
  struct Sass_Color     color;
  struct Sass_String    string;
  struct Sass_List      list;
};