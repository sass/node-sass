// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"

#include <cctype>
#include "utf8.h"
#include "ast.hpp"
#include "fn_utils.hpp"
#include "fn_strings.hpp"

namespace Sass {

  namespace Functions {

    void handle_utf8_error (const ParserState& pstate, Backtraces traces)
    {
      try {
       throw;
      }
      catch (utf8::invalid_code_point&) {
        std::string msg("utf8::invalid_code_point");
        error(msg, pstate, traces);
      }
      catch (utf8::not_enough_room&) {
        std::string msg("utf8::not_enough_room");
        error(msg, pstate, traces);
      }
      catch (utf8::invalid_utf8&) {
        std::string msg("utf8::invalid_utf8");
        error(msg, pstate, traces);
      }
      catch (...) { throw; }
    }

    ///////////////////
    // STRING FUNCTIONS
    ///////////////////

    Signature unquote_sig = "unquote($string)";
    BUILT_IN(sass_unquote)
    {
      AST_Node_Obj arg = env["$string"];
      if (String_Quoted* string_quoted = Cast<String_Quoted>(arg)) {
        String_Constant* result = SASS_MEMORY_NEW(String_Constant, pstate, string_quoted->value());
        // remember if the string was quoted (color tokens)
        result->is_delayed(true); // delay colors
        return result;
      }
      else if (String_Constant* str = Cast<String_Constant>(arg)) {
        return str;
      }
      else if (Value* ex = Cast<Value>(arg)) {
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
      const String_Constant* s = ARG("$string", String_Constant);
      String_Quoted *result = SASS_MEMORY_NEW(
          String_Quoted, pstate, s->value(),
          /*q=*/'\0', /*keep_utf8_escapes=*/false, /*skip_unquoting=*/true);
      result->quote_mark('*');
      return result;
    }

    Signature str_length_sig = "str-length($string)";
    BUILT_IN(str_length)
    {
      size_t len = std::string::npos;
      try {
        String_Constant* s = ARG("$string", String_Constant);
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
        String_Constant* s = ARG("$string", String_Constant);
        str = s->value();
        String_Constant* i = ARG("$insert", String_Constant);
        std::string ins = i->value();
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

        if (String_Quoted* ss = Cast<String_Quoted>(s)) {
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
        String_Constant* s = ARG("$string", String_Constant);
        String_Constant* t = ARG("$substring", String_Constant);
        std::string str = s->value();
        std::string substr = t->value();

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
        String_Constant* s = ARG("$string", String_Constant);
        double start_at = ARGVAL("$start-at");
        double end_at = ARGVAL("$end-at");
        String_Quoted* ss = Cast<String_Quoted>(s);

        std::string str(s->value());

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
      String_Constant* s = ARG("$string", String_Constant);
      std::string str = s->value();

      for (size_t i = 0, L = str.length(); i < L; ++i) {
        if (Sass::Util::isAscii(str[i])) {
          str[i] = std::toupper(str[i]);
        }
      }

      if (String_Quoted* ss = Cast<String_Quoted>(s)) {
        String_Quoted* cpy = SASS_MEMORY_COPY(ss);
        cpy->value(str);
        return cpy;
      } else {
        return SASS_MEMORY_NEW(String_Quoted, pstate, str);
      }
    }

    Signature to_lower_case_sig = "to-lower-case($string)";
    BUILT_IN(to_lower_case)
    {
      String_Constant* s = ARG("$string", String_Constant);
      std::string str = s->value();

      for (size_t i = 0, L = str.length(); i < L; ++i) {
        if (Sass::Util::isAscii(str[i])) {
          str[i] = std::tolower(str[i]);
        }
      }

      if (String_Quoted* ss = Cast<String_Quoted>(s)) {
        String_Quoted* cpy = SASS_MEMORY_COPY(ss);
        cpy->value(str);
        return cpy;
      } else {
        return SASS_MEMORY_NEW(String_Quoted, pstate, str);
      }
    }

  }

}
