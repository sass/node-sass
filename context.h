#include <setjmp.h>
#include <stdlib.h>
#include "tree.h"

#define CONTEXT_HEADER

/*
Probably gonna' need to stratify this a bit more. A context object will store
the symbol table and any other necessary project-wide info. Each sass_document
struct will store src strings, line counts, etc.
*/

typedef struct {
  char *path;              /* the full directory+filename of the source file */
  char *src;               /* the text of the entire source file */
  char *result;            /* the final result, after all compiling */
  char *pos;               /* keeps track of the parser's current position */
  size_t line;             /* the number of the line currently being parsed */
  jmp_buf env;             /* the top of the exception-handling stack */
  sass_document *doc;      /* the primary AST tree */
  sass_document **imports; /* all imported files */
  /* more to come */
} sass_context;

sass_context *sass_make_context_from_file(char *path);
sass_context *sass_make_context_from_string(char *src);
