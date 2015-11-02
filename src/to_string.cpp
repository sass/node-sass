#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "ast.hpp"
#include "inspect.hpp"
#include "context.hpp"
#include "to_string.hpp"

namespace Sass {

  To_String::To_String(Context* ctx, bool in_declaration, bool in_debug)
  : ctx(ctx), in_declaration(in_declaration), in_debug(in_debug) { }
  To_String::~To_String() { }

  inline std::string To_String::fallback_impl(AST_Node* n)
  {
    Emitter emitter(ctx);
    Inspect i(emitter);
    i.in_declaration = in_declaration;
    i.in_debug = in_debug;
    if (n) n->perform(&i);
    return i.get_buffer();
  }

  inline std::string To_String::operator()(String_Schema* s)
  {
    std::string acc("");
    for (size_t i = 0, L = s->length(); i < L; ++i) {
      acc += s->elements()[i]->perform(this);
    }
    return acc;
  }

  inline std::string To_String::operator()(String_Quoted* s)
  {
    return s->value();
  }

  inline std::string To_String::operator()(String_Constant* s)
  {
    return s->value();
  }

  inline std::string To_String::operator()(Null* n)
  { return in_debug ? "null" : ""; }
}
