#include "sass.h"
#include "values.hpp"

#include <stdint.h>

namespace Sass {
  using namespace std;

  // convert value from C++ side to C-API
  union Sass_Value* ast_node_to_sass_value (const Expression* val)
  {
    if (val->concrete_type() == Expression::NUMBER)
    {
      const Number* res = dynamic_cast<const Number*>(val);
      return sass_make_number(res->value(), res->unit().c_str());
    }
    else if (val->concrete_type() == Expression::COLOR)
    {
      const Color* col = dynamic_cast<const Color*>(val);
      return sass_make_color(col->r(), col->g(), col->b(), col->a());
    }
    else if (val->concrete_type() == Expression::LIST)
    {
      const List* l = dynamic_cast<const List*>(val);
      union Sass_Value* list = sass_make_list(l->size(), l->separator());
      for (size_t i = 0, L = l->length(); i < L; ++i) {
        auto val = ast_node_to_sass_value((*l)[i]);
        sass_list_set_value(list, i, val);
      }
      return list;
    }
    else if (val->concrete_type() == Expression::MAP)
    {
      const Map* m = dynamic_cast<const Map*>(val);
      union Sass_Value* map = sass_make_map(m->length());
      size_t i = 0; for (auto key : m->keys()) {
        sass_map_set_key(map, i, ast_node_to_sass_value(key));
        sass_map_set_value(map, i, ast_node_to_sass_value(m->at(key)));
        ++ i;
      }
      return map;
    }
    else if (val->concrete_type() == Expression::NULL_VAL)
    {
      return sass_make_null();
    }
    else if (val->concrete_type() == Expression::BOOLEAN)
    {
      const Boolean* res = dynamic_cast<const Boolean*>(val);
      return sass_make_boolean(res->value());
    }
    else if (val->concrete_type() == Expression::STRING)
    {
      if (const String_Quoted* qstr = dynamic_cast<const String_Quoted*>(val))
      {
        return sass_make_qstring(qstr->value().c_str());
      }
      else if (const String_Constant* cstr = dynamic_cast<const String_Constant*>(val))
      {
        return sass_make_string(cstr->value().c_str());
      }
    }
    return sass_make_error("unknown sass value type");
  }

  // convert value from C-API to C++ side
  Value* sass_value_to_ast_node (Memory_Manager<AST_Node>& mem, const union Sass_Value* val)
  {
    switch (sass_value_get_tag(val)) {
      case SASS_NUMBER:
        return new (mem) Number(ParserState("[C-VALUE]"),
                                sass_number_get_value(val),
                                sass_number_get_unit(val));
      break;
      case SASS_BOOLEAN:
        return new (mem) Boolean(ParserState("[C-VALUE]"),
                                 sass_boolean_get_value(val));
      break;
      case SASS_COLOR:
        return new (mem) Color(ParserState("[C-VALUE]"),
                               sass_color_get_r(val),
                               sass_color_get_g(val),
                               sass_color_get_b(val),
                               sass_color_get_a(val));
      break;
      case SASS_STRING:
        return new (mem) String_Quoted(ParserState("[C-VALUE]"),
                                       sass_string_get_value(val));
      break;
      case SASS_LIST: {
        List* l = new (mem) List(ParserState("[C-VALUE]"),
                                 sass_list_get_length(val),
                                 sass_list_get_separator(val));
        for (size_t i = 0, L = sass_list_get_length(val); i < L; ++i) {
          *l << sass_value_to_ast_node(mem, sass_list_get_value(val, i));
        }
        return l;
      }
      break;
      case SASS_MAP: {
        Map* m = new (mem) Map(ParserState("[C-VALUE]"));
        for (size_t i = 0, L = sass_map_get_length(val); i < L; ++i) {
          *m << std::make_pair(
            sass_value_to_ast_node(mem, sass_map_get_key(val, i)),
            sass_value_to_ast_node(mem, sass_map_get_value(val, i)));
        }
        return m;
      }
      break;
      case SASS_NULL:
        return new (mem) Null(ParserState("[C-VALUE]"));
      break;
      case SASS_ERROR:
        return new (mem) Custom_Error(ParserState("[C-VALUE]"),
                                      sass_error_get_message(val));
      break;
      case SASS_WARNING:
        return new (mem) Custom_Warning(ParserState("[C-VALUE]"),
                                        sass_warning_get_message(val));
      break;
    }
    return 0;
  }

}
