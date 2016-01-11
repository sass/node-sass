// must be the first include in all compile units
#ifndef SASS_SASS_H
#define SASS_SASS_H

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

#endif
