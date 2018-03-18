#ifndef SASS_AST_VALUES_H
#define SASS_AST_VALUES_H

#include "sass.hpp"
#include <set>
#include <deque>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include "sass/base.h"
#include "ast_fwd_decl.hpp"

#include "util.hpp"
#include "units.hpp"
#include "context.hpp"
#include "position.hpp"
#include "constants.hpp"
#include "operation.hpp"
#include "position.hpp"
#include "inspect.hpp"
#include "source_map.hpp"
#include "environment.hpp"
#include "error_handling.hpp"
#include "ast_def_macros.hpp"
#include "ast_fwd_decl.hpp"
#include "source_map.hpp"
#include "fn_utils.hpp"

#include "sass.h"

namespace Sass {

  //////////////////////////////////////////////////////////////////////
  // Still just an expression, but with a to_string method
  //////////////////////////////////////////////////////////////////////
  class PreValue : public Expression {
  public:
    PreValue(ParserState pstate,
               bool d = false, bool e = false, bool i = false, Concrete_Type ct = NONE)
    : Expression(pstate, d, e, i, ct)
    { }
    PreValue(const PreValue* ptr)
    : Expression(ptr)
    { }
    ATTACH_VIRTUAL_AST_OPERATIONS(PreValue);
    virtual ~PreValue() { }
  };

  //////////////////////////////////////////////////////////////////////
  // base class for values that support operations
  //////////////////////////////////////////////////////////////////////
  class Value : public PreValue {
  public:
    Value(ParserState pstate,
          bool d = false, bool e = false, bool i = false, Concrete_Type ct = NONE)
    : PreValue(pstate, d, e, i, ct)
    { }
    Value(const Value* ptr)
    : PreValue(ptr)
    { }
    ATTACH_VIRTUAL_AST_OPERATIONS(Value);
    virtual bool operator== (const Expression& rhs) const = 0;
  };

  ///////////////////////////////////////////////////////////////////////
  // Lists of values, both comma- and space-separated (distinguished by a
  // type-tag.) Also used to represent variable-length argument lists.
  ///////////////////////////////////////////////////////////////////////
  class List : public Value, public Vectorized<Expression_Obj> {
    void adjust_after_pushing(Expression_Obj e) { is_expanded(false); }
  private:
    ADD_PROPERTY(enum Sass_Separator, separator)
    ADD_PROPERTY(bool, is_arglist)
    ADD_PROPERTY(bool, is_bracketed)
    ADD_PROPERTY(bool, from_selector)
  public:
    List(ParserState pstate,
         size_t size = 0, enum Sass_Separator sep = SASS_SPACE, bool argl = false, bool bracket = false)
    : Value(pstate),
      Vectorized<Expression_Obj>(size),
      separator_(sep),
      is_arglist_(argl),
      is_bracketed_(bracket),
      from_selector_(false)
    { concrete_type(LIST); }
    List(const List* ptr)
    : Value(ptr),
      Vectorized<Expression_Obj>(*ptr),
      separator_(ptr->separator_),
      is_arglist_(ptr->is_arglist_),
      is_bracketed_(ptr->is_bracketed_),
      from_selector_(ptr->from_selector_)
    { concrete_type(LIST); }
    std::string type() const { return is_arglist_ ? "arglist" : "list"; }
    static std::string type_name() { return "list"; }
    const char* sep_string(bool compressed = false) const {
      return separator() == SASS_SPACE ?
        " " : (compressed ? "," : ", ");
    }
    bool is_invisible() const { return empty() && !is_bracketed(); }
    Expression_Obj value_at_index(size_t i);

    virtual size_t size() const;

    virtual size_t hash()
    {
      if (hash_ == 0) {
        hash_ = std::hash<std::string>()(sep_string());
        hash_combine(hash_, std::hash<bool>()(is_bracketed()));
        for (size_t i = 0, L = length(); i < L; ++i)
          hash_combine(hash_, (elements()[i])->hash());
      }
      return hash_;
    }

    virtual void set_delayed(bool delayed)
    {
      is_delayed(delayed);
      // don't set children
    }

    virtual bool operator== (const Expression& rhs) const;

