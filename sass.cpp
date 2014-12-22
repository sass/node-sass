#include <cstdlib>
#include <cstring>
#include <vector>
#include <sstream>

#include "sass.h"
#include "inspect.hpp"

extern "C" {
  using namespace std;

  // caller must free the returned memory
  char* ADDCALL sass_string_quote (const char *str, const char quotemark) {
    string quoted = Sass::quote(str, quotemark);
    char *cstr = (char*) malloc(quoted.length() + 1);
    std::strcpy(cstr, quoted.c_str());
    return cstr;
  }

  // caller must free the returned memory
  char* ADDCALL sass_string_unquote (const char *str) {
    string unquoted = Sass::unquote(str);
    char *cstr = (char*) malloc(unquoted.length() + 1);
    std::strcpy(cstr, unquoted.c_str());
    return cstr;
  }

  // Get compiled libsass version
  const char* ADDCALL libsass_version(void) {
    return LIBSASS_VERSION;
  }

}
