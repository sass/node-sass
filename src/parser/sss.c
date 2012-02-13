enum sss_type {
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
  sss_type tag;
  size_t (*matcher)(char *);
} sss_component;

typedef struct {
  size_t length;
  sss_component *body;
} sss_pattern;

