// must be the first include in all compile units
#ifndef SASS_SASS_H
#define SASS_SASS_H

// include C-API header
#include "sass/base.h"

// undefine extensions macro to tell sys includes
// that we do not want any macros to be exported
// mainly fixes an issue on SmartOS (SEC macro)
#undef __EXTENSIONS__

#ifdef _MSC_VER
#pragma warning(disable : 4005)
#endif

// aplies to MSVC and MinGW
#ifdef _WIN32
// we do not want the ERROR macro
# define NOGDI
// we do not want the min/max macro
# define NOMINMAX
// we do not want the IN/OUT macro
# define _NO_W32_PSEUDO_MODIFIERS
#endif


// should we be case insensitive
// when dealing with files or paths
#ifndef FS_CASE_SENSITIVE
# ifdef _WIN32
#  define FS_CASE_SENSITIVE 0
# else
#  define FS_CASE_SENSITIVE 1
# endif
#endif

// path separation char
#ifndef PATH_SEP
# ifdef _WIN32
#  define PATH_SEP ';'
# else
#  define PATH_SEP ':'
# endif
#endif


// some syntax sugar
namespace Sass {

  // create some C++ aliases for the most used options
  const static Sass_Output_Style NESTED = SASS_STYLE_NESTED;
  const static Sass_Output_Style COMPACT = SASS_STYLE_COMPACT;
  const static Sass_Output_Style EXPANDED = SASS_STYLE_EXPANDED;
  const static Sass_Output_Style COMPRESSED = SASS_STYLE_COMPRESSED;

};

#endif
