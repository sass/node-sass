#include <cstdio>
#include "document.hpp"

using namespace Sass;

int main(int argc, char* argv[]) {
  Document doc(argv[1], 0);
  char* src = doc.source;
  printf("FILE BEGINS ON NEXT LINE\n");
  while (*src) std::putchar(*(src++));
  printf("<EOF>\n");
  return 0;
}