    ATTACH_AST_OPERATIONS(List)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////////////////////////////////
  // Key value paris.
  ///////////////////////////////////////////////////////////////////////
  class Map : public Value, public Hashed {
    void adjust_after_pushing(std::pair<Expression_Obj, Expression_Obj> p) { is_expanded(false); }
  public:
    Map(ParserState pstate,
         size_t size = 0)
    : Value(pstate),
      Hashed(size)
    { concrete_type(MAP); }
    Map(const Map* ptr)
    : Value(ptr),
      Hashed(*ptr)
    { concrete_type(MAP); }
    std::string type() const { return "map"; }
    static std::string type_name() { return "map"; }
    bool is_invisible() const { return empty(); }
    List_Obj to_list(ParserState& pstate);

    virtual size_t hash()
    {
      if (hash_ == 0) {
        for (auto key : keys()) {
          hash_combine(hash_, key->hash());
          hash_combine(hash_, at(key)->hash());
        }
      }

      return hash_;
    }

    virtual bool operator== (const Expression& rhs) const;

    ATTACH_AST_OPERATIONS(Map)
    ATTACH_CRTP_PERFORM_METHODS()
  };


  //////////////////////////////////////////////////////////////////////////
  // Binary expressions. Represents logical, relational, and arithmetic
  // operations. Templatized to avoid large switch statements and repetitive
  // subclassing.
  //////////////////////////////////////////////////////////////////////////
  class Binary_Expression : public PreValue {
  private:
    HASH_PROPERTY(Operand, op)
    HASH_PROPERTY(Expression_Obj, left)
    HASH_PROPERTY(Expression_Obj, right)
    size_t hash_;
  public:
    Binary_Expression(ParserState pstate,
                      Operand op, Expression_Obj lhs, Expression_Obj rhs)
    : PreValue(pstate), op_(op), left_(lhs), right_(rhs), hash_(0)
    { }
    Binary_Expression(const Binary_Expression* ptr)
    : PreValue(ptr),
      op_(ptr->op_),
      left_(ptr->left_),
      right_(ptr->right_),
      hash_(ptr->hash_)
    { }
    const std::string type_name() {
      return sass_op_to_name(optype());
    }
    const std::string separator() {
      return sass_op_separator(optype());
    }
    bool is_left_interpolant(void) const;
    bool is_right_interpolant(void) const;
    bool has_interpolant() const
    {
      return is_left_interpolant() ||
             is_right_interpolant();
    }
    virtual void set_delayed(bool delayed)
    {
      right()->set_delayed(delayed);
      left()->set_delayed(delayed);
      is_delayed(delayed);
    }
    virtual bool operator==(const Expression& rhs) const
    {
      try
      {
        Binary_Expression_Ptr_Const m = Cast<Binary_Expression>(&rhs);
        if (m == 0) return false;
        return type() == m->type() &&
               *left() == *m->left() &&
               *right() == *m->right();
      }
      catch (std::bad_cast&)
      {
        return false;
      }
      catch (...) { throw; }
    }
    virtual size_t hash()
    {
      if (hash_ == 0) {
        hash_ = std::hash<size_t>()(optype());
        hash_combine(hash_, left()->hash());
        hash_combine(hash_, right()->hash());
      }
      return hash_;
    }
    enum Sass_OP optype() const { return op_.operand; }
    ATTACH_AST_OPERATIONS(Binary_Expression)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////
  // Function reference.
  ////////////////////////////////////////////////////
  class Function final : public Value {
  public:
    ADD_PROPERTY(Definition_Obj, definition)
    ADD_PROPERTY(bool, is_css)
  public:
    Function(ParserState pstate, Definition_Obj def, bool css)
    : Value(pstate), definition_(def), is_css_(css)
    { concrete_type(FUNCTION_VAL); }
    Function(const Function* ptr)
    : Value(ptr), definition_(ptr->definition_), is_css_(ptr->is_css_)
    { concrete_type(FUNCTION_VAL); }

    std::string type() const override { return "function"; }
    static std::string type_name() { return "function"; }
    bool is_invisible() const override { return true; }

    std::string name() {
      if (definition_) {
        return definition_->name();
      }
      return "";
    }

    bool operator== (const Expression& rhs) const override;

    ATTACH_AST_OPERATIONS(Function)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////
  // Function calls.
  //////////////////
  class Function_Call final : public PreValue {
    HASH_CONSTREF(String_Obj, sname)
    HASH_PROPERTY(Arguments_Obj, arguments)
    HASH_PROPERTY(Function_Obj, func)
    ADD_PROPERTY(bool, via_call)
    ADD_PROPERTY(void*, cookie)
    mutable size_t hash_;
  public:
    Function_Call(ParserState pstate, std::string n, Arguments_Obj args, void* cookie);
    Function_Call(ParserState pstate, std::string n, Arguments_Obj args, Function_Obj func);
    Function_Call(ParserState pstate, std::string n, Arguments_Obj args);

