#include <setjmp.h>

typedef struct {
  char *path;  /* the full directory+filename of the source file */
  char *src;   /* the text of the entire source file */
  char *pos;   /* keeps track of the parser/scanner's current position */
  jmp_buf env; /* the top of the exception-handling stack */
  /* more to come */
} sass_context;

sass_context *make_sass_context_from_file(char *path);