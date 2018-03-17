#include "sass.hpp"
#include "functions.hpp"
#include "ast.hpp"
#include "context.hpp"
#include "backtrace.hpp"
#include "parser.hpp"
#include "constants.hpp"
#include "inspect.hpp"
#include "extend.hpp"
#include "eval.hpp"
#include "util.hpp"
#include "expand.hpp"
#include "operators.hpp"
#include "utf8_string.hpp"
#include "sass/base.h"
#include "utf8.h"

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cctype>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <set>

#include "fn_utils.hpp"
#include "fn_colors.hpp"
#include "fn_numbers.hpp"
#include "fn_selectors.hpp"

namespace Sass {
  using std::stringstream;
  using std::endl;

  std::string function_name(Signature sig)
  {
    std::string str(sig);
    return str.substr(0, str.find('('));
  }

  namespace Functions {

    inline void handle_utf8_error (const ParserState& pstate, Backtraces traces)
    {
      try {
       throw;
      }
      catch (utf8::invalid_code_point) {
        std::string msg("utf8::invalid_code_point");
        error(msg, pstate, traces);
      }
      catch (utf8::not_enough_room) {
        std::string msg("utf8::not_enough_room");
        error(msg, pstate, traces);
      }
      catch (utf8::invalid_utf8) {
        std::string msg("utf8::invalid_utf8");
        error(msg, pstate, traces);
      }
      catch (...) { throw; }
    }

    // features
    static std::set<std::string> features {
      "global-variable-shadowing",
      "extend-selector-pseudoclass",
      "at-error",
      "units-level-3",
      "custom-property"
    };

    ///////////////////
    // STRING FUNCTIONS
    ///////////////////

    Signature unquote_sig = "unquote($string)";
    BUILT_IN(sass_unquote)
    {
      AST_Node_Obj arg = env["$string"];
      if (String_Quoted_Ptr string_quoted = Cast<String_Quoted>(arg)) {
        String_Constant_Ptr result = SASS_MEMORY_NEW(String_Constant, pstate, string_quoted->value());
        // remember if the string was quoted (color tokens)
        result->is_delayed(true); // delay colors
        return result;
      }
      else if (String_Constant_Ptr str = Cast<String_Constant>(arg)) {
        return str;
      }
      else if (Expression_Ptr ex = Cast<Expression>(arg)) {
        Sass_Output_Style oldstyle = ctx.c_options.output_style;
        ctx.c_options.output_style = SASS_STYLE_NESTED;
        std::string val(arg->to_string(ctx.c_options));
        val = Cast<Null>(arg) ? "null" : val;
        ctx.c_options.output_style = oldstyle;

        deprecated_function("Passing " + val + ", a non-string value, to unquote()", pstate);
        return ex;
      }
      throw std::runtime_error("Invalid Data Type for unquote");
    }

    Signature quote_sig = "quote($string)";
    BUILT_IN(sass_quote)
    {
      AST_Node_Obj arg = env["$string"];
      // only set quote mark to true if already a string
      if (String_Quoted_Ptr qstr = Cast<String_Quoted>(arg)) {
        qstr->quote_mark('*');
        return qstr;
      }
      // all other nodes must be converted to a string node
      std::string str(quote(arg->to_string(ctx.c_options), String_Constant::double_quote()));
      String_Quoted_Ptr result = SASS_MEMORY_NEW(String_Quoted, pstate, str);
      result->quote_mark('*');
      return result;
    }


    Signature str_length_sig = "str-length($string)";
    BUILT_IN(str_length)
    {
      size_t len = std::string::npos;
      try {
        String_Constant_Ptr s = ARG("$string", String_Constant);
        len = UTF_8::code_point_count(s->value(), 0, s->value().size());

      }
      // handle any invalid utf8 errors
      // other errors will be re-thrown
      catch (...) { handle_utf8_error(pstate, traces); }
      // return something even if we had an error (-1)
      return SASS_MEMORY_NEW(Number, pstate, (double)len);
    }

    Signature str_insert_sig = "str-insert($string, $insert, $index)";
    BUILT_IN(str_insert)
    {
      std::string str;
      try {
        String_Constant_Ptr s = ARG("$string", String_Constant);
        str = s->value();
        str = unquote(str);
        String_Constant_Ptr i = ARG("$insert", String_Constant);
        std::string ins = i->value();
        ins = unquote(ins);
        double index = ARGVAL("$index");
        size_t len = UTF_8::code_point_count(str, 0, str.size());

        if (index > 0 && index <= len) {
          // positive and within string length
          str.insert(UTF_8::offset_at_position(str, static_cast<size_t>(index) - 1), ins);
        }
        else if (index > len) {
          // positive and past string length
          str += ins;
        }
        else if (index == 0) {
          str = ins + str;
        }
        else if (std::abs(index) <= len) {
          // negative and within string length
          index += len + 1;
          str.insert(UTF_8::offset_at_position(str, static_cast<size_t>(index)), ins);
        }
        else {
          // negative and past string length
          str = ins + str;
        }

        if (String_Quoted_Ptr ss = Cast<String_Quoted>(s)) {
          if (ss->quote_mark()) str = quote(str);
        }
      }
      // handle any invalid utf8 errors
      // other errors will be re-thrown
      catch (...) { handle_utf8_error(pstate, traces); }
      return SASS_MEMORY_NEW(String_Quoted, pstate, str);
    }

    Signature str_index_sig = "str-index($string, $substring)";
    BUILT_IN(str_index)
    {
      size_t index = std::string::npos;
      try {
        String_Constant_Ptr s = ARG("$string", String_Constant);
        String_Constant_Ptr t = ARG("$substring", String_Constant);
        std::string str = s->value();
        str = unquote(str);
        std::string substr = t->value();
        substr = unquote(substr);

        size_t c_index = str.find(substr);
        if(c_index == std::string::npos) {
          return SASS_MEMORY_NEW(Null, pstate);
        }
        index = UTF_8::code_point_count(str, 0, c_index) + 1;
      }
      // handle any invalid utf8 errors
      // other errors will be re-thrown
      catch (...) { handle_utf8_error(pstate, traces); }
      // return something even if we had an error (-1)
      return SASS_MEMORY_NEW(Number, pstate, (double)index);
    }

    Signature str_slice_sig = "str-slice($string, $start-at, $end-at:-1)";
    BUILT_IN(str_slice)
    {
      std::string newstr;
      try {
        String_Constant_Ptr s = ARG("$string", String_Constant);
        double start_at = ARGVAL("$start-at");
        double end_at = ARGVAL("$end-at");
        String_Quoted_Ptr ss = Cast<String_Quoted>(s);

        std::string str = unquote(s->value());

        size_t size = utf8::distance(str.begin(), str.end());

        if (!Cast<Number>(env["$end-at"])) {
          end_at = -1;
        }

        if (end_at == 0 || (end_at + size) < 0) {
          if (ss && ss->quote_mark()) newstr = quote("");
          return SASS_MEMORY_NEW(String_Quoted, pstate, newstr);
        }

        if (end_at < 0) {
          end_at += size + 1;
          if (end_at == 0) end_at = 1;
        }
        if (end_at > size) { end_at = (double)size; }
        if (start_at < 0) {
          start_at += size + 1;
          if (start_at < 0)  start_at = 0;
        }
        else if (start_at == 0) { ++ start_at; }

        if (start_at <= end_at)
        {
          std::string::iterator start = str.begin();
          utf8::advance(start, start_at - 1, str.end());
          std::string::iterator end = start;
          utf8::advance(end, end_at - start_at + 1, str.end());
          newstr = std::string(start, end);
        }
        if (ss) {
          if(ss->quote_mark()) newstr = quote(newstr);
        }
      }
      // handle any invalid utf8 errors
      // other errors will be re-thrown
      catch (...) { handle_utf8_error(pstate, traces); }
      return SASS_MEMORY_NEW(String_Quoted, pstate, newstr);
    }

    Signature to_upper_case_sig = "to-upper-case($string)";
    BUILT_IN(to_upper_case)
    {
      String_Constant_Ptr s = ARG("$string", String_Constant);
      std::string str = s->value();

      for (size_t i = 0, L = str.length(); i < L; ++i) {
        if (Sass::Util::isAscii(str[i])) {
          str[i] = std::toupper(str[i]);
        }
      }

      if (String_Quoted_Ptr ss = Cast<String_Quoted>(s)) {
        String_Quoted_Ptr cpy = SASS_MEMORY_COPY(ss);
        cpy->value(str);
        return cpy;
      } else {
        return SASS_MEMORY_NEW(String_Quoted, pstate, str);
      }
    }

    Signature to_lower_case_sig = "to-lower-case($string)";
    BUILT_IN(to_lower_case)
    {
      String_Constant_Ptr s = ARG("$string", String_Constant);
      std::string str = s->value();

      for (size_t i = 0, L = str.length(); i < L; ++i) {
        if (Sass::Util::isAscii(str[i])) {
          str[i] = std::tolower(str[i]);
        }
      }

      if (String_Quoted_Ptr ss = Cast<String_Quoted>(s)) {
        String_Quoted_Ptr cpy = SASS_MEMORY_COPY(ss);
        cpy->value(str);
        return cpy;
      } else {
        return SASS_MEMORY_NEW(String_Quoted, pstate, str);
      }
    }
    /////////////////
    // LIST FUNCTIONS
    /////////////////

    Signature length_sig = "length($list)";
    BUILT_IN(length)
    {
      if (Selector_List_Ptr sl = Cast<Selector_List>(env["$list"])) {
        return SASS_MEMORY_NEW(Number, pstate, (double)sl->length());
      }
      Expression_Ptr v = ARG("$list", Expression);
      if (v->concrete_type() == Expression::MAP) {
        Map_Ptr map = Cast<Map>(env["$list"]);
        return SASS_MEMORY_NEW(Number, pstate, (double)(map ? map->length() : 1));
      }
      if (v->concrete_type() == Expression::SELECTOR) {
        if (Compound_Selector_Ptr h = Cast<Compound_Selector>(v)) {
          return SASS_MEMORY_NEW(Number, pstate, (double)h->length());
        } else if (Selector_List_Ptr ls = Cast<Selector_List>(v)) {
          return SASS_MEMORY_NEW(Number, pstate, (double)ls->length());
        } else {
          return SASS_MEMORY_NEW(Number, pstate, 1);
        }
      }

      List_Ptr list = Cast<List>(env["$list"]);
      return SASS_MEMORY_NEW(Number,
                             pstate,
                             (double)(list ? list->size() : 1));
    }

    Signature nth_sig = "nth($list, $n)";
    BUILT_IN(nth)
    {
      double nr = ARGVAL("$n");
      Map_Ptr m = Cast<Map>(env["$list"]);
      if (Selector_List_Ptr sl = Cast<Selector_List>(env["$list"])) {
        size_t len = m ? m->length() : sl->length();
        bool empty = m ? m->empty() : sl->empty();
        if (empty) error("argument `$list` of `" + std::string(sig) + "` must not be empty", pstate, traces);
        double index = std::floor(nr < 0 ? len + nr : nr - 1);
        if (index < 0 || index > len - 1) error("index out of bounds for `" + std::string(sig) + "`", pstate, traces);
        // return (*sl)[static_cast<int>(index)];
        Listize listize;
        return (*sl)[static_cast<int>(index)]->perform(&listize);
      }
      List_Obj l = Cast<List>(env["$list"]);
      if (nr == 0) error("argument `$n` of `" + std::string(sig) + "` must be non-zero", pstate, traces);
      // if the argument isn't a list, then wrap it in a singleton list
      if (!m && !l) {
        l = SASS_MEMORY_NEW(List, pstate, 1);
        l->append(ARG("$list", Expression));
      }
      size_t len = m ? m->length() : l->length();
      bool empty = m ? m->empty() : l->empty();
      if (empty) error("argument `$list` of `" + std::string(sig) + "` must not be empty", pstate, traces);
      double index = std::floor(nr < 0 ? len + nr : nr - 1);
      if (index < 0 || index > len - 1) error("index out of bounds for `" + std::string(sig) + "`", pstate, traces);

      if (m) {
        l = SASS_MEMORY_NEW(List, pstate, 1);
        l->append(m->keys()[static_cast<unsigned int>(index)]);
        l->append(m->at(m->keys()[static_cast<unsigned int>(index)]));
        return l.detach();
      }
      else {
        Expression_Obj rv = l->value_at_index(static_cast<int>(index));
        rv->set_delayed(false);
        return rv.detach();
      }
    }

