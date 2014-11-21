#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <cstdlib>
#include <cstring>
#include "sass_values.h"

extern "C" {
  using namespace std;

  struct Sass_Unknown {
    enum Sass_Tag tag;
  };

  struct Sass_Boolean {
    enum Sass_Tag tag;
    bool          value;
  };

  struct Sass_Number {
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
    // null terminated "array"
    union Sass_Value**  values;
  };

  struct Sass_Map {
    enum Sass_Tag        tag;
    size_t               length;
    struct Sass_MapPair* pairs;
  };

  struct Sass_Null {
    enum Sass_Tag tag;
  };

  struct Sass_Error {
    enum Sass_Tag tag;
    char*         message;
  };

  union Sass_Value {
    struct Sass_Unknown unknown;
    struct Sass_Boolean boolean;
    struct Sass_Number  number;
    struct Sass_Color   color;
    struct Sass_String  string;
    struct Sass_List    list;
    struct Sass_Map     map;
    struct Sass_Null    null;
    struct Sass_Error   error;
  };

  struct Sass_MapPair {
    union Sass_Value* key;
    union Sass_Value* value;
  };

  // Return the sass tag for a generic sass value
  enum Sass_Tag sass_value_get_tag(union Sass_Value* v) { return v->unknown.tag; }

  // Check value for specified type
  bool sass_value_is_null(union Sass_Value* v) { return v->unknown.tag == SASS_NULL; }
  bool sass_value_is_map(union Sass_Value* v) { return v->unknown.tag == SASS_MAP; }
  bool sass_value_is_list(union Sass_Value* v) { return v->unknown.tag == SASS_LIST; }
  bool sass_value_is_number(union Sass_Value* v) { return v->unknown.tag == SASS_NUMBER; }
  bool sass_value_is_string(union Sass_Value* v) { return v->unknown.tag == SASS_STRING; }
  bool sass_value_is_boolean(union Sass_Value* v) { return v->unknown.tag == SASS_BOOLEAN; }
  bool sass_value_is_error(union Sass_Value* v) { return v->unknown.tag == SASS_ERROR; }
  bool sass_value_is_color(union Sass_Value* v) { return v->unknown.tag == SASS_COLOR; }

  // Getters and setters for Sass_Number
  double sass_number_get_value(union Sass_Value* v) { return v->number.value; }
  void sass_number_set_value(union Sass_Value* v, double value) { v->number.value = value; }
  const char* sass_number_get_unit(union Sass_Value* v) { return v->number.unit; }
  void sass_number_set_unit(union Sass_Value* v, char* unit) { v->number.unit = unit; }

  // Getters and setters for Sass_String
  const char* sass_string_get_value(union Sass_Value* v) { return v->string.value; }
  void sass_string_set_value(union Sass_Value* v, char* value) { v->string.value = value; }

  // Getters and setters for Sass_Boolean
  bool sass_boolean_get_value(union Sass_Value* v) { return v->boolean.value; }
  void sass_boolean_set_value(union Sass_Value* v, bool value) { v->boolean.value = value; }

  // Getters and setters for Sass_Color
  double sass_color_get_r(union Sass_Value* v) { return v->color.r; }
  void sass_color_set_r(union Sass_Value* v, double r) { v->color.r = r; }
  double sass_color_get_g(union Sass_Value* v) { return v->color.g; }
  void sass_color_set_g(union Sass_Value* v, double g) { v->color.g = g; }
  double sass_color_get_b(union Sass_Value* v) { return v->color.b; }
  void sass_color_set_b(union Sass_Value* v, double b) { v->color.b = b; }
  double sass_color_get_a(union Sass_Value* v) { return v->color.a; }
  void sass_color_set_a(union Sass_Value* v, double a) { v->color.a = a; }

  // Getters and setters for Sass_List
  size_t sass_list_get_length(union Sass_Value* v) { return v->list.length; }
  enum Sass_Separator sass_list_get_separator(union Sass_Value* v) { return v->list.separator; }
  void sass_list_set_separator(union Sass_Value* v, enum Sass_Separator separator) { v->list.separator = separator; }
  // Getters and setters for Sass_List values
  union Sass_Value* sass_list_get_value(union Sass_Value* v, size_t i) { return v->list.values[i]; }
  void sass_list_set_value(union Sass_Value* v, size_t i, union Sass_Value* value) { v->list.values[i] = value; }

  // Getters and setters for Sass_Map
  size_t sass_map_get_length(union Sass_Value* v) { return v->map.length; }
  // Getters and setters for Sass_List keys and values
  union Sass_Value* sass_map_get_key(union Sass_Value* v, size_t i) { return v->map.pairs[i].key; }
  union Sass_Value* sass_map_get_value(union Sass_Value* v, size_t i) { return v->map.pairs[i].value; }
  void sass_map_set_key(union Sass_Value* v, size_t i, union Sass_Value* key) { v->map.pairs[i].key = key; }
  void sass_map_set_value(union Sass_Value* v, size_t i, union Sass_Value* val) { v->map.pairs[i].value = val; }

  // Getters and setters for Sass_Error
  char* sass_error_get_message(union Sass_Value* v) { return v->error.message; };
  void sass_error_set_message(union Sass_Value* v, char* msg) { v->error.message = msg; };

  // Creator functions for all value types

  union Sass_Value* sass_make_boolean(bool val)
  {
    Sass_Value* v = (Sass_Value*) calloc(1, sizeof(Sass_Value));
    if (v == 0) return 0;
    v->boolean.tag = SASS_BOOLEAN;
    v->boolean.value = val;
    return v;
  }

  union Sass_Value* sass_make_number(double val, const char* unit)
  {
    Sass_Value* v = (Sass_Value*) calloc(1, sizeof(Sass_Value));
    if (v == 0) return 0;
    v->number.tag = SASS_NUMBER;
    v->number.value = val;
    v->number.unit = strdup(unit);
    if (v->number.unit == 0) { free(v); return 0; }
    return v;
  }

  union Sass_Value* sass_make_color(double r, double g, double b, double a)
  {
    Sass_Value* v = (Sass_Value*) calloc(1, sizeof(Sass_Value));
    if (v == 0) return 0;
    v->color.tag = SASS_COLOR;
    v->color.r = r;
    v->color.g = g;
    v->color.b = b;
    v->color.a = a;
    return v;
  }

  union Sass_Value* sass_make_string(const char* val)
  {
    Sass_Value* v = (Sass_Value*) calloc(1, sizeof(Sass_Value));
    if (v == 0) return 0;
    v->string.tag = SASS_STRING;
    v->string.value = strdup(val);
    if (v->string.value == 0) { free(v); return 0; }
    return v;
  }

  union Sass_Value* sass_make_list(size_t len, enum Sass_Separator sep)
  {
    Sass_Value* v = (Sass_Value*) calloc(1, sizeof(Sass_Value));
    if (v == 0) return 0;
    v->list.tag = SASS_LIST;
    v->list.length = len;
    v->list.separator = sep;
    v->list.values = (union Sass_Value**) calloc(len, sizeof(union Sass_Value));
    if (v->list.values == 0) { free(v); return 0; }
    return v;
  }

  union Sass_Value* sass_make_map(size_t len)
  {
    Sass_Value* v = (Sass_Value*) calloc(1, sizeof(Sass_Value));
    if (v == 0) return 0;
    v->map.tag = SASS_MAP;
    v->map.length = len;
    v->map.pairs = (struct Sass_MapPair*) calloc(len, sizeof(struct Sass_MapPair));
    if (v->map.pairs == 0) { free(v); return 0; }
    return v;
  }

  union Sass_Value* sass_make_null(void)
  {
    Sass_Value* v = (Sass_Value*) calloc(1, sizeof(Sass_Value));
    if (v == 0) return 0;
    v->null.tag = SASS_NULL;
    return v;
  }

  union Sass_Value* sass_make_error(const char* msg)
  {
    Sass_Value* v = (Sass_Value*) calloc(1, sizeof(Sass_Value));
    if (v == 0) return 0;
    v->error.tag = SASS_ERROR;
    v->error.message = strdup(msg);
    if (v->error.message == 0) { free(v); return 0; }
    return v;
  }

  // will free all associated sass values
  void sass_delete_value(union Sass_Value* val) {

    size_t i;
    if (val == 0) return;
    switch(val->unknown.tag) {
        case SASS_NULL: {
        }   break;
        case SASS_BOOLEAN: {
        }   break;
        case SASS_NUMBER: {
                free(val->number.unit);
        }   break;
        case SASS_COLOR: {
        }   break;
        case SASS_STRING: {
                free(val->string.value);
        }   break;
        case SASS_LIST: {
                for (i=0; i<val->list.length; i++) {
                    sass_delete_value(val->list.values[i]);
                }
                free(val->list.values);
        }   break;
        case SASS_MAP: {
                for (i=0; i<val->map.length; i++) {
                    sass_delete_value(val->map.pairs[i].key);
                    sass_delete_value(val->map.pairs[i].value);
                }
                free(val->map.pairs);
        }   break;
        case SASS_ERROR: {
                free(val->error.message);
        }   break;
    }

    free(val);

  }

}
