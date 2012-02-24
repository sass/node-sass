
#ifndef CONTEXT_HEADER
#include "context.h"
#endif
#include "array.h"

enum {
  IDENT,
  ATKEYWORD,
  STRING,
  IDNAME,
  CLASSNAME,
  NUMBER,
  PERCENTAGE,
  DIMENSION,
  URI,
  SPACES,
  COMMENT,
  FUNCTION,
  EXACTMATCH,
  INCLUDES,
  DASHMATCH,
  PREFIXMATCH,
  SUFFIXMATCH,
  SUBSTRINGMATCH
};
  
typedef struct {
  int lexeme;
  char *beg;
  char *end;
} Token;

#define sass_peek_token(src, lexeme) (sass_prefix_is_ ## lexeme(src))

int sass_parse(sass_context *ctx);