    Function_Call(ParserState pstate, String_Obj n, Arguments_Obj args, void* cookie)
    : PreValue(pstate), sname_(n), arguments_(args), func_(), via_call_(false), cookie_(cookie), hash_(0)
    { concrete_type(FUNCTION); }
    Function_Call(ParserState pstate, String_Obj n, Arguments_Obj args, Function_Obj func)
    : PreValue(pstate), sname_(n), arguments_(args), func_(func), via_call_(false), cookie_(0), hash_(0)
    { concrete_type(FUNCTION); }
    Function_Call(ParserState pstate, String_Obj n, Arguments_Obj args)
    : PreValue(pstate), sname_(n), arguments_(args), via_call_(false), cookie_(0), hash_(0)
    { concrete_type(FUNCTION); }

    std::string name() const {
      return sname();
    }

    Function_Call(const Function_Call* ptr)
    : PreValue(ptr),
      sname_(ptr->sname_),
      arguments_(ptr->arguments_),
      func_(ptr->func_),
      via_call_(ptr->via_call_),
      cookie_(ptr->cookie_),
      hash_(ptr->hash_)
    { concrete_type(FUNCTION); }

    bool is_css() {
      if (func_) return func_->is_css();
      return false;
    }

    bool operator==(const Expression& rhs) const override;

    size_t hash() const override;

    ATTACH_AST_OPERATIONS(Function_Call)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////
  // Variable references.
  ///////////////////////
  class Variable final : public PreValue {
    ADD_CONSTREF(std::string, name)
  public:
    Variable(ParserState pstate, std::string n)
    : PreValue(pstate), name_(n)
    { concrete_type(VARIABLE); }
    Variable(const Variable* ptr)
    : PreValue(ptr), name_(ptr->name_)
    { concrete_type(VARIABLE); }

    bool operator==(const Expression& rhs) const override
    {
      try
      {
        Variable_Ptr_Const e = Cast<Variable>(&rhs);
        return e && name() == e->name();
      }
      catch (std::bad_cast&)
      {
        return false;
      }
      catch (...) { throw; }
    }

    size_t hash() const override
    {
      return std::hash<std::string>()(name());
    }

    ATTACH_AST_OPERATIONS(Variable)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////
  // Numbers, percentages, dimensions, and colors.
  ////////////////////////////////////////////////
  class Number final : public Value, public Units {
    HASH_PROPERTY(double, value)
    ADD_PROPERTY(bool, zero)
    mutable size_t hash_;
  public:
    Number(ParserState pstate, double val, std::string u = "", bool zero = true);

    Number(const Number* ptr)
    : Value(ptr),
      Units(ptr),
      value_(ptr->value_), zero_(ptr->zero_),
      hash_(ptr->hash_)
    { concrete_type(NUMBER); }

    bool zero() { return zero_; }
    std::string type() const override { return "number"; }
    static std::string type_name() { return "number"; }

    void reduce();
    void normalize();

    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_ = std::hash<double>()(value_);
        for (const auto numerator : numerators)
          hash_combine(hash_, std::hash<std::string>()(numerator));
        for (const auto denominator : denominators)
          hash_combine(hash_, std::hash<std::string>()(denominator));
      }
      return hash_;
    }

    bool operator< (const Number& rhs) const;
    bool operator== (const Number& rhs) const;
    bool operator== (const Expression& rhs) const override;
    ATTACH_AST_OPERATIONS(Number)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////
  // Colors.
  //////////
  class Color final : public Value {
    HASH_PROPERTY(double, r)
    HASH_PROPERTY(double, g)
    HASH_PROPERTY(double, b)
    HASH_PROPERTY(double, a)
    ADD_CONSTREF(std::string, disp)
    mutable size_t hash_;
  public:
    Color(ParserState pstate, double r, double g, double b, double a = 1, const std::string disp = "")
    : Value(pstate), r_(r), g_(g), b_(b), a_(a), disp_(disp),
      hash_(0)
    { concrete_type(COLOR); }
    Color(const Color* ptr)
    : Value(ptr),
      r_(ptr->r_),
      g_(ptr->g_),
      b_(ptr->b_),
      a_(ptr->a_),
      disp_(ptr->disp_),
      hash_(ptr->hash_)
    { concrete_type(COLOR); }
    std::string type() const override { return "color"; }
    static std::string type_name() { return "color"; }

    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_ = std::hash<double>()(a_);
        hash_combine(hash_, std::hash<double>()(r_));
        hash_combine(hash_, std::hash<double>()(g_));
        hash_combine(hash_, std::hash<double>()(b_));
      }
      return hash_;
    }

