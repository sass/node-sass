

typedef struct {
  char *path;  /* the full directory+filename of the source file */
  sass_ruleset *rules[];
} sass_document;

typedef struct {
  sass_selector_group *selector;
  sass_ruleset *rules[];
  sass_declaration *declarations[];
} sass_ruleset;

typedef struct {
  sass_property *property;
  sass_expression *expression;
} sass_declaration;

typedef struct {
  char *value; // Strip out the #{} nonsense
  int indexes[];
  sass_expression *expressions[];
} sass_property;

typedef struct {
  sass_selector *selectors[];
} sass_selector_group;

typedef struct {
  char *value;
} sass_selector;

typedef struct {
  char *value;
} sass_expression;