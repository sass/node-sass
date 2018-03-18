#include "sass.hpp"
#include "ast.hpp"
#include "context.hpp"
#include "node.hpp"
#include "eval.hpp"
#include "extend.hpp"
#include "emitter.hpp"
#include "color_maps.hpp"
#include "ast_fwd_decl.hpp"
#include <set>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include "ast_values.hpp"

namespace Sass {

  void str_rtrim(std::string& str, const std::string& delimiters = " \f\n\r\t\v")
  {
    str.erase( str.find_last_not_of( delimiters ) + 1 );
  }

  void String_Constant::rtrim()
  {
    str_rtrim(value_);
  }

  void String_Schema::rtrim()
  {
    if (!empty()) {
      if (String_Ptr str = Cast<String>(last())) str->rtrim();
    }
  }

  Number::Number(ParserState pstate, double val, std::string u, bool zero)
  : Value(pstate),
    Units(),
    value_(val),
    zero_(zero),
    hash_(0)
  {
    size_t l = 0;
    size_t r;
    if (!u.empty()) {
      bool nominator = true;
      while (true) {
        r = u.find_first_of("*/", l);
        std::string unit(u.substr(l, r == std::string::npos ? r : r - l));
        if (!unit.empty()) {
          if (nominator) numerators.push_back(unit);
          else denominators.push_back(unit);
        }
        if (r == std::string::npos) break;
        // ToDo: should error for multiple slashes
        // if (!nominator && u[r] == '/') error(...)
        if (u[r] == '/')
          nominator = false;
        // strange math parsing?
        // else if (u[r] == '*')
        //  nominator = true;
        l = r + 1;
      }
    }
    concrete_type(NUMBER);
  }

  // cancel out unnecessary units
  void Number::reduce()
  {
    // apply conversion factor
    value_ *= this->Units::reduce();
  }

  void Number::normalize()
  {
    // apply conversion factor
    value_ *= this->Units::normalize();
  }

  bool Custom_Warning::operator== (const Expression& rhs) const
  {
    if (Custom_Warning_Ptr_Const r = Cast<Custom_Warning>(&rhs)) {
      return message() == r->message();
    }
    return false;
  }

  bool Custom_Error::operator== (const Expression& rhs) const
  {
    if (Custom_Error_Ptr_Const r = Cast<Custom_Error>(&rhs)) {
      return message() == r->message();
    }
    return false;
  }

  bool Number::operator== (const Expression& rhs) const
  {
    if (auto rhsnr = Cast<Number>(&rhs)) {
      return *this == *rhsnr;
    }
    return false;
  }

  bool Number::operator== (const Number& rhs) const
  {
    Number l(*this), r(rhs); l.reduce(); r.reduce();
    size_t lhs_units = l.numerators.size() + l.denominators.size();
    size_t rhs_units = r.numerators.size() + r.denominators.size();
    // unitless and only having one unit seems equivalent (will change in future)
    if (!lhs_units || !rhs_units) {
      return NEAR_EQUAL(l.value(), r.value());
    }
    l.normalize(); r.normalize();
    Units &lhs_unit = l, &rhs_unit = r;
    return lhs_unit == rhs_unit &&
      NEAR_EQUAL(l.value(), r.value());
  }

  bool Number::operator< (const Number& rhs) const
  {
    Number l(*this), r(rhs); l.reduce(); r.reduce();
    size_t lhs_units = l.numerators.size() + l.denominators.size();
    size_t rhs_units = r.numerators.size() + r.denominators.size();
    // unitless and only having one unit seems equivalent (will change in future)
    if (!lhs_units || !rhs_units) {
      return l.value() < r.value();
    }
    l.normalize(); r.normalize();
    Units &lhs_unit = l, &rhs_unit = r;
    if (!(lhs_unit == rhs_unit)) {
      /* ToDo: do we always get usefull backtraces? */
      throw Exception::IncompatibleUnits(rhs, *this);
    }
    return lhs_unit < rhs_unit ||
           l.value() < r.value();
  }

  bool String_Quoted::operator== (const Expression& rhs) const
  {
    if (String_Quoted_Ptr_Const qstr = Cast<String_Quoted>(&rhs)) {
      return (value() == qstr->value());
    } else if (String_Constant_Ptr_Const cstr = Cast<String_Constant>(&rhs)) {
      return (value() == cstr->value());
    }
    return false;
  }

  bool String_Constant::is_invisible() const {
    return value_.empty() && quote_mark_ == 0;
  }

  bool String_Constant::operator== (const Expression& rhs) const
  {
    if (String_Quoted_Ptr_Const qstr = Cast<String_Quoted>(&rhs)) {
      return (value() == qstr->value());
    } else if (String_Constant_Ptr_Const cstr = Cast<String_Constant>(&rhs)) {
      return (value() == cstr->value());
    }
    return false;
  }

  bool String_Schema::is_left_interpolant(void) const
  {
    return length() && first()->is_left_interpolant();
  }
  bool String_Schema::is_right_interpolant(void) const
  {
    return length() && last()->is_right_interpolant();
  }

  bool String_Schema::operator== (const Expression& rhs) const
  {
    if (String_Schema_Ptr_Const r = Cast<String_Schema>(&rhs)) {
      if (length() != r->length()) return false;
      for (size_t i = 0, L = length(); i < L; ++i) {
        Expression_Obj rv = (*r)[i];
        Expression_Obj lv = (*this)[i];
        if (!lv || !rv) return false;
        if (!(*lv == *rv)) return false;
      }
      return true;
    }
    return false;
  }

  bool Boolean::operator== (const Expression& rhs) const
  {
    if (Boolean_Ptr_Const r = Cast<Boolean>(&rhs)) {
      return (value() == r->value());
    }
    return false;
  }

