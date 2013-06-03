#include "sass.h"

namespace Sass {

  union Sass_Value new_sass_c_boolean    (int val);
  union Sass_Value new_sass_c_number     (double val);
  union Sass_Value new_sass_c_percentage (double val);
  union Sass_Value new_sass_c_dimension  (double val, const char* unit);
  union Sass_Value new_sass_c_color      (double r, double g, double b, double a);
  union Sass_Value new_sass_c_string     (const char* val);
  union Sass_Value new_sass_c_list       (size_t len, enum Sass_Separator sep);
  union Sass_Value new_sass_c_error      (const char* msg);

  void             free_sass_value       (union Sass_Value);

}