    Signature set_nth_sig = "set-nth($list, $n, $value)";
    BUILT_IN(set_nth)
    {
      Map_Obj m = Cast<Map>(env["$list"]);
      List_Obj l = Cast<List>(env["$list"]);
      Number_Obj n = ARG("$n", Number);
      Expression_Obj v = ARG("$value", Expression);
      if (!l) {
        l = SASS_MEMORY_NEW(List, pstate, 1);
        l->append(ARG("$list", Expression));
      }
      if (m) {
        l = m->to_list(pstate);
      }
      if (l->empty()) error("argument `$list` of `" + std::string(sig) + "` must not be empty", pstate, traces);
      double index = std::floor(n->value() < 0 ? l->length() + n->value() : n->value() - 1);
      if (index < 0 || index > l->length() - 1) error("index out of bounds for `" + std::string(sig) + "`", pstate, traces);
      List_Ptr result = SASS_MEMORY_NEW(List, pstate, l->length(), l->separator(), false, l->is_bracketed());
      for (size_t i = 0, L = l->length(); i < L; ++i) {
        result->append(((i == index) ? v : (*l)[i]));
      }
      return result;
    }

    Signature index_sig = "index($list, $value)";
    BUILT_IN(index)
    {
      Map_Obj m = Cast<Map>(env["$list"]);
      List_Obj l = Cast<List>(env["$list"]);
      Expression_Obj v = ARG("$value", Expression);
      if (!l) {
        l = SASS_MEMORY_NEW(List, pstate, 1);
        l->append(ARG("$list", Expression));
      }
      if (m) {
        l = m->to_list(pstate);
      }
      for (size_t i = 0, L = l->length(); i < L; ++i) {
        if (Operators::eq(l->value_at_index(i), v)) return SASS_MEMORY_NEW(Number, pstate, (double)(i+1));
      }
      return SASS_MEMORY_NEW(Null, pstate);
    }

    Signature join_sig = "join($list1, $list2, $separator: auto, $bracketed: auto)";
    BUILT_IN(join)
    {
      Map_Obj m1 = Cast<Map>(env["$list1"]);
      Map_Obj m2 = Cast<Map>(env["$list2"]);
      List_Obj l1 = Cast<List>(env["$list1"]);
      List_Obj l2 = Cast<List>(env["$list2"]);
      String_Constant_Obj sep = ARG("$separator", String_Constant);
      enum Sass_Separator sep_val = (l1 ? l1->separator() : SASS_SPACE);
      Value* bracketed = ARG("$bracketed", Value);
      bool is_bracketed = (l1 ? l1->is_bracketed() : false);
      if (!l1) {
        l1 = SASS_MEMORY_NEW(List, pstate, 1);
        l1->append(ARG("$list1", Expression));
        sep_val = (l2 ? l2->separator() : SASS_SPACE);
        is_bracketed = (l2 ? l2->is_bracketed() : false);
      }
      if (!l2) {
        l2 = SASS_MEMORY_NEW(List, pstate, 1);
        l2->append(ARG("$list2", Expression));
      }
      if (m1) {
        l1 = m1->to_list(pstate);
        sep_val = SASS_COMMA;
      }
      if (m2) {
        l2 = m2->to_list(pstate);
      }
      size_t len = l1->length() + l2->length();
      std::string sep_str = unquote(sep->value());
      if (sep_str == "space") sep_val = SASS_SPACE;
      else if (sep_str == "comma") sep_val = SASS_COMMA;
      else if (sep_str != "auto") error("argument `$separator` of `" + std::string(sig) + "` must be `space`, `comma`, or `auto`", pstate, traces);
      String_Constant_Obj bracketed_as_str = Cast<String_Constant>(bracketed);
      bool bracketed_is_auto = bracketed_as_str && unquote(bracketed_as_str->value()) == "auto";
      if (!bracketed_is_auto) {
        is_bracketed = !bracketed->is_false();
      }
      List_Obj result = SASS_MEMORY_NEW(List, pstate, len, sep_val, false, is_bracketed);
      result->concat(l1);
      result->concat(l2);
      return result.detach();
    }

