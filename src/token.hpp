namespace Sass {
  struct Token {
    const char* begin;
    const char* end;
    unsigned int line_number;
    Token();
    Token(const char* _begin,
          const char* _end,
          unsigned int _line_number);
    inline bool is_null() { return begin == 0 || end == 0; }
  };
}