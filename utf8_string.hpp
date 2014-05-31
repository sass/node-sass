#ifndef SASS_UTF8_STRING
#define SASS_UTF8_STRING

#include <string>

namespace Sass {
  namespace UTF_8 {
    // class utf8_string {
    //   string s_;
    // public:
    //   utf8_string(const string &s): s_(s) {}
    //   utf8_string(const char* c): s_(string(c)) {}

    //   char operator[](size_t i);
    //   size_t length();
    //   size_t byte_to_char(size_t i);
    // };

    // function that will count the number of code points (utf-8 characters) from the beginning to the given end
    size_t code_point_count(const string& str, size_t start, size_t end);
    size_t code_point_count(const string& str);

    size_t code_point_offset_to_byte_offset(const string& str, size_t offset);

  }
}

#endif 