  bool Color::operator== (const Expression& rhs) const
  {
    if (Color_Ptr_Const r = Cast<Color>(&rhs)) {
      return r_ == r->r() &&
             g_ == r->g() &&
             b_ == r->b() &&
             a_ == r->a();
    }
    return false;
  }

  bool List::operator== (const Expression& rhs) const
  {
    if (List_Ptr_Const r = Cast<List>(&rhs)) {
      if (length() != r->length()) return false;
      if (separator() != r->separator()) return false;
      if (is_bracketed() != r->is_bracketed()) return false;
      for (size_t i = 0, L = length(); i < L; ++i) {
        Expression_Obj rv = r->at(i);
        Expression_Obj lv = this->at(i);
        if (!lv || !rv) return false;
        if (!(*lv == *rv)) return false;
      }
      return true;
    }
    return false;
  }

  bool Map::operator== (const Expression& rhs) const
  {
    if (Map_Ptr_Const r = Cast<Map>(&rhs)) {
      if (length() != r->length()) return false;
      for (auto key : keys()) {
        Expression_Obj lv = at(key);
        Expression_Obj rv = r->at(key);
        if (!rv || !lv) return false;
        if (!(*lv == *rv)) return false;
      }
      return true;
    }
    return false;
  }

  bool Null::operator== (const Expression& rhs) const
  {
    return rhs.concrete_type() == NULL_VAL;
  }

  bool Function::operator== (const Expression& rhs) const
  {
    if (Function_Ptr_Const r = Cast<Function>(&rhs)) {
      Definition_Ptr_Const d1 = Cast<Definition>(definition());
      Definition_Ptr_Const d2 = Cast<Definition>(r->definition());
      return d1 && d2 && d1 == d2 && is_css() == r->is_css();
    }
    return false;
  }

  size_t List::size() const {
    if (!is_arglist_) return length();
    // arglist expects a list of arguments
    // so we need to break before keywords
    for (size_t i = 0, L = length(); i < L; ++i) {
      Expression_Obj obj = this->at(i);
      if (Argument_Ptr arg = Cast<Argument>(obj)) {
        if (!arg->name().empty()) return i;
      }
    }
    return length();
  }

  std::string String_Quoted::inspect() const
  {
    return quote(value_, '*');
  }

  std::string String_Constant::inspect() const
  {
    return quote(value_, '*');
  }

  Function_Call::Function_Call(ParserState pstate, std::string n, Arguments_Obj args, void* cookie)
  : PreValue(pstate), sname_(SASS_MEMORY_NEW(String_Constant, pstate, n)), arguments_(args), func_(), via_call_(false), cookie_(cookie), hash_(0)
  { concrete_type(FUNCTION); }
  Function_Call::Function_Call(ParserState pstate, std::string n, Arguments_Obj args, Function_Obj func)
  : PreValue(pstate), sname_(SASS_MEMORY_NEW(String_Constant, pstate, n)), arguments_(args), func_(func), via_call_(false), cookie_(0), hash_(0)
  { concrete_type(FUNCTION); }
  Function_Call::Function_Call(ParserState pstate, std::string n, Arguments_Obj args)
  : PreValue(pstate), sname_(SASS_MEMORY_NEW(String_Constant, pstate, n)), arguments_(args), via_call_(false), cookie_(0), hash_(0)
  { concrete_type(FUNCTION); }

  bool Function_Call::operator==(const Expression& rhs) const
  {
    try
    {
      Function_Call_Ptr_Const m = Cast<Function_Call>(&rhs);
      if (!(m && *sname() == *m->sname())) return false;
      if (!(m && arguments()->length() == m->arguments()->length())) return false;
      for (size_t i =0, L = arguments()->length(); i < L; ++i)
        if (!(*(*arguments())[i] == *(*m->arguments())[i])) return false;
      return true;
    }
    catch (std::bad_cast&)
    {
      return false;
    }
    catch (...) { throw; }
  }

  size_t Function_Call::hash() const
  {
    if (hash_ == 0) {
      hash_ = std::hash<std::string>()(name());
      for (auto argument : arguments()->elements())
        hash_combine(hash_, argument->hash());
    }
    return hash_;
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // Convert map to (key, value) list.
  //////////////////////////////////////////////////////////////////////////////////////////
  List_Obj Map::to_list(ParserState& pstate) {
    List_Obj ret = SASS_MEMORY_NEW(List, pstate, length(), SASS_COMMA);

    for (auto key : keys()) {
      List_Obj l = SASS_MEMORY_NEW(List, pstate, 2);
      l->append(key);
      l->append(at(key));
      ret->append(l);
    }

    return ret;
  }

  IMPLEMENT_AST_OPERATORS(List);
  IMPLEMENT_AST_OPERATORS(Map);
  IMPLEMENT_AST_OPERATORS(Binary_Expression);
  IMPLEMENT_AST_OPERATORS(Function);
  IMPLEMENT_AST_OPERATORS(Function_Call);
  IMPLEMENT_AST_OPERATORS(Variable);
  IMPLEMENT_AST_OPERATORS(Number);
  IMPLEMENT_AST_OPERATORS(Color);
  IMPLEMENT_AST_OPERATORS(Custom_Error);
  IMPLEMENT_AST_OPERATORS(Custom_Warning);
  IMPLEMENT_AST_OPERATORS(Boolean);
  IMPLEMENT_AST_OPERATORS(String_Schema);
  IMPLEMENT_AST_OPERATORS(String_Constant);
  IMPLEMENT_AST_OPERATORS(String_Quoted);
  IMPLEMENT_AST_OPERATORS(Null);
  IMPLEMENT_AST_OPERATORS(Parent_Reference);

}