enum Sass_Tag {
  SASS_BOOLEAN,
  SASS_NUMBER,
  SASS_PERCENTAGE,
  SASS_DIMENSION,
  SASS_COLOR,
  SASS_STRING,
  SASS_LIST
};

enum Sass_Separator {
  SASS_COMMA,
  SASS_SPACE
};

union Sass_Value;

struct Sass_Unknown {
  enum Sass_Tag tag;
};

struct Sass_Boolean {
  enum Sass_Tag tag;
  int           value;
};

struct Sass_Number {
  enum Sass_Tag tag;
  double        value;
};

struct Sass_Percentage {
  enum Sass_Tag tag;
  double        value;
};

struct Sass_Dimension {
  enum Sass_Tag tag;
  double        value;
  char*         unit;
};

struct Sass_Color {
  enum Sass_Tag tag;
  double        r;
  double        g;
  double        b;
  double        a;
};

struct Sass_String {
  enum Sass_Tag tag;
  char*         value;
};

struct Sass_List {
  enum Sass_Tag       tag;
  enum Sass_Separator separator;
  size_t              length;
  union Sass_Value*   values;
};

union Sass_Value {
  struct Sass_Unknown    whatever;
  struct Sass_Boolean    boolean;
  struct Sass_Number     number;
  struct Sass_Percentage percentage;
  struct Sass_Dimension  dimension;
  struct Sass_Color      color;
  struct Sass_String     string;
  struct Sass_List       list;
};

void free_sass_value(union Sass_Value);