    Signature append_sig = "append($list, $val, $separator: auto)";
    BUILT_IN(append)
    {
      Map_Obj m = Cast<Map>(env["$list"]);
      List_Obj l = Cast<List>(env["$list"]);
      Expression_Obj v = ARG("$val", Expression);
      if (Selector_List_Ptr sl = Cast<Selector_List>(env["$list"])) {
        Listize listize;
        l = Cast<List>(sl->perform(&listize));
      }
      String_Constant_Obj sep = ARG("$separator", String_Constant);
      if (!l) {
        l = SASS_MEMORY_NEW(List, pstate, 1);
        l->append(ARG("$list", Expression));
      }
      if (m) {
        l = m->to_list(pstate);
      }
      List_Ptr result = SASS_MEMORY_COPY(l);
      std::string sep_str(unquote(sep->value()));
      if (sep_str != "auto") { // check default first
        if (sep_str == "space") result->separator(SASS_SPACE);
        else if (sep_str == "comma") result->separator(SASS_COMMA);
        else error("argument `$separator` of `" + std::string(sig) + "` must be `space`, `comma`, or `auto`", pstate, traces);
      }
      if (l->is_arglist()) {
        result->append(SASS_MEMORY_NEW(Argument,
                                       v->pstate(),
                                       v,
                                       "",
                                       false,
                                       false));

      } else {
        result->append(v);
      }
      return result;
    }

    Signature zip_sig = "zip($lists...)";
    BUILT_IN(zip)
    {
      List_Obj arglist = SASS_MEMORY_COPY(ARG("$lists", List));
      size_t shortest = 0;
      for (size_t i = 0, L = arglist->length(); i < L; ++i) {
        List_Obj ith = Cast<List>(arglist->value_at_index(i));
        Map_Obj mith = Cast<Map>(arglist->value_at_index(i));
        if (!ith) {
          if (mith) {
            ith = mith->to_list(pstate);
          } else {
            ith = SASS_MEMORY_NEW(List, pstate, 1);
            ith->append(arglist->value_at_index(i));
          }
          if (arglist->is_arglist()) {
            Argument_Obj arg = (Argument_Ptr)(arglist->at(i).ptr()); // XXX
            arg->value(ith);
          } else {
            (*arglist)[i] = ith;
          }
        }
        shortest = (i ? std::min(shortest, ith->length()) : ith->length());
      }
      List_Ptr zippers = SASS_MEMORY_NEW(List, pstate, shortest, SASS_COMMA);
      size_t L = arglist->length();
      for (size_t i = 0; i < shortest; ++i) {
        List_Ptr zipper = SASS_MEMORY_NEW(List, pstate, L);
        for (size_t j = 0; j < L; ++j) {
          zipper->append(Cast<List>(arglist->value_at_index(j))->at(i));
        }
        zippers->append(zipper);
      }
      return zippers;
    }

    Signature list_separator_sig = "list_separator($list)";
    BUILT_IN(list_separator)
    {
      List_Obj l = Cast<List>(env["$list"]);
      if (!l) {
        l = SASS_MEMORY_NEW(List, pstate, 1);
        l->append(ARG("$list", Expression));
      }
      return SASS_MEMORY_NEW(String_Quoted,
                               pstate,
                               l->separator() == SASS_COMMA ? "comma" : "space");
    }

    /////////////////
    // MAP FUNCTIONS
    /////////////////

    Signature map_get_sig = "map-get($map, $key)";
    BUILT_IN(map_get)
    {
      // leaks for "map-get((), foo)" if not Obj
      // investigate why this is (unexpected)
      Map_Obj m = ARGM("$map", Map, ctx);
      Expression_Obj v = ARG("$key", Expression);
      try {
        Expression_Obj val = m->at(v);
        if (!val) return SASS_MEMORY_NEW(Null, pstate);
        val->set_delayed(false);
        return val.detach();
      } catch (const std::out_of_range&) {
        return SASS_MEMORY_NEW(Null, pstate);
      }
      catch (...) { throw; }
    }

    Signature map_has_key_sig = "map-has-key($map, $key)";
    BUILT_IN(map_has_key)
    {
      Map_Obj m = ARGM("$map", Map, ctx);
      Expression_Obj v = ARG("$key", Expression);
      return SASS_MEMORY_NEW(Boolean, pstate, m->has(v));
    }

    Signature map_keys_sig = "map-keys($map)";
    BUILT_IN(map_keys)
    {
      Map_Obj m = ARGM("$map", Map, ctx);
      List_Ptr result = SASS_MEMORY_NEW(List, pstate, m->length(), SASS_COMMA);
      for ( auto key : m->keys()) {
        result->append(key);
      }
      return result;
    }

    Signature map_values_sig = "map-values($map)";
    BUILT_IN(map_values)
    {
      Map_Obj m = ARGM("$map", Map, ctx);
      List_Ptr result = SASS_MEMORY_NEW(List, pstate, m->length(), SASS_COMMA);
      for ( auto key : m->keys()) {
        result->append(m->at(key));
      }
      return result;
    }

    Signature map_merge_sig = "map-merge($map1, $map2)";
    BUILT_IN(map_merge)
    {
      Map_Obj m1 = ARGM("$map1", Map, ctx);
      Map_Obj m2 = ARGM("$map2", Map, ctx);

      size_t len = m1->length() + m2->length();
      Map_Ptr result = SASS_MEMORY_NEW(Map, pstate, len);
      // concat not implemented for maps
      *result += m1;
      *result += m2;
      return result;
    }

    Signature map_remove_sig = "map-remove($map, $keys...)";
    BUILT_IN(map_remove)
    {
      bool remove;
      Map_Obj m = ARGM("$map", Map, ctx);
      List_Obj arglist = ARG("$keys", List);
      Map_Ptr result = SASS_MEMORY_NEW(Map, pstate, 1);
      for (auto key : m->keys()) {
        remove = false;
        for (size_t j = 0, K = arglist->length(); j < K && !remove; ++j) {
          remove = Operators::eq(key, arglist->value_at_index(j));
        }
        if (!remove) *result << std::make_pair(key, m->at(key));
      }
      return result;
    }

    Signature keywords_sig = "keywords($args)";
    BUILT_IN(keywords)
    {
      List_Obj arglist = SASS_MEMORY_COPY(ARG("$args", List)); // copy
      Map_Obj result = SASS_MEMORY_NEW(Map, pstate, 1);
      for (size_t i = arglist->size(), L = arglist->length(); i < L; ++i) {
        Expression_Obj obj = arglist->at(i);
        Argument_Obj arg = (Argument_Ptr) obj.ptr(); // XXX
        std::string name = std::string(arg->name());
        name = name.erase(0, 1); // sanitize name (remove dollar sign)
        *result << std::make_pair(SASS_MEMORY_NEW(String_Quoted,
                 pstate, name),
                 arg->value());
      }
      return result.detach();
    }

    //////////////////////////
    // INTROSPECTION FUNCTIONS
    //////////////////////////

    Signature type_of_sig = "type-of($value)";
    BUILT_IN(type_of)
    {
      Expression_Ptr v = ARG("$value", Expression);
      return SASS_MEMORY_NEW(String_Quoted, pstate, v->type());
    }

    Signature variable_exists_sig = "variable-exists($name)";
    BUILT_IN(variable_exists)
    {
      std::string s = Util::normalize_underscores(unquote(ARG("$name", String_Constant)->value()));

      if(d_env.has("$"+s)) {
        return SASS_MEMORY_NEW(Boolean, pstate, true);
      }
      else {
        return SASS_MEMORY_NEW(Boolean, pstate, false);
      }
    }

    Signature global_variable_exists_sig = "global-variable-exists($name)";
    BUILT_IN(global_variable_exists)
    {
      std::string s = Util::normalize_underscores(unquote(ARG("$name", String_Constant)->value()));

      if(d_env.has_global("$"+s)) {
        return SASS_MEMORY_NEW(Boolean, pstate, true);
      }
      else {
        return SASS_MEMORY_NEW(Boolean, pstate, false);
      }
    }

    Signature function_exists_sig = "function-exists($name)";
    BUILT_IN(function_exists)
    {
      String_Constant_Ptr ss = Cast<String_Constant>(env["$name"]);
      if (!ss) {
        error("$name: " + (env["$name"]->to_string()) + " is not a string for `function-exists'", pstate, traces);
      }

      std::string name = Util::normalize_underscores(unquote(ss->value()));

      if(d_env.has_global(name+"[f]")) {
        return SASS_MEMORY_NEW(Boolean, pstate, true);
      }
      else {
        return SASS_MEMORY_NEW(Boolean, pstate, false);
      }
    }

    Signature mixin_exists_sig = "mixin-exists($name)";
    BUILT_IN(mixin_exists)
    {
      std::string s = Util::normalize_underscores(unquote(ARG("$name", String_Constant)->value()));

      if(d_env.has_global(s+"[m]")) {
        return SASS_MEMORY_NEW(Boolean, pstate, true);
      }
      else {
        return SASS_MEMORY_NEW(Boolean, pstate, false);
      }
    }

    Signature feature_exists_sig = "feature-exists($name)";
    BUILT_IN(feature_exists)
    {
      std::string s = unquote(ARG("$name", String_Constant)->value());

      if(features.find(s) == features.end()) {
        return SASS_MEMORY_NEW(Boolean, pstate, false);
      }
      else {
        return SASS_MEMORY_NEW(Boolean, pstate, true);
      }
    }

    Signature call_sig = "call($name, $args...)";
    BUILT_IN(call)
    {
      std::string name;
      Function_Ptr ff = Cast<Function>(env["$name"]);
      String_Constant_Ptr ss = Cast<String_Constant>(env["$name"]);

      if (ss) {
        name = Util::normalize_underscores(unquote(ss->value()));
        std::cerr << "DEPRECATION WARNING: ";
        std::cerr << "Passing a string to call() is deprecated and will be illegal" << std::endl;
        std::cerr << "in Sass 4.0. Use call(get-function(" + quote(name) + ")) instead." << std::endl;
        std::cerr << std::endl;
      } else if (ff) {
        name = ff->name();
      }

      List_Obj arglist = SASS_MEMORY_COPY(ARG("$args", List));

      Arguments_Obj args = SASS_MEMORY_NEW(Arguments, pstate);
      // std::string full_name(name + "[f]");
      // Definition_Ptr def = d_env.has(full_name) ? Cast<Definition>((d_env)[full_name]) : 0;
      // Parameters_Ptr params = def ? def->parameters() : 0;
      // size_t param_size = params ? params->length() : 0;
      for (size_t i = 0, L = arglist->length(); i < L; ++i) {
        Expression_Obj expr = arglist->value_at_index(i);
        // if (params && params->has_rest_parameter()) {
        //   Parameter_Obj p = param_size > i ? (*params)[i] : 0;
        //   List_Ptr list = Cast<List>(expr);
        //   if (list && p && !p->is_rest_parameter()) expr = (*list)[0];
        // }
        if (arglist->is_arglist()) {
          Expression_Obj obj = arglist->at(i);
          Argument_Obj arg = (Argument_Ptr) obj.ptr(); // XXX
          args->append(SASS_MEMORY_NEW(Argument,
                                       pstate,
                                       expr,
                                       arg ? arg->name() : "",
                                       arg ? arg->is_rest_argument() : false,
                                       arg ? arg->is_keyword_argument() : false));
        } else {
          args->append(SASS_MEMORY_NEW(Argument, pstate, expr));
        }
      }
      Function_Call_Obj func = SASS_MEMORY_NEW(Function_Call, pstate, name, args);
      Expand expand(ctx, &d_env, &selector_stack);
      func->via_call(true); // calc invoke is allowed
      if (ff) func->func(ff);
      return func->perform(&expand.eval);
    }

