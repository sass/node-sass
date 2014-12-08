#ifndef SASS
#define SASS

#include <stddef.h>
#include <stdbool.h>
#include "sass_values.h"
#include "sass_functions.h"

#ifndef LIBSASS_VERSION
#define LIBSASS_VERSION "[NA]"
#endif

#ifdef __cplusplus
extern "C" {
#endif


// Different render styles
enum Sass_Output_Style {
  SASS_STYLE_NESTED,
  SASS_STYLE_EXPANDED,
  SASS_STYLE_COMPACT,
  SASS_STYLE_COMPRESSED
};

// Some convenient string helper function
char* sass_string_quote (const char *str, const char quotemark);
char* sass_string_unquote (const char *str);

// Get compiled libsass version
const char* libsass_version(void);

#ifdef __cplusplus
}
#endif

#endif
