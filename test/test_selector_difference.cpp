#include "../ast.hpp"
#include "../context.hpp"
#include "../parser.hpp"
#include "../to_string.hpp"
#include <string>
#include <iostream>

using namespace Sass;

Context ctx = Context::Data();
To_String to_string;

Compound_Selector* selector(std::string src)
{ return Parser::from_c_str(src.c_str(), ctx, "", Position()).parse_compound_selector(); }

void diff(std::string s, std::string t)
{
  std::cout << s << " - " << t << " = " << selector(s + ";")->minus(selector(t + ";"), ctx)->perform(&to_string) << std::endl;
}

int main()
{
  diff(".a.b.c", ".c.b");
  diff(".a.b.c", ".fludge.b");

  return 0;
}
