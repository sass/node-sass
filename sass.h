#ifndef SASS
#define SASS

#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32

  /* You should define ADD_EXPORTS *only* when building the DLL. */
  #ifdef ADD_EXPORTS
    #define ADDAPI __declspec(dllexport)
	#define ADDCALL __cdecl
  #else
    #define ADDAPI
	#define ADDCALL
  #endif

#else /* _WIN32 not defined. */

  /* Define with no value on non-Windows OSes. */
  #define ADDAPI
  #define ADDCALL

#endif

#ifndef LIBSASS_VERSION
#define LIBSASS_VERSION "[NA]"
#endif

// include API headers
#include "sass_values.h"
#include "sass_functions.h"

/* Make sure functions are exported with C linkage under C++ compilers. */
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
ADDAPI char* ADDCALL sass_string_quote (const char *str, const char quotemark);
ADDAPI char* ADDCALL sass_string_unquote (const char *str);

// Get compiled libsass version
ADDAPI const char* ADDCALL libsass_version(void);


#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif
