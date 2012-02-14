enum prefix_pattern_type {
  ATOM,
  SEQUENCE,
  CHOICE,
  OPTION,
  AT_LEAST_ZERO,
  AT_LEAST_ONE,
  AT_LEAST_N,
  AT_MOST_N,
  EXACTLY_N,
  END
};
  

typedef struct {
  prefix_pattern_type tag;
  int (*matcher)(char *);
} prefix_pattern_component;

typedef struct {
  int length;
  prefix_pattern_component *body;
} prefix_pattern;

