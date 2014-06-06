#ifndef SASS_UTF8_STRING
#define SASS_UTF8_STRING

#include <string>
#include <cstdlib>
#include <cmath>

namespace Sass {
  namespace UTF_8 {
    using std::string;
    // class utf8_string {
    //   string s_;
    // public:
    //   utf8_string(const string &s): s_(s) {}
    //   utf8_string(const char* c): s_(string(c)) {}

    //   char operator[](size_t i);
    //   size_t length();
    //   size_t byte_to_char(size_t i);
    // };

    // function that will count the number of code points (utf-8 characters) from the given beginning to the given end
    size_t code_point_count(const string& str, size_t start, size_t end) {
      size_t len = 0;
      size_t i = start;

      while (i < end) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c < 128) {
          // it's a single-byte character
          ++len;
          ++i;
        }
        // it's a multi byte sequence and presumably it's a leading byte
        else {
          ++i; // go to the next byte
          // see if it's still part of the sequence
          while ((i < end) && ((static_cast<unsigned char>(str[i]) & 0xC0) == 0x80)) {
            ++i;
          }
          // when it's not [aka a new leading byte], increment and move on
          ++len;
        }
      }
      return len;
    }

    size_t code_point_count(const string& str) {
      return code_point_count(str, 0, str.length());
    }

    // function that will return the byte offset of a code point in a
    size_t code_point_offset_to_byte_offset(const string& str, size_t offset) {
      size_t i = 0;
      size_t len = 0;

      while (len < offset) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c < 128) {
          // it's a single-byte character
          ++len;
          ++i;
        }
        // it's a multi byte sequence and presumably it's a leading byte
        else {
          ++i; // go to the next byte
          // see if it's still part of the sequence
          while ((i < str.length()) && ((static_cast<unsigned char>(str[i]) & 0xC0) == 0x80)) {
            ++i;
          }
          // when it's not [aka a new leading byte], increment and move on
          ++len;
        }
      }
      return i;
    }

    // function that returns number of bytes in a character in a string
    size_t length_of_code_point_at(const string& str, size_t pos) {
      unsigned char c = static_cast<unsigned char>(str[pos]);
      size_t i = 0;
      if(c < 128) {
        return 1;
      } else {
        ++i; // go to the next byte
        ++pos;
        // see if it's still part of the sequence
        while ((i < str.length()) && ((static_cast<unsigned char>(str[pos]) & 0xC0) == 0x80)) {
          ++i;
          ++pos;
        }
      }
      return i;
    }

    // function that will return a normalized index, given a crazy one
    size_t normalize_index(int index, size_t len) {
      int signed_len = len;
      // assuming the index is 1-based
      // we are returning a 0-based index
      if (index > 0 && index <= signed_len) {
        // positive and within string length
        return index-1;
      } 
      else if (index > signed_len) {
        // positive and past string length
        return len;
      }
      else if (index == 0) {
        return 0;
      }
      else if (std::abs(index) <= signed_len) {
        // negative and within string length
        return index + signed_len;
      }
      else {
        // negative and past string length
        return 0;
      }
    }

  }
}

#endif
