#define DECLARE_SASS_ARRAY_OF(type) \
typedef struct { \
  size_t length; \
  size_t capacity; \
  type *contents; \
} sass_ ## type ## _array; \
sass_ ## type ## _array sass_make_ ## type ## _array(size_t cap); \
void sass_ ## type ## _array_push(sass_ ## type ## _array *a, type elem); \
void sass_ ## type ## _array_cat(sass_ ## type ## _array *a, sass_ ## type ## _array *b); \
void sass_free_ ## type ## _array(sass_ ## type ## _array *a)

#define DEFINE_SASS_ARRAY_OF(type) \
sass_ ## type ## _array sass_make_ ## type ## _array(size_t cap) { \
  sass_ ## type ## _array a; \
  a.contents = (type *) malloc(cap * sizeof(type)); \
  if (!a.contents) { \
    printf("ERROR: unable to allocate array of %s\n", #type); \
    abort(); \
  } \
  a.capacity = cap; \
  a.length = 0; \
  return a; \
} \
void sass_ ## type ## _array_push(sass_ ## type ## _array *a, type elem) { \
  size_t len = a->length, cap = a->capacity; \
  if (len < cap) { \
    a->contents[len] = elem; \
    a->length++; \
  } \
  else { \
    type *new = (type *) realloc(a->contents, len * 2); \
    if (!new) { \
      printf("ERROR: unable to resize array of %s\n", #type); \
      abort(); \
    } \
    else { \
      new[len] = elem; \
      a->contents = new; \
      a->length++; \
      a->capacity = len * 2; \
    } \
  } \
} \
void sass_ ## type ## _array_cat(sass_ ## type ## _array *a, sass_ ## type ## _array *b) { \
  size_t b_len = b->length; \
  type *b_cont = b->contents; \
  int i; \
  for (i = 0; i < b_len; i++) sass_ ## type ## _array_push(a, b_cont[i]); \
}
