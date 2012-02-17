typedef char *(*prefix_matcher)(char *);

#define DECLARE(name) \
char *prefix_is_ ## name(char *)

#define CHAR_MATCHER(name, prefix) \
char *prefix_is_ ## name(char *src) { \
  return prefix_is_char(src, prefix); \
}

#define CHARS_MATCHER(name, prefix) \
char *prefix_is_ ## name(char *src) { \
  return prefix_is_chars(src, prefix); \
}

#define CLASS_CHAR_MATCHER(name, class) \
char *prefix_is ## name(char *src) { \
  return prefix_is_one_of(src, class); \
}

#define CLASS_CHARS_MATCHER(name, class) \
char *prefix)us ## name(char *src) { \
  return prefix_is_some_of(src, class); \
}

#define SINGLE_CTYPE_MATCHER(type) \
char *prefix_is_ ## type(char *src) { \
  return is ## type(*src) ? src+1 : NULL; \
}

#define CTYPE_SEQUENCE_MATCHER(type) \
char *prefix_is_ ## type ## s(char *src) { \
  char *p = src; \
  while (is ## type(*p)) p++; \
  return p == src ? NULL : p; \
}

#define TO_EOL_MATCHER(name, prefix) \
char *prefix_is_ ## name(char *src) { \
  if (!(src = prefix_is_chars(src, prefix))) return NULL; \
  while(*src && *src != '\n') src++; \
  return src; \
}

#define DELIMITED_MATCHER(name, begin, end, escapable) \
char *prefix_is_ ## name(char *src) { \
  return prefix_is_delimited_by(src, begin, end, escapable); \
}

#define ALTERNATIVES_MATCHER(name, ...) \
char *prefix_is_ ## name(char *src) { \
  return prefix_alternatives(src, __VA_ARGS__); \
}

#define SEQUENCE_MATCHER(name, ...) \
char *prefix_is_ ## name(char *src) { \
  return prefix_sequence(src, __VA_ARGS__); \
}

#define OPTIONAL_MATCHER(name, matcher) \
char *prefix_is_ ## name(char *src) { \
  return prefix_optional(src, matcher); \
}

#define FIRST_REST_MATCHER(name, first_matcher, rest_matcher) \
char *prefix_is_ ## name(char *src) { \
  if (src = first_matcher(src)) src = prefix_zero_plus(src, rest_matcher); \
  return src; \
}

char *prefix_is_char(char *src, char pre);
char *prefix_is_chars(char *src, char *pre);
char *prefix_is_one_of(char *src, char *class);
char *prefix_is_some_of(char *src, char *class);
char *prefix_is_delimited_by(char *src, char *beg, char *end, int esc);

char *prefix_epsilon(char *src);
char *_prefix_alternatives(char *src, ...);
#define prefix_alternatives(src, ...) _prefix_alternatives(src, __VA_ARGS__, NULL)
char *_prefix_sequence(char *src, ...);
#define prefix_sequence(src, ...) _prefix_sequence(src, __VA_ARGS__, NULL)
char *prefix_optional(char *src, prefix_matcher m);
char *prefix_zero_plus(char *src, prefix_matcher m);
char *prefix_one_plus(char *src, prefix_matcher m);

DECLARE(space);
DECLARE(alpha);
DECLARE(digit);
DECLARE(xdigit);
DECLARE(alnum);
DECLARE(punct);

DECLARE(spaces);
DECLARE(alphas);
DECLARE(digits);
DECLARE(xdigits);
DECLARE(alnums);
DECLARE(puncts);

DECLARE(shell_comment);
DECLARE(c_line_comment);
DECLARE(c_block_comment);
DECLARE(double_quoted_string);
DECLARE(single_quoted_string);
DECLARE(interpolant);

DECLARE(lparen);
DECLARE(rparen);
DECLARE(lbrack);
DECLARE(rbrack);
DECLARE(lbrace);
DECLARE(rbrace);

DECLARE(underscore);
DECLARE(hyphen);
DECLARE(semicolon);
DECLARE(colon);
DECLARE(period);
DECLARE(question);
DECLARE(exclamation);
DECLARE(tilde);
DECLARE(backquote);
DECLARE(quote);
DECLARE(apostrophe);
DECLARE(ampersand);
DECLARE(caret);
DECLARE(pipe);
DECLARE(slash);
DECLARE(backslash);
DECLARE(asterisk);
DECLARE(pound);
DECLARE(hash);
                                  
DECLARE(plus);
DECLARE(minus);
DECLARE(times);
DECLARE(divide);
                                  
DECLARE(percent);
DECLARE(dollar);
                                  
DECLARE(gt);
DECLARE(gte);
DECLARE(lt);
DECLARE(lte);
DECLARE(eq);
DECLARE(assign);
DECLARE(equal);

DECLARE(identifier);
DECLARE(integer);