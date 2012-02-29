#include <stdlib.h>
#include "parser.h"
#include "prefix.h"

DEFINE_SASS_ARRAY_OF(sass_node_ptr);

sass_token sass_make_token(int lexeme, char *beg, char *end) {
  sass_token t;
  t.lexeme = lexeme;
  t.beg = beg;
  t.end = end;
  return t;
}

int sass_parse(sass_context *ctx) {
  sass_document *doc = (sass_document*)malloc(sizeof(sass_document));
  doc->src = ctx->src;
  return 0;
}

sass_sass_node_ptr_array sass_parse(sass_context *ctx) {
  /* not sure how big of an array to allocate by default */
  sass_sass_node_ptr_array statements = sass_make_sass_node_ptr_array(16);
  while (ctx->*p) sass_sass_node_ptr_array_push(statements, sass_parse_statement(ctx));
  return statements;
}

static sass_node_ptr sass_parse_statement(sass_context *ctx) {
  /* currently only handles rulesets; will expand to handle atkeywords */
  return sass_parse_ruleset(ctx);
}

static sass_node_ptr sass_parse_ruleset(sass_context *ctx) {
  sass_node_ptr selector = sass_parse_selector(ctx);
  sass_node_ptr declarations = sass_parse_declarations(ctx);
  return sass_make_node(RULESET, selector, declarations);
}

static sass_node_ptr sass_parse_selector(sass_context *ctx) {
  /* will eventually need to be much more complicated */
  char *lbrace = sass_prefix_find_first(ctx->pos, sass_prefix_is_lbrace);
  sass_node_ptr node = sass_make_node(SELECTOR);
  node->token = sass_make_token(SELECTOR, pos, lbrace);
  ctx->pos = lbrace;
  return node;
}