#include <cstdio>
#include <iostream>
#include <sstream>
#include "document.hpp"

using namespace Sass;

void print_slice(const char *s, const char *t) {
  if (t) {
    printf("succeeded with %ld characters:\t", t - s);
    while (s < t) putchar(*s++);
    putchar('\n');
  }
  else {
    printf("failed\n");
  }
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    Document doc(argv[1], 0);
    char* src = doc.source;
    printf("FILE BEGINS ON NEXT LINE\n");
    while (*src) std::putchar(*(src++));
    printf("<EOF>\n");

    doc.try_munching<Prelexer::identifier>();
    print_slice(doc.top.begin, doc.top.end);
    doc.try_munching<Prelexer::exactly<'{'> >();
    print_slice(doc.top.begin, doc.top.end);
    doc.try_munching<Prelexer::block_comment>();
    print_slice(doc.top.begin, doc.top.end);
    doc.try_munching<Prelexer::identifier>();
    print_slice(doc.top.begin, doc.top.end);
    doc.try_munching<Prelexer::dash_match>();
    print_slice(doc.top.begin, doc.top.end);
    
    printf("sizeof char is %ld\n", sizeof(char));
    printf("sizeof int is %ld\n", sizeof(int));
    printf("sizeof document object is %ld\n", sizeof(doc));
    printf("sizeof node object is %ld\n", sizeof(Node));
    printf("sizeof Node vector object is %ld\n", sizeof(std::vector<Node>));
    printf("sizeof pointer to Node vector is %ld\n", sizeof(std::vector<Node>*));

    printf("GOING TO TRY PARSING NOW\n");

    doc.position = doc.source;
    std::cout << doc.position << std::endl;
    if (*(doc.position)) std::cout << "position has content!" << std::endl;
    doc.parse_scss();
    
    int i;
    int j = doc.statements.size();
    std::stringstream output;
    for (i = 0; i < j; ++i) {
      doc.statements[i].emit_expanded_css(output, "");
    }
    
    std::cout << output.str();
    
    
  }
  
  return 0;
}