    ////////////////////
    // BOOLEAN FUNCTIONS
    ////////////////////

    Signature not_sig = "not($value)";
    BUILT_IN(sass_not)
    {
      return SASS_MEMORY_NEW(Boolean, pstate, ARG("$value", Expression)->is_false());
    }

    Signature if_sig = "if($condition, $if-true, $if-false)";
    // BUILT_IN(sass_if)
    // { return ARG("$condition", Expression)->is_false() ? ARG("$if-false", Expression) : ARG("$if-true", Expression); }
    BUILT_IN(sass_if)
    {
      Expand expand(ctx, &d_env, &selector_stack);
      Expression_Obj cond = ARG("$condition", Expression)->perform(&expand.eval);
      bool is_true = !cond->is_false();
      Expression_Obj res = ARG(is_true ? "$if-true" : "$if-false", Expression);
      res = res->perform(&expand.eval);
      res->set_delayed(false); // clone?
      return res.detach();
    }

    //////////////////////////
    // MISCELLANEOUS FUNCTIONS
    //////////////////////////

    // value.check_deprecated_interp if value.is_a?(Sass::Script::Value::String)
    // unquoted_string(value.to_sass)

    Signature inspect_sig = "inspect($value)";
    BUILT_IN(inspect)
    {
      Expression_Ptr v = ARG("$value", Expression);
      if (v->concrete_type() == Expression::NULL_VAL) {
        return SASS_MEMORY_NEW(String_Quoted, pstate, "null");
      } else if (v->concrete_type() == Expression::BOOLEAN && v->is_false()) {
        return SASS_MEMORY_NEW(String_Quoted, pstate, "false");
      } else if (v->concrete_type() == Expression::STRING) {
        return v;
      } else {
        // ToDo: fix to_sass for nested parentheses
        Sass_Output_Style old_style;
        old_style = ctx.c_options.output_style;
        ctx.c_options.output_style = TO_SASS;
        Emitter emitter(ctx.c_options);
        Inspect i(emitter);
        i.in_declaration = false;
        v->perform(&i);
        ctx.c_options.output_style = old_style;
        return SASS_MEMORY_NEW(String_Quoted, pstate, i.get_buffer());
      }
      // return v;
    }

    Signature is_bracketed_sig = "is-bracketed($list)";
    BUILT_IN(is_bracketed)
    {
      Value_Obj value = ARG("$list", Value);
      List_Obj list = Cast<List>(value);
      return SASS_MEMORY_NEW(Boolean, pstate, list && list->is_bracketed());
    }

    Signature content_exists_sig = "content-exists()";
    BUILT_IN(content_exists)
    {
      if (!d_env.has_global("is_in_mixin")) {
        error("Cannot call content-exists() except within a mixin.", pstate, traces);
      }
      return SASS_MEMORY_NEW(Boolean, pstate, d_env.has_lexical("@content[m]"));
    }

    Signature get_function_sig = "get-function($name, $css: false)";
    BUILT_IN(get_function)
    {
      String_Constant_Ptr ss = Cast<String_Constant>(env["$name"]);
      if (!ss) {
        error("$name: " + (env["$name"]->to_string()) + " is not a string for `get-function'", pstate, traces);
      }

      std::string name = Util::normalize_underscores(unquote(ss->value()));
      std::string full_name = name + "[f]";

      Boolean_Obj css = ARG("$css", Boolean);
      if (!css->is_false()) {
        Definition_Ptr def = SASS_MEMORY_NEW(Definition,
                                         pstate,
                                         name,
                                         SASS_MEMORY_NEW(Parameters, pstate),
                                         SASS_MEMORY_NEW(Block, pstate, 0, false),
                                         Definition::FUNCTION);
        return SASS_MEMORY_NEW(Function, pstate, def, true);
      }


      if (!d_env.has_global(full_name)) {
        error("Function not found: " + name, pstate, traces);
      }

      Definition_Ptr def = Cast<Definition>(d_env[full_name]);
      return SASS_MEMORY_NEW(Function, pstate, def, false);
    }
  }
}
