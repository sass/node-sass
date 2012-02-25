#include <iostream>
#include <string>
#include "prelexer.hpp"

using std::cout;
using std::endl;
using std::string;
using namespace Sass::Prelexer;

extern const char msg1[] = "Matched an 'a'!";
extern const char msg2[] = "Failed to match a 'b'!";

int main() {
  
  prelexer p = exactly<'a'>;
  prelexer q = exactly<'b'>;
  
  if (p("abcd")) cout << msg1 << endl;
  if (!q("abcd")) cout << msg2 << endl;

  return 0;
}