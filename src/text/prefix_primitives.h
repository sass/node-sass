typedef char *(*prefix_matcher)(char *);

#define DECLARE_MATCHER(name) \
char *prefix_is_ ## name(char *)

#define DEFINE_CHAR_MATCHER(name, prefix) \
char *prefix_is_ ## name(char *src) { \
  return prefix_is_char(src, prefix); \
}

#define DEFINE_CHARS_MATCHER(name, prefix) \
char *prefix_is_ ## name(char *src) { \
  return prefix_is_chars(src, prefix); \
}

#define DEFINE_SINGLE_CTYPE_MATCHER(type) \
char *prefix_is_ ## type(char *src) { \
  return is ## type(*src) ? src+1 : NULL; \
}

#define DEFINE_CTYPE_SEQUENCE_MATCHER(type) \
char *prefix_is_ ## type ## s(char *src) { \
  char *p = src; \
  while (is ## type(*p)) p++; \
  return p == src ? NULL : p; \
}

#define DEFINE_TO_EOL_MATCHER(name, prefix) \
char *prefix_is_ ## name(char *src) { \
  if (!(src = prefix_is_chars(src, prefix))) return NULL; \
  while(*src && *src != '\n') src++; \
  return src; \
}

#define DEFINE_DELIMITED_MATCHER(name, begin, end, escapable) \
char *prefix_is_ ## name(char *src) { \
  return prefix_is_delimited_by(src, begin, end, escapable); \
}

#define DEFINE_ALTERNATIVES_MATCHER(name, ...) \
char *prefix_is_ ## name(char *src) { \
  return prefix_alternatives(src, __VA_ARGS__); \
}

#define DEFINE_SEQUENCE_MATCHER(name, ...) \
char *prefix_is_ ## name(char *src) { \
  return prefix_sequence(src, __VA_ARGS__); \
}

#define DEFINE_OPTIONAL_MATCHER(name, matcher) \
char *prefix_is_ ## name(char *src) { \
  return prefix_optional(src, matcher); \
}

#define DEFINE_FIRST_REST_MATCHER(name, first_matcher, rest_matcher) \
char *prefix_is_ ## name(char *src) { \
  if (src = first_matcher(src)) src = prefix_zero_plus(src, rest_matcher); \
  return src; \
}

char *prefix_is_char(char *src, char pre);
char *prefix_is_chars(char *src, char *pre);
char *prefix_is_one_of(char *src, char *class);
char *prefix_is_some_of(char *src, char *class);
char *prefix_is_delimited_by(char *src, char *beg, char *end, int esc);

char *_prefix_alternatives(char *src, ...);
#define prefix_alternatives(src, ...) _prefix_alternatives(src, __VA_ARGS__, NULL)
char *_prefix_sequence(char *src, ...);
#define prefix_sequence(src, ...) _prefix_sequence(src, __VA_ARGS__, NULL)
char *prefix_optional(char *src, prefix_matcher m);
char *prefix_zero_plus(char *src, prefix_matcher m);
char *prefix_one_plus(char *src, prefix_matcher m);

DECLARE_MATCHER(space);
DECLARE_MATCHER(alpha);
DECLARE_MATCHER(digit);
DECLARE_MATCHER(xdigit);
DECLARE_MATCHER(alnum);
DECLARE_MATCHER(punct);

DECLARE_MATCHER(spaces);
DECLARE_MATCHER(alphas);
DECLARE_MATCHER(digits);
DECLARE_MATCHER(xdigits);
DECLARE_MATCHER(alnums);
DECLARE_MATCHER(puncts);

DECLARE_MATCHER(shell_comment);
DECLARE_MATCHER(c_line_comment);
DECLARE_MATCHER(c_block_comment);
DECLARE_MATCHER(double_quoted_string);
DECLARE_MATCHER(single_quoted_string);
DECLARE_MATCHER(interpolant);

DECLARE_MATCHER(lparen);
DECLARE_MATCHER(rparen);
DECLARE_MATCHER(lbrack);
DECLARE_MATCHER(rbrack);
DECLARE_MATCHER(lbrace);
DECLARE_MATCHER(rbrace);

DECLARE_MATCHER(underscore);
DECLARE_MATCHER(hyphen);
DECLARE_MATCHER(semicolon);
DECLARE_MATCHER(colon);
DECLARE_MATCHER(period);
DECLARE_MATCHER(question);
DECLARE_MATCHER(exclamation);
DECLARE_MATCHER(tilde);
DECLARE_MATCHER(backquote);
DECLARE_MATCHER(quote);
DECLARE_MATCHER(apostrophe);
DECLARE_MATCHER(ampersand);
DECLARE_MATCHER(caret);
DECLARE_MATCHER(pipe);
DECLARE_MATCHER(slash);
DECLARE_MATCHER(backslash);
DECLARE_MATCHER(asterisk);
DECLARE_MATCHER(pound);
DECLARE_MATCHER(hash);
                                  
DECLARE_MATCHER(plus);
DECLARE_MATCHER(minus);
DECLARE_MATCHER(times);
DECLARE_MATCHER(divide);
                                  
DECLARE_MATCHER(percent);
DECLARE_MATCHER(dollar);
                                  
DECLARE_MATCHER(gt);
DECLARE_MATCHER(gte);
DECLARE_MATCHER(lt);
DECLARE_MATCHER(lte);
DECLARE_MATCHER(eq);
DECLARE_MATCHER(assign);
DECLARE_MATCHER(equal);

DECLARE_MATCHER(identifier);
