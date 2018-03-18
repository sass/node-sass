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

namespace Sass {

  static Null sass_null(ParserState("null"));

  bool Supports_Operator::needs_parens(Supports_Condition_Obj cond) const {
    if (Supports_Operator_Obj op = Cast<Supports_Operator>(cond)) {
      return op->operand() != operand();
    }
    return Cast<Supports_Negation>(cond) != NULL;
  }

  bool Supports_Negation::needs_parens(Supports_Condition_Obj cond) const {
    return Cast<Supports_Negation>(cond) ||
           Cast<Supports_Operator>(cond);
  }

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

  void Argument::set_delayed(bool delayed)
  {
    if (value_) value_->set_delayed(delayed);
    is_delayed(delayed);
  }

  void Arguments::set_delayed(bool delayed)
  {
    for (Argument_Obj arg : elements()) {
      if (arg) arg->set_delayed(delayed);
    }
    is_delayed(delayed);
  }


  bool At_Root_Query::exclude(std::string str)
  {
    bool with = feature() && unquote(feature()->to_string()).compare("with") == 0;
    List_Ptr l = static_cast<List_Ptr>(value().ptr());
    std::string v;

    if (with)
    {
      if (!l || l->length() == 0) return str.compare("rule") != 0;
      for (size_t i = 0, L = l->length(); i < L; ++i)
      {
        v = unquote((*l)[i]->to_string());
        if (v.compare("all") == 0 || v == str) return false;
      }
      return true;
    }
    else
    {
      if (!l || !l->length()) return str.compare("rule") == 0;
      for (size_t i = 0, L = l->length(); i < L; ++i)
      {
        v = unquote((*l)[i]->to_string());
        if (v.compare("all") == 0 || v == str) return true;
      }
      return false;
    }
  }

  void AST_Node::update_pstate(const ParserState& pstate)
  {
    pstate_.offset += pstate - pstate_ + pstate.offset;
  }

  Argument_Obj Arguments::get_rest_argument()
  {
    if (this->has_rest_argument()) {
      for (Argument_Obj arg : this->elements()) {
        if (arg->is_rest_argument()) {
          return arg;
        }
      }
    }
    return {};
  }

  Argument_Obj Arguments::get_keyword_argument()
  {
    if (this->has_keyword_argument()) {
      for (Argument_Obj arg : this->elements()) {
        if (arg->is_keyword_argument()) {
          return arg;
        }
      }
    }
    return {};
  }

  void Arguments::adjust_after_pushing(Argument_Obj a)
  {
    if (!a->name().empty()) {
      if (has_keyword_argument()) {
        coreError("named arguments must precede variable-length argument", a->pstate());
      }
      has_named_arguments(true);
    }
    else if (a->is_rest_argument()) {
      if (has_rest_argument()) {
        coreError("functions and mixins may only be called with one variable-length argument", a->pstate());
      }
      if (has_keyword_argument_) {
        coreError("only keyword arguments may follow variable arguments", a->pstate());
      }
      has_rest_argument(true);
    }
    else if (a->is_keyword_argument()) {
      if (has_keyword_argument()) {
        coreError("functions and mixins may only be called with one keyword argument", a->pstate());
      }
      has_keyword_argument(true);
    }
    else {
      if (has_rest_argument()) {
        coreError("ordinal arguments must precede variable-length arguments", a->pstate());
      }
      if (has_named_arguments()) {
        coreError("ordinal arguments must precede named arguments", a->pstate());
      }
    }
  }

  bool Ruleset::is_invisible() const {
    if (Selector_List_Ptr sl = Cast<Selector_List>(selector())) {
      for (size_t i = 0, L = sl->length(); i < L; ++i)
        if (!(*sl)[i]->has_placeholder()) return false;
    }
    return true;
  }

