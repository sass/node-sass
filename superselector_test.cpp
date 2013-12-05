#include "ast.hpp"
#include "context.hpp"
#include "parser.hpp"
#include "to_string.hpp"
#include <string>

using namespace Sass;

Compound_Selector* compound_selector(string src, Context& ctx)
{ return Parser::from_c_str(src.c_str(), ctx, "", 0).parse_simple_selector_sequence(); }

int main() {
	Context ctx = Context(Context::Data());
	To_String to_string;
	Compound_Selector* sel = compound_selector(".foo.bar[hux  =  'mumble'];", ctx);
	cout << sel->perform(&to_string) << endl;
	return 0;
}


