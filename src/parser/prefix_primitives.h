typedef int(*prefix_matcher)(char *);

int prefix_is_exactly(char *, char*);
int prefix_is_one_of(char *, char *);
int prefix_is_some_of(char *, char *);
int prefix_is_delimited_by(char *, char *, char *, int);
int prefix_alternatives(char *, ...);
int prefix_sequence(char *, ...);
int prefix_optional(char *, prefix_matcher);

#define DECLARE_MATCHER(name) \
int prefix_is_ ## name(char *)

#define DEFINE_EXACT_MATCHER(name, prefix) \
int prefix_is_ ## name(char *src) { \
  return prefix_is_exactly(src, prefix); \
}

#define DEFINE_SINGLE_CTYPE_MATCHER(type) \
int prefix_is_ ## type(char *src) { \
  return is ## type(src[0]) ? 1 : 0; \
}

#define DEFINE_CTYPE_SEQUENCE_MATCHER(type) \
int prefix_is_ ## type ## s(char *src) { \
  int p = 0; \
  while (is ## type(src[p])) p++; \
  return p; \
}

#define DEFINE_DELIMITED_MATCHER(name, begin, end, escapable) \
int prefix_is_ ## name(char *src) { \
  return prefix_is_delimited_by(src, begin, end, escapable); \
}

#define DEFINE_TO_EOL_MATCHER(name, prefix) \
int prefix_is_ ## name(char *src) { \
  int p = prefix_is_exactly(src, prefix); \
  while (src[p] && src[p] != '\n') p++; \
  return p; \
}

#define DEFINE_ALTERNATIVES_MATCHER(name, ...) \
int prefix_is_ ## name(char *src) { \
  return prefix_alternatives(src, __VA_ARGS__); \
}

#define DEFINE_SEQUENCE_MATCHER(name, ...) \
int prefix_is_ ## name(char *src) { \
  return prefix_sequence(src, __VA_ARGS__); \
}

#define DEFINE_OPTIONAL_MATCHER(name, matcher) \
int prefix_is_ ## name(char *src) { \
  return prefix_optional(src, matcher); \
}

#define DEFINE_FIRST_REST_MATCHER(name, first_matcher, rest_matcher) \
int prefix_is_ ## name(char *src) { \
  int p = first_matcher(src); \
  int p_sum = p; \
  while (p) { \
    p = rest_matcher(src+p); \
    p_sum += p; \
  } \
  return p_sum; \
}

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
DECLARE_MATCHER(line_comment);
DECLARE_MATCHER(block_comment);
DECLARE_MATCHER(double_quoted_string);
DECLARE_MATCHER(single_quoted_string);
DECLARE_MATCHER(interpolant);
DECLARE_MATCHER(string);
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
