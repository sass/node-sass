#include "ast.hpp"
#include "context.hpp"
#include "parser.hpp"
#include "to_string.hpp"
#include <string>

using namespace Sass;

Context ctx = Context(Context::Data());
To_String to_string;

Compound_Selector* compound_selector(string src)
{ return Parser::from_c_str(src.c_str(), ctx, "", 0).parse_simple_selector_sequence(); }

void check(string s1, string s2)
{
  cout << "Is "
       << s1
       << " a superselector of "
       << s2
       << "?\t"
       << compound_selector(s1 + ";")->is_superselector_of(compound_selector(s2 + ";"))
       << endl;
}

int main()
{
  check(".foo", ".foo.bar");
  check(".foo.bar", ".foo");
  check(".foo.bar", "div.foo");
  check(".foo", "div.foo");
  check("div.foo", ".foo");
  check("div.foo", "div.bar.foo");
  check("p.foo", "div.bar.foo");

  return 0;
}


