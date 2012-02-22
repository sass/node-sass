#include <stdlib.h>
#include <stdio.h>
#include "exceptions.h"

raise(int code) {
  printf("Aborted with error code %d.", code);
  abort();
}