    bool operator== (const Expression& rhs) const override;

    ATTACH_AST_OPERATIONS(Color)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////
  // Errors from Sass_Values.
  //////////////////////////////
  class Custom_Error final : public Value {
    ADD_CONSTREF(std::string, message)
  public:
    Custom_Error(ParserState pstate, std::string msg)
    : Value(pstate), message_(msg)
    { concrete_type(C_ERROR); }
    Custom_Error(const Custom_Error* ptr)
    : Value(ptr), message_(ptr->message_)
    { concrete_type(C_ERROR); }
    bool operator== (const Expression& rhs) const override;
    ATTACH_AST_OPERATIONS(Custom_Error)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////
  // Warnings from Sass_Values.
  //////////////////////////////
  class Custom_Warning final : public Value {
    ADD_CONSTREF(std::string, message)
  public:
    Custom_Warning(ParserState pstate, std::string msg)
    : Value(pstate), message_(msg)
    { concrete_type(C_WARNING); }
    Custom_Warning(const Custom_Warning* ptr)
    : Value(ptr), message_(ptr->message_)
    { concrete_type(C_WARNING); }
    bool operator== (const Expression& rhs) const override;
    ATTACH_AST_OPERATIONS(Custom_Warning)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////
  // Booleans.
  ////////////
  class Boolean final : public Value {
    HASH_PROPERTY(bool, value)
    mutable size_t hash_;
  public:
    Boolean(ParserState pstate, bool val)
    : Value(pstate), value_(val),
      hash_(0)
    { concrete_type(BOOLEAN); }
    Boolean(const Boolean* ptr)
    : Value(ptr),
      value_(ptr->value_),
      hash_(ptr->hash_)
    { concrete_type(BOOLEAN); }
    operator bool() override { return value_; }
    std::string type() const override { return "bool"; }
    static std::string type_name() { return "bool"; }
    bool is_false() override { return !value_; }

    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_ = std::hash<bool>()(value_);
      }
      return hash_;
    }

    bool operator== (const Expression& rhs) const override;

    ATTACH_AST_OPERATIONS(Boolean)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////
  // Abstract base class for Sass string values. Includes interpolated and
  // "flat" strings.
  ////////////////////////////////////////////////////////////////////////
  class String : public Value {
  public:
    String(ParserState pstate, bool delayed = false)
    : Value(pstate, delayed)
    { concrete_type(STRING); }
    String(const String* ptr)
    : Value(ptr)
    { concrete_type(STRING); }
    static std::string type_name() { return "string"; }
    virtual ~String() = 0;
    virtual void rtrim() = 0;
    virtual bool operator<(const Expression& rhs) const {
      return this->to_string() < rhs.to_string();
    };
    ATTACH_VIRTUAL_AST_OPERATIONS(String);
    ATTACH_CRTP_PERFORM_METHODS()
  };
  inline String::~String() { };

  ///////////////////////////////////////////////////////////////////////
  // Interpolated strings. Meant to be reduced to flat strings during the
  // evaluation phase.
  ///////////////////////////////////////////////////////////////////////
  class String_Schema final : public String, public Vectorized<PreValue_Obj> {
    ADD_PROPERTY(bool, css)
    mutable size_t hash_;
  public:
    String_Schema(ParserState pstate, size_t size = 0, bool css = true)
    : String(pstate), Vectorized<PreValue_Obj>(size), css_(css), hash_(0)
    { concrete_type(STRING); }
    String_Schema(const String_Schema* ptr)
    : String(ptr),
      Vectorized<PreValue_Obj>(*ptr),
      css_(ptr->css_),
      hash_(ptr->hash_)
    { concrete_type(STRING); }

    std::string type() const override { return "string"; }
    static std::string type_name() { return "string"; }

    bool is_left_interpolant(void) const override;
    bool is_right_interpolant(void) const override;
    // void has_interpolants(bool tc) { }
    bool has_interpolants() {
      for (auto el : elements()) {
        if (el->is_interpolant()) return true;
      }
      return false;
    }
    void rtrim() override;

    size_t hash() const override
    {
      if (hash_ == 0) {
        for (const auto &str : elements())
          hash_combine(hash_, str->hash());
      }
      return hash_;
    }

