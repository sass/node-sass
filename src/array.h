typedef struct {
  size_t length;
  void *contents;
} array;

array make_array(size_t);
void free_array(array);
