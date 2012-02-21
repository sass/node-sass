typedef char *(*sass_prefix_matcher)(char *);

#define DECLARE(name) \
char *sass_prefix_is_ ## name(char *)

#define DECLARE_ALIAS(name) \
sass_prefix_matcher sass_prefix_is_ ## name

#define ALIAS_MATCHERS(orig, new) \
sass_prefix_matcher sass_prefix_is_ ## new = &orig

#define CHAR_MATCHER(name, prefix) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_is_char(src, prefix); \
}

#define CHARS_MATCHER(name, prefix) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_is_chars(src, prefix); \
}

#define CHAR_CLASS_MATCHER(name, class) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_is_one_of(src, class); \
}

#define CHARS_CLASS_MATCHER(name, class) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_is_some_of(src, class); \
}

#define SINGLE_CTYPE_MATCHER(type) \
char *sass_prefix_is_ ## type(char *src) { \
  return is ## type(*src) ? src+1 : NULL; \
}

#define CTYPE_SEQUENCE_MATCHER(type) \
char *sass_prefix_is_ ## type ## s(char *src) { \
  char *p = src; \
  while (is ## type(*p)) p++; \
  return p == src ? NULL : p; \
}

#define TO_EOL_MATCHER(name, prefix) \
char *sass_prefix_is_ ## name(char *src) { \
  if (!(src = sass_prefix_is_chars(src, prefix))) return NULL; \
  while(*src && *src != '\n') src++; \
  return src; \
}

#define DELIMITED_MATCHER(name, begin, end, escapable) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_is_delimited_by(src, begin, end, escapable); \
}

#define ALTERNATIVES_MATCHER(name, ...) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_alternatives(src, __VA_ARGS__); \
}

#define SEQUENCE_MATCHER(name, ...) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_sequence(src, __VA_ARGS__); \
}

#define OPTIONAL_MATCHER(name, matcher) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_optional(src, matcher); \
}

#define ZERO_PLUS_MATCHER(name, matcher) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_zero_plus(src, matcher); \
}

#define ONE_PLUS_MATCHER(name, matcher) \
char *sass_prefix_is_ ## name(char *src) { \
  return sass_prefix_one_plus(src, matcher); \
}

#define FIRST_REST_MATCHER(name, first_matcher, rest_matcher) \
char *sass_prefix_is_ ## name(char *src) { \
  if (src = first_matcher(src)) src = sass_prefix_zero_plus(src, rest_matcher); \
  return src; \
}

char *sass_prefix_is_char(char *src, char pre);
char *sass_prefix_is_chars(char *src, char *pre);
char *sass_prefix_is_one_of(char *src, char *class);
char *sass_prefix_is_some_of(char *src, char *class);
char *sass_prefix_is_delimited_by(char *src, char *beg, char *end, int esc);

char *sass_prefix_epsilon(char *src);
char *sass_prefix_not(char *src, sass_prefix_matcher m);
char *_sass_prefix_alternatives(char *src, ...);
#define sass_prefix_alternatives(src, ...) _sass_prefix_alternatives(src, __VA_ARGS__, NULL)
char *_sass_prefix_sequence(char *src, ...);
#define sass_prefix_sequence(src, ...) _sass_prefix_sequence(src, __VA_ARGS__, NULL)
char *sass_prefix_optional(char *src, sass_prefix_matcher m);
char *sass_prefix_zero_plus(char *src, sass_prefix_matcher m);
char *sass_prefix_one_plus(char *src, sass_prefix_matcher m);
char *sass_prefix_find_first(char *src, sass_prefix_matcher m);

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
DECLARE(line_comment);
DECLARE(block_comment);
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
DECLARE(dot);
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
DECLARE(star);
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

DECLARE(exactmatch);
DECLARE(classmatch);
DECLARE(dashmatch);
DECLARE(prefixmatch);
DECLARE(suffixmatch);
DECLARE(substringmatch);

DECLARE(name);
DECLARE(identifier);

DECLARE(variable);

DECLARE(number);
DECLARE(string);

DECLARE(idname);
DECLARE(classname);
DECLARE(functional);

DECLARE(adjacent_to);
DECLARE(parent_of);
DECLARE(precedes);
DECLARE_ALIAS(ancestor_of);