    void set_delayed(bool delayed) override {
      is_delayed(delayed);
    }

    bool operator==(const Expression& rhs) const override;
    ATTACH_AST_OPERATIONS(String_Schema)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////
  // Flat strings -- the lowest level of raw textual data.
  ////////////////////////////////////////////////////////
  class String_Constant : public String {
    ADD_PROPERTY(char, quote_mark)
    ADD_PROPERTY(bool, can_compress_whitespace)
    HASH_CONSTREF(std::string, value)
  protected:
    mutable size_t hash_;
  public:
    String_Constant(const String_Constant* ptr)
    : String(ptr),
      quote_mark_(ptr->quote_mark_),
      can_compress_whitespace_(ptr->can_compress_whitespace_),
      value_(ptr->value_),
      hash_(ptr->hash_)
    { }
    String_Constant(ParserState pstate, std::string val, bool css = true)
    : String(pstate), quote_mark_(0), can_compress_whitespace_(false), value_(read_css_string(val, css)), hash_(0)
    { }
    String_Constant(ParserState pstate, const char* beg, bool css = true)
    : String(pstate), quote_mark_(0), can_compress_whitespace_(false), value_(read_css_string(std::string(beg), css)), hash_(0)
    { }
    String_Constant(ParserState pstate, const char* beg, const char* end, bool css = true)
    : String(pstate), quote_mark_(0), can_compress_whitespace_(false), value_(read_css_string(std::string(beg, end-beg), css)), hash_(0)
    { }
    String_Constant(ParserState pstate, const Token& tok, bool css = true)
    : String(pstate), quote_mark_(0), can_compress_whitespace_(false), value_(read_css_string(std::string(tok.begin, tok.end), css)), hash_(0)
    { }
    std::string type() const override { return "string"; }
    static std::string type_name() { return "string"; }
    bool is_invisible() const override;
    virtual void rtrim() override;

    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_ = std::hash<std::string>()(value_);
      }
      return hash_;
    }

    bool operator==(const Expression& rhs) const override;
    virtual std::string inspect() const override; // quotes are forced on inspection

    // static char auto_quote() { return '*'; }
    static char double_quote() { return '"'; }
    static char single_quote() { return '\''; }

    ATTACH_AST_OPERATIONS(String_Constant)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////
  // Possibly quoted string (unquote on instantiation)
  ////////////////////////////////////////////////////////
  class String_Quoted final : public String_Constant {
  public:
    String_Quoted(ParserState pstate, std::string val, char q = 0,
      bool keep_utf8_escapes = false, bool skip_unquoting = false,
      bool strict_unquoting = true, bool css = true)
    : String_Constant(pstate, val, css)
    {
      if (skip_unquoting == false) {
        value_ = unquote(value_, &quote_mark_, keep_utf8_escapes, strict_unquoting);
      }
      if (q && quote_mark_) quote_mark_ = q;
    }
    String_Quoted(const String_Quoted* ptr)
    : String_Constant(ptr)
    { }
    bool operator==(const Expression& rhs) const override;
    std::string inspect() const override; // quotes are forced on inspection
    ATTACH_AST_OPERATIONS(String_Quoted)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////
  // The null value.
  //////////////////
  class Null final : public Value {
  public:
    Null(ParserState pstate) : Value(pstate) { concrete_type(NULL_VAL); }
    Null(const Null* ptr) : Value(ptr) { concrete_type(NULL_VAL); }
    std::string type() const override { return "null"; }
    static std::string type_name() { return "null"; }
    bool is_invisible() const override { return true; }
    operator bool() override { return false; }
    bool is_false() override { return true; }

    size_t hash() const override
    {
      return -1;
    }

    bool operator== (const Expression& rhs) const override;

    ATTACH_AST_OPERATIONS(Null)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////////
  // The Parent Reference Expression.
  //////////////////////////////////
  class Parent_Reference final : public Value {
  public:
    Parent_Reference(ParserState pstate)
    : Value(pstate) {}
    Parent_Reference(const Parent_Reference* ptr)
    : Value(ptr) {}
    std::string type() const override { return "parent"; }
    static std::string type_name() { return "parent"; }
    bool operator==(const Expression& rhs) const override {
      return true; // can they ever be not equal?
    };
    ATTACH_AST_OPERATIONS(Parent_Reference)
    ATTACH_CRTP_PERFORM_METHODS()
  };

}

#endif