  bool Media_Block::is_invisible() const {
    for (size_t i = 0, L = block()->length(); i < L; ++i) {
      Statement_Obj stm = block()->at(i);
      if (!stm->is_invisible()) return false;
    }
    return true;
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

  Expression_Obj Hashed::at(Expression_Obj k) const
  {
    if (elements_.count(k))
    { return elements_.at(k); }
    else { return {}; }
  }

  bool Binary_Expression::is_left_interpolant(void) const
  {
    return is_interpolant() || (left() && left()->is_left_interpolant());
  }
  bool Binary_Expression::is_right_interpolant(void) const
  {
    return is_interpolant() || (right() && right()->is_right_interpolant());
  }

  const std::string AST_Node::to_string(Sass_Inspect_Options opt) const
  {
    Sass_Output_Options out(opt);
    Emitter emitter(out);
    Inspect i(emitter);
    i.in_declaration = true;
    // ToDo: inspect should be const
    const_cast<AST_Node_Ptr>(this)->perform(&i);
    return i.get_buffer();
  }

  const std::string AST_Node::to_string() const
  {
    return to_string({ NESTED, 5 });
  }

  std::string String_Quoted::inspect() const
  {
    return quote(value_, '*');
  }

  std::string String_Constant::inspect() const
  {
    return quote(value_, '*');
  }

  bool Declaration::is_invisible() const
  {
    if (is_custom_property()) return false;

    return !(value_ && value_->concrete_type() != Expression::NULL_VAL);
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // Additional method on Lists to retrieve values directly or from an encompassed Argument.
  //////////////////////////////////////////////////////////////////////////////////////////
  Expression_Obj List::value_at_index(size_t i) {
    Expression_Obj obj = this->at(i);
    if (is_arglist_) {
      if (Argument_Ptr arg = Cast<Argument>(obj)) {
        return arg->value();
      } else {
        return obj;
      }
    } else {
      return obj;
    }
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

  IMPLEMENT_AST_OPERATORS(Supports_Operator);
  IMPLEMENT_AST_OPERATORS(Supports_Negation);
  IMPLEMENT_AST_OPERATORS(Ruleset);
  IMPLEMENT_AST_OPERATORS(Media_Block);
  IMPLEMENT_AST_OPERATORS(Custom_Warning);
  IMPLEMENT_AST_OPERATORS(Custom_Error);
  IMPLEMENT_AST_OPERATORS(List);
  IMPLEMENT_AST_OPERATORS(Map);
  IMPLEMENT_AST_OPERATORS(Function);
  IMPLEMENT_AST_OPERATORS(Number);
  IMPLEMENT_AST_OPERATORS(Binary_Expression);
  IMPLEMENT_AST_OPERATORS(String_Schema);
  IMPLEMENT_AST_OPERATORS(String_Constant);
  IMPLEMENT_AST_OPERATORS(String_Quoted);
  IMPLEMENT_AST_OPERATORS(Boolean);
  IMPLEMENT_AST_OPERATORS(Color);
  IMPLEMENT_AST_OPERATORS(Null);
  IMPLEMENT_AST_OPERATORS(Parent_Reference);
  IMPLEMENT_AST_OPERATORS(Import);
  IMPLEMENT_AST_OPERATORS(Import_Stub);
  IMPLEMENT_AST_OPERATORS(Function_Call);
  IMPLEMENT_AST_OPERATORS(Directive);
  IMPLEMENT_AST_OPERATORS(At_Root_Block);
  IMPLEMENT_AST_OPERATORS(Supports_Block);
  IMPLEMENT_AST_OPERATORS(While);
  IMPLEMENT_AST_OPERATORS(Each);
  IMPLEMENT_AST_OPERATORS(For);
  IMPLEMENT_AST_OPERATORS(If);
  IMPLEMENT_AST_OPERATORS(Mixin_Call);
  IMPLEMENT_AST_OPERATORS(Extension);
  IMPLEMENT_AST_OPERATORS(Media_Query);
  IMPLEMENT_AST_OPERATORS(Media_Query_Expression);
  IMPLEMENT_AST_OPERATORS(Debug);
  IMPLEMENT_AST_OPERATORS(Error);
  IMPLEMENT_AST_OPERATORS(Warning);
  IMPLEMENT_AST_OPERATORS(Assignment);
  IMPLEMENT_AST_OPERATORS(Return);
  IMPLEMENT_AST_OPERATORS(At_Root_Query);
  IMPLEMENT_AST_OPERATORS(Variable);
  IMPLEMENT_AST_OPERATORS(Comment);
  IMPLEMENT_AST_OPERATORS(Supports_Interpolation);
  IMPLEMENT_AST_OPERATORS(Supports_Declaration);
  IMPLEMENT_AST_OPERATORS(Supports_Condition);
  IMPLEMENT_AST_OPERATORS(Parameters);
  IMPLEMENT_AST_OPERATORS(Parameter);
  IMPLEMENT_AST_OPERATORS(Arguments);
  IMPLEMENT_AST_OPERATORS(Argument);
  IMPLEMENT_AST_OPERATORS(Unary_Expression);
  IMPLEMENT_AST_OPERATORS(Block);
  IMPLEMENT_AST_OPERATORS(Content);
  IMPLEMENT_AST_OPERATORS(Trace);
  IMPLEMENT_AST_OPERATORS(Keyframe_Rule);
  IMPLEMENT_AST_OPERATORS(Bubble);
  IMPLEMENT_AST_OPERATORS(Definition);
  IMPLEMENT_AST_OPERATORS(Declaration);
}
