#include <stdlib.h>
#include <stdio.h>
#include "exception.h"

raise(int code) {
  printf("Aborted with error code %d.", code);
  abort();
}