#include "ast.hpp"
#include "context.hpp"
#include "parser.hpp"
#include "to_string.hpp"
#include <string>

using namespace Sass;

Context ctx = Context(Context::Data());
To_String to_string;

Compound_Selector* compound_selector(string src, Context& ctx)
{ return Parser::from_c_str(src.c_str(), ctx, "", 0).parse_simple_selector_sequence(); }

void check(Compound_Selector* s1, Compound_Selector* s2)
{
  cout << "Is "
       << s1->perform(&to_string)
       << " a superselector of "
       << s2->perform(&to_string)
       << "?\t"
       << s1->is_superselector_of(s2)
       << endl;
}

int main() {
  Compound_Selector* s1 = compound_selector(".foo.bar;", ctx);
  Compound_Selector* s2 = compound_selector(".foo;", ctx);
  Compound_Selector* s3 = compound_selector("div.foo;", ctx);
  Compound_Selector* s4 = compound_selector("div.bar.foo;", ctx);
  Compound_Selector* s5 = compound_selector("p.foo;", ctx);

  check(s2, s1);
  check(s1, s2);
  check(s1, s3);
  check(s2, s3);
  check(s3, s2);
  check(s3, s4);
  check(s5, s4);

  return 0;
}


