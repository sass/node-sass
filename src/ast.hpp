#ifndef SASS_AST_H
#define SASS_AST_H

// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
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

  // easier to search with name
  const bool DELAYED = true;

  // ToDo: should this really be hardcoded
  // Note: most methods follow precision option
  const double NUMBER_EPSILON = 1e-12;

  // macro to test if numbers are equal within a small error margin
  #define NEAR_EQUAL(lhs, rhs) std::fabs(lhs - rhs) < NUMBER_EPSILON

  // ToDo: where does this fit best?
  // We don't share this with C-API?
  class Operand {
    public:
      Operand(Sass_OP operand, bool ws_before = false, bool ws_after = false)
      : operand(operand), ws_before(ws_before), ws_after(ws_after)
      { }
    public:
      enum Sass_OP operand;
      bool ws_before;
      bool ws_after;
  };

  //////////////////////////////////////////////////////////
  // `hash_combine` comes from boost (functional/hash):
  // http://www.boost.org/doc/libs/1_35_0/doc/html/hash/combine.html
  // Boost Software License - Version 1.0
  // http://www.boost.org/users/license.html
  template <typename T>
  void hash_combine (std::size_t& seed, const T& val)
  {
    seed ^= std::hash<T>()(val) + 0x9e3779b9
             + (seed<<6) + (seed>>2);
  }
  //////////////////////////////////////////////////////////

  const char* sass_op_to_name(enum Sass_OP op);

  const char* sass_op_separator(enum Sass_OP op);

  //////////////////////////////////////////////////////////
  // Abstract base class for all abstract syntax tree nodes.
  //////////////////////////////////////////////////////////
  class AST_Node : public SharedObj {
    ADD_PROPERTY(ParserState, pstate)
  public:
    AST_Node(ParserState pstate)
    : pstate_(pstate)
    { }
    AST_Node(const AST_Node* ptr)
    : pstate_(ptr->pstate_)
    { }

    // allow implicit conversion to string
    // needed for by SharedPtr implementation
    operator std::string() {
      return to_string();
    }

    // AST_Node(AST_Node& ptr) = delete;

    virtual ~AST_Node() = 0;
    virtual size_t hash() const { return 0; }
    virtual std::string inspect() const { return to_string({ INSPECT, 5 }); }
    virtual std::string to_sass() const { return to_string({ TO_SASS, 5 }); }
    virtual const std::string to_string(Sass_Inspect_Options opt) const;
    virtual const std::string to_string() const;
    virtual void cloneChildren() {};
    // generic find function (not fully implemented yet)
    // ToDo: add specific implementions to all children
    virtual bool find ( bool (*f)(AST_Node_Obj) ) { return f(this); };
    void update_pstate(const ParserState& pstate);
    Offset off() { return pstate(); }
    Position pos() { return pstate(); }
    ATTACH_ABSTRACT_AST_OPERATIONS(AST_Node);
    ATTACH_ABSTRACT_CRTP_PERFORM_METHODS()
  };
  inline AST_Node::~AST_Node() { }

  //////////////////////////////////////////////////////////////////////
  // define cast template now (need complete type)
  //////////////////////////////////////////////////////////////////////

  template<class T>
  T* Cast(AST_Node* ptr) {
    return ptr && typeid(T) == typeid(*ptr) ?
           static_cast<T*>(ptr) : NULL;
  };

  template<class T>
  const T* Cast(const AST_Node* ptr) {
    return ptr && typeid(T) == typeid(*ptr) ?
           static_cast<const T*>(ptr) : NULL;
  };

  //////////////////////////////////////////////////////////////////////
  // Abstract base class for expressions. This side of the AST hierarchy
  // represents elements in value contexts, which exist primarily to be
  // evaluated and returned.
  //////////////////////////////////////////////////////////////////////
  class Expression : public AST_Node {
  public:
    enum Type {
      NONE,
      BOOLEAN,
      NUMBER,
      COLOR,
      STRING,
      LIST,
      MAP,
      SELECTOR,
      NULL_VAL,
      FUNCTION_VAL,
      C_WARNING,
      C_ERROR,
      FUNCTION,
      VARIABLE,
      PARENT,
      NUM_TYPES
    };
  private:
    // expressions in some contexts shouldn't be evaluated
    ADD_PROPERTY(bool, is_delayed)
    ADD_PROPERTY(bool, is_expanded)
    ADD_PROPERTY(bool, is_interpolant)
    ADD_PROPERTY(Type, concrete_type)
  public:
    Expression(ParserState pstate, bool d = false, bool e = false, bool i = false, Type ct = NONE);
    virtual operator bool() { return true; }
    virtual ~Expression() { }
    virtual bool is_invisible() const { return false; }

    virtual std::string type() const { return ""; }
    static std::string type_name() { return ""; }

    virtual bool is_false() { return false; }
    // virtual bool is_true() { return !is_false(); }
    virtual bool operator< (const Expression& rhs) const { return false; }
    virtual bool operator== (const Expression& rhs) const { return false; }
    inline bool operator>(const Expression& rhs) const { return rhs < *this; }
    inline bool operator!=(const Expression& rhs) const { return !(rhs == *this); }
    virtual bool eq(const Expression& rhs) const { return *this == rhs; };
    virtual void set_delayed(bool delayed) { is_delayed(delayed); }
    virtual bool has_interpolant() const { return is_interpolant(); }
    virtual bool is_left_interpolant() const { return is_interpolant(); }
    virtual bool is_right_interpolant() const { return is_interpolant(); }
    ATTACH_VIRTUAL_AST_OPERATIONS(Expression);
    size_t hash() const override { return 0; }
  };

}

/////////////////////////////////////////////////////////////////////////////////////
// Hash method specializations for std::unordered_map to work with Sass::Expression
/////////////////////////////////////////////////////////////////////////////////////

namespace std {
  template<>
  struct hash<Sass::Expression_Obj>
  {
    size_t operator()(Sass::Expression_Obj s) const
    {
      return s->hash();
    }
  };
  template<>
  struct equal_to<Sass::Expression_Obj>
  {
    bool operator()( Sass::Expression_Obj lhs,  Sass::Expression_Obj rhs) const
    {
      return lhs->hash() == rhs->hash();
    }
  };
}

namespace Sass {

  /////////////////////////////////////////////////////////////////////////////
  // Mixin class for AST nodes that should behave like vectors. Uses the
  // "Template Method" design pattern to allow subclasses to adjust their flags
  // when certain objects are pushed.
  /////////////////////////////////////////////////////////////////////////////
  template <typename T>
  class Vectorized {
    std::vector<T> elements_;
  protected:
    mutable size_t hash_;
    void reset_hash() { hash_ = 0; }
    virtual void adjust_after_pushing(T element) { }
  public:
    Vectorized(size_t s = 0) : hash_(0)
    { elements_.reserve(s); }
    virtual ~Vectorized() = 0;
    size_t length() const   { return elements_.size(); }
    bool empty() const      { return elements_.empty(); }
    void clear()            { return elements_.clear(); }
    T last() const          { return elements_.back(); }
    T first() const         { return elements_.front(); }
    T& operator[](size_t i) { return elements_[i]; }
    virtual const T& at(size_t i) const { return elements_.at(i); }
    virtual T& at(size_t i) { return elements_.at(i); }
    const T& get(size_t i) const { return elements_[i]; }
    const T& operator[](size_t i) const { return elements_[i]; }
    virtual void append(T element)
    {
      if (element) {
        reset_hash();
        elements_.push_back(element);
        adjust_after_pushing(element);
      }
    }
    virtual void concat(Vectorized* v)
    {
      for (size_t i = 0, L = v->length(); i < L; ++i) this->append((*v)[i]);
    }
    Vectorized& unshift(T element)
    {
      elements_.insert(elements_.begin(), element);
      return *this;
    }
    std::vector<T>& elements() { return elements_; }
    const std::vector<T>& elements() const { return elements_; }
    std::vector<T>& elements(std::vector<T>& e) { elements_ = e; return elements_; }

    virtual size_t hash() const
    {
      if (hash_ == 0) {
        for (const T& el : elements_) {
          hash_combine(hash_, el->hash());
        }
      }
      return hash_;
    }

    template <typename P, typename V>
    typename std::vector<T>::iterator insert(P position, const V& val) {
      reset_hash();
      return elements_.insert(position, val);
    }

    typename std::vector<T>::iterator end() { return elements_.end(); }
    typename std::vector<T>::iterator begin() { return elements_.begin(); }
    typename std::vector<T>::const_iterator end() const { return elements_.end(); }
    typename std::vector<T>::const_iterator begin() const { return elements_.begin(); }
    typename std::vector<T>::iterator erase(typename std::vector<T>::iterator el) { return elements_.erase(el); }
    typename std::vector<T>::const_iterator erase(typename std::vector<T>::const_iterator el) { return elements_.erase(el); }

  };
  template <typename T>
  inline Vectorized<T>::~Vectorized() { }

  /////////////////////////////////////////////////////////////////////////////
  // Mixin class for AST nodes that should behave like a hash table. Uses an
  // extra <std::vector> internally to maintain insertion order for interation.
  /////////////////////////////////////////////////////////////////////////////
  class Hashed {
  private:
    ExpressionMap elements_;
    std::vector<Expression_Obj> list_;
  protected:
    mutable size_t hash_;
    Expression_Obj duplicate_key_;
    void reset_hash() { hash_ = 0; }
    void reset_duplicate_key() { duplicate_key_ = {}; }
    virtual void adjust_after_pushing(std::pair<Expression_Obj, Expression_Obj> p) { }
  public:
    Hashed(size_t s = 0)
    : elements_(ExpressionMap(s)),
      list_(std::vector<Expression_Obj>()),
      hash_(0), duplicate_key_({})
    { elements_.reserve(s); list_.reserve(s); }
    virtual ~Hashed();
    size_t length() const                  { return list_.size(); }
    bool empty() const                     { return list_.empty(); }
    bool has(Expression_Obj k) const          { return elements_.count(k) == 1; }
    Expression_Obj at(Expression_Obj k) const;
    bool has_duplicate_key() const         { return duplicate_key_ != nullptr; }
    Expression_Obj get_duplicate_key() const  { return duplicate_key_; }
    const ExpressionMap elements() { return elements_; }
    Hashed& operator<<(std::pair<Expression_Obj, Expression_Obj> p)
    {
      reset_hash();

      if (!has(p.first)) list_.push_back(p.first);
      else if (!duplicate_key_) duplicate_key_ = p.first;

      elements_[p.first] = p.second;

      adjust_after_pushing(p);
      return *this;
    }
    Hashed& operator+=(Hashed* h)
    {
      if (length() == 0) {
        this->elements_ = h->elements_;
        this->list_ = h->list_;
        return *this;
      }

      for (auto key : h->keys()) {
        *this << std::make_pair(key, h->at(key));
      }

      reset_duplicate_key();
      return *this;
    }
    const ExpressionMap& pairs() const { return elements_; }
    const std::vector<Expression_Obj>& keys() const { return list_; }

//    std::unordered_map<Expression_Obj, Expression_Obj>::iterator end() { return elements_.end(); }
//    std::unordered_map<Expression_Obj, Expression_Obj>::iterator begin() { return elements_.begin(); }
//    std::unordered_map<Expression_Obj, Expression_Obj>::const_iterator end() const { return elements_.end(); }
//    std::unordered_map<Expression_Obj, Expression_Obj>::const_iterator begin() const { return elements_.begin(); }

  };
  inline Hashed::~Hashed() { }


  /////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements. This side of the AST hierarchy
  // represents elements in expansion contexts, which exist primarily to be
  // rewritten and macro-expanded.
  /////////////////////////////////////////////////////////////////////////
  class Statement : public AST_Node {
  public:
    enum Type {
      NONE,
      RULESET,
      MEDIA,
      DIRECTIVE,
      SUPPORTS,
      ATROOT,
      BUBBLE,
      CONTENT,
      KEYFRAMERULE,
      DECLARATION,
      ASSIGNMENT,
      IMPORT_STUB,
      IMPORT,
      COMMENT,
      WARNING,
      RETURN,
      EXTEND,
      ERROR,
      DEBUGSTMT,
      WHILE,
      EACH,
      FOR,
      IF
    };
  private:
    ADD_PROPERTY(Type, statement_type)
    ADD_PROPERTY(size_t, tabs)
    ADD_PROPERTY(bool, group_end)
  public:
    Statement(ParserState pstate, Type st = NONE, size_t t = 0);
    virtual ~Statement() = 0; // virtual destructor
    // needed for rearranging nested rulesets during CSS emission
    virtual bool bubbles();
    virtual bool has_content();
    virtual bool is_invisible() const;
    ATTACH_VIRTUAL_AST_OPERATIONS(Statement)
  };
  inline Statement::~Statement() { }

  ////////////////////////
  // Blocks of statements.
  ////////////////////////
  class Block final : public Statement, public Vectorized<Statement_Obj> {
    ADD_PROPERTY(bool, is_root)
    // needed for properly formatted CSS emission
  protected:
    void adjust_after_pushing(Statement_Obj s) override {}
  public:
    Block(ParserState pstate, size_t s = 0, bool r = false);
    bool has_content() override;
    ATTACH_AST_OPERATIONS(Block)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements that contain blocks of statements.
  ////////////////////////////////////////////////////////////////////////
  class Has_Block : public Statement {
    ADD_PROPERTY(Block_Obj, block)
  public:
    Has_Block(ParserState pstate, Block_Obj b);
    Has_Block(const Has_Block* ptr); // copy constructor
    virtual ~Has_Block() = 0; // virtual destructor
    virtual bool has_content() override;
  };
  inline Has_Block::~Has_Block() { }

  /////////////////////////////////////////////////////////////////////////////
  // Rulesets (i.e., sets of styles headed by a selector and containing a block
  // of style declarations.
  /////////////////////////////////////////////////////////////////////////////
  class Ruleset final : public Has_Block {
    ADD_PROPERTY(Selector_List_Obj, selector)
    ADD_PROPERTY(bool, is_root);
  public:
    Ruleset(ParserState pstate, Selector_List_Obj s = {}, Block_Obj b = {});
    bool is_invisible() const override;
    ATTACH_AST_OPERATIONS(Ruleset)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////
  // Bubble.
  /////////////////
  class Bubble final : public Statement {
    ADD_PROPERTY(Statement_Obj, node)
    ADD_PROPERTY(bool, group_end)
  public:
    Bubble(ParserState pstate, Statement_Obj n, Statement_Obj g = {}, size_t t = 0);
    bool bubbles() override;
    ATTACH_AST_OPERATIONS(Bubble)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////
  // Trace.
  /////////////////
  class Trace final : public Has_Block {
    ADD_CONSTREF(char, type)
    ADD_CONSTREF(std::string, name)
  public:
    Trace(ParserState pstate, std::string n, Block_Obj b = {}, char type = 'm');
    ATTACH_AST_OPERATIONS(Trace)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////
  // Media queries.
  /////////////////
  class Media_Block final : public Has_Block {
    ADD_PROPERTY(List_Obj, media_queries)
  public:
    Media_Block(ParserState pstate, List_Obj mqs, Block_Obj b);
    bool bubbles() override;
    bool is_invisible() const override;
    ATTACH_AST_OPERATIONS(Media_Block)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////////////////////////////////
  // At-rules -- arbitrary directives beginning with "@" that may have an
  // optional statement block.
  ///////////////////////////////////////////////////////////////////////
  class Directive final : public Has_Block {
    ADD_CONSTREF(std::string, keyword)
    ADD_PROPERTY(Selector_List_Obj, selector)
    ADD_PROPERTY(Expression_Obj, value)
  public:
    Directive(ParserState pstate, std::string kwd, Selector_List_Obj sel = {}, Block_Obj b = {}, Expression_Obj val = {});
    bool bubbles() override;
    bool is_media();
    bool is_keyframes();
    ATTACH_AST_OPERATIONS(Directive)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////////////////////////////////
  // Keyframe-rules -- the child blocks of "@keyframes" nodes.
  ///////////////////////////////////////////////////////////////////////
  class Keyframe_Rule final : public Has_Block {
    // according to css spec, this should be <keyframes-name>
    // <keyframes-name> = <custom-ident> | <string>
    ADD_PROPERTY(Selector_List_Obj, name)
  public:
    Keyframe_Rule(ParserState pstate, Block_Obj b);
    ATTACH_AST_OPERATIONS(Keyframe_Rule)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////
  // Declarations -- style rules consisting of a property name and values.
  ////////////////////////////////////////////////////////////////////////
  class Declaration final : public Has_Block {
    ADD_PROPERTY(String_Obj, property)
    ADD_PROPERTY(Expression_Obj, value)
    ADD_PROPERTY(bool, is_important)
    ADD_PROPERTY(bool, is_custom_property)
    ADD_PROPERTY(bool, is_indented)
  public:
    Declaration(ParserState pstate, String_Obj prop, Expression_Obj val, bool i = false, bool c = false, Block_Obj b = {});
    bool is_invisible() const override;
    ATTACH_AST_OPERATIONS(Declaration)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////
  // Assignments -- variable and value.
  /////////////////////////////////////
  class Assignment final : public Statement {
    ADD_CONSTREF(std::string, variable)
    ADD_PROPERTY(Expression_Obj, value)
    ADD_PROPERTY(bool, is_default)
    ADD_PROPERTY(bool, is_global)
  public:
    Assignment(ParserState pstate, std::string var, Expression_Obj val, bool is_default = false, bool is_global = false);
    ATTACH_AST_OPERATIONS(Assignment)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////////
  // Import directives. CSS and Sass import lists can be intermingled, so it's
  // necessary to store a list of each in an Import node.
  ////////////////////////////////////////////////////////////////////////////
  class Import final : public Statement {
    std::vector<Expression_Obj> urls_;
    std::vector<Include>        incs_;
    ADD_PROPERTY(List_Obj,      import_queries);
  public:
    Import(ParserState pstate);
    std::vector<Include>& incs();
    std::vector<Expression_Obj>& urls();
    ATTACH_AST_OPERATIONS(Import)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  // not yet resolved single import
  // so far we only know requested name
  class Import_Stub final : public Statement {
    Include resource_;
  public:
    Import_Stub(ParserState pstate, Include res);
    Include resource();
    std::string imp_path();
    std::string abs_path();
    ATTACH_AST_OPERATIONS(Import_Stub)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////
  // The Sass `@warn` directive.
  //////////////////////////////
  class Warning final : public Statement {
    ADD_PROPERTY(Expression_Obj, message)
  public:
    Warning(ParserState pstate, Expression_Obj msg);
    ATTACH_AST_OPERATIONS(Warning)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////
  // The Sass `@error` directive.
  ///////////////////////////////
  class Error final : public Statement {
    ADD_PROPERTY(Expression_Obj, message)
  public:
    Error(ParserState pstate, Expression_Obj msg);
    ATTACH_AST_OPERATIONS(Error)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////
  // The Sass `@debug` directive.
  ///////////////////////////////
  class Debug final : public Statement {
    ADD_PROPERTY(Expression_Obj, value)
  public:
    Debug(ParserState pstate, Expression_Obj val);
    ATTACH_AST_OPERATIONS(Debug)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////
  // CSS comments. These may be interpolated.
  ///////////////////////////////////////////
  class Comment final : public Statement {
    ADD_PROPERTY(String_Obj, text)
    ADD_PROPERTY(bool, is_important)
  public:
    Comment(ParserState pstate, String_Obj txt, bool is_important);
    virtual bool is_invisible() const override;
    ATTACH_AST_OPERATIONS(Comment)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////
  // The Sass `@if` control directive.
  ////////////////////////////////////
  class If final : public Has_Block {
    ADD_PROPERTY(Expression_Obj, predicate)
    ADD_PROPERTY(Block_Obj, alternative)
  public:
    If(ParserState pstate, Expression_Obj pred, Block_Obj con, Block_Obj alt = {});
    virtual bool has_content() override;
    ATTACH_AST_OPERATIONS(If)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////
  // The Sass `@for` control directive.
  /////////////////////////////////////
  class For final : public Has_Block {
    ADD_CONSTREF(std::string, variable)
    ADD_PROPERTY(Expression_Obj, lower_bound)
    ADD_PROPERTY(Expression_Obj, upper_bound)
    ADD_PROPERTY(bool, is_inclusive)
  public:
    For(ParserState pstate, std::string var, Expression_Obj lo, Expression_Obj hi, Block_Obj b, bool inc);
    ATTACH_AST_OPERATIONS(For)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////////////
  // The Sass `@each` control directive.
  //////////////////////////////////////
  class Each final : public Has_Block {
    ADD_PROPERTY(std::vector<std::string>, variables)
    ADD_PROPERTY(Expression_Obj, list)
  public:
    Each(ParserState pstate, std::vector<std::string> vars, Expression_Obj lst, Block_Obj b);
    ATTACH_AST_OPERATIONS(Each)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////
  // The Sass `@while` control directive.
  ///////////////////////////////////////
  class While final : public Has_Block {
    ADD_PROPERTY(Expression_Obj, predicate)
  public:
    While(ParserState pstate, Expression_Obj pred, Block_Obj b);
    ATTACH_AST_OPERATIONS(While)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////////////////
  // The @return directive for use inside SassScript functions.
  /////////////////////////////////////////////////////////////
  class Return final : public Statement {
    ADD_PROPERTY(Expression_Obj, value)
  public:
    Return(ParserState pstate, Expression_Obj val);
    ATTACH_AST_OPERATIONS(Return)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////
  // The Sass `@extend` directive.
  ////////////////////////////////
  class Extension final : public Statement {
    ADD_PROPERTY(Selector_List_Obj, selector)
  public:
    Extension(ParserState pstate, Selector_List_Obj s);
    ATTACH_AST_OPERATIONS(Extension)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////////////////////////////////
  // Definitions for both mixins and functions. The two cases are distinguished
  // by a type tag.
  /////////////////////////////////////////////////////////////////////////////
  class Definition final : public Has_Block {
  public:
    enum Type { MIXIN, FUNCTION };
    ADD_CONSTREF(std::string, name)
    ADD_PROPERTY(Parameters_Obj, parameters)
    ADD_PROPERTY(Env*, environment)
    ADD_PROPERTY(Type, type)
    ADD_PROPERTY(Native_Function, native_function)
    ADD_PROPERTY(Sass_Function_Entry, c_function)
    ADD_PROPERTY(void*, cookie)
    ADD_PROPERTY(bool, is_overload_stub)
    ADD_PROPERTY(Signature, signature)
  public:
    Definition(ParserState pstate,
               std::string n,
               Parameters_Obj params,
               Block_Obj b,
               Type t);
    Definition(ParserState pstate,
               Signature sig,
               std::string n,
               Parameters_Obj params,
               Native_Function func_ptr,
               bool overload_stub = false);
    Definition(ParserState pstate,
               Signature sig,
               std::string n,
               Parameters_Obj params,
               Sass_Function_Entry c_func);
    ATTACH_AST_OPERATIONS(Definition)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////////////
  // Mixin calls (i.e., `@include ...`).
  //////////////////////////////////////
  class Mixin_Call final : public Has_Block {
    ADD_CONSTREF(std::string, name)
    ADD_PROPERTY(Arguments_Obj, arguments)
    ADD_PROPERTY(Parameters_Obj, block_parameters)
  public:
    Mixin_Call(ParserState pstate, std::string n, Arguments_Obj args, Parameters_Obj b_params = {}, Block_Obj b = {});
    ATTACH_AST_OPERATIONS(Mixin_Call)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////////////
  // The @content directive for mixin content blocks.
  ///////////////////////////////////////////////////
  class Content final : public Statement {
    ADD_PROPERTY(Arguments_Obj, arguments)
  public:
    Content(ParserState pstate, Arguments_Obj args);
    ATTACH_AST_OPERATIONS(Content)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////////
  // Arithmetic negation (logical negation is just an ordinary function call).
  ////////////////////////////////////////////////////////////////////////////
  class Unary_Expression final : public Expression {
  public:
    enum Type { PLUS, MINUS, NOT, SLASH };
  private:
    HASH_PROPERTY(Type, optype)
    HASH_PROPERTY(Expression_Obj, operand)
    mutable size_t hash_;
  public:
    Unary_Expression(ParserState pstate, Type t, Expression_Obj o);
    const std::string type_name();
    virtual bool operator==(const Expression& rhs) const override;
    size_t hash() const override;
    ATTACH_AST_OPERATIONS(Unary_Expression)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////
  // Individual argument objects for mixin and function calls.
  ////////////////////////////////////////////////////////////
  class Argument final : public Expression {
    HASH_PROPERTY(Expression_Obj, value)
    HASH_CONSTREF(std::string, name)
    ADD_PROPERTY(bool, is_rest_argument)
    ADD_PROPERTY(bool, is_keyword_argument)
    mutable size_t hash_;
  public:
    Argument(ParserState pstate, Expression_Obj val, std::string n = "", bool rest = false, bool keyword = false);
    void set_delayed(bool delayed) override;
    bool operator==(const Expression& rhs) const override;
    size_t hash() const override;
    ATTACH_AST_OPERATIONS(Argument)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////
  // Argument lists -- in their own class to facilitate context-sensitive
  // error checking (e.g., ensuring that all ordinal arguments precede all
  // named arguments).
  ////////////////////////////////////////////////////////////////////////
  class Arguments final : public Expression, public Vectorized<Argument_Obj> {
    ADD_PROPERTY(bool, has_named_arguments)
    ADD_PROPERTY(bool, has_rest_argument)
    ADD_PROPERTY(bool, has_keyword_argument)
  protected:
    void adjust_after_pushing(Argument_Obj a) override;
  public:
    Arguments(ParserState pstate);
    void set_delayed(bool delayed) override;
    Argument_Obj get_rest_argument();
    Argument_Obj get_keyword_argument();
    ATTACH_AST_OPERATIONS(Arguments)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////
  // Media queries.
  /////////////////
  class Media_Query final : public Expression,
                            public Vectorized<Media_Query_Expression_Obj> {
    ADD_PROPERTY(String_Obj, media_type)
    ADD_PROPERTY(bool, is_negated)
    ADD_PROPERTY(bool, is_restricted)
  public:
    Media_Query(ParserState pstate, String_Obj t = {}, size_t s = 0, bool n = false, bool r = false);
    ATTACH_AST_OPERATIONS(Media_Query)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////
  // Media expressions (for use inside media queries).
  ////////////////////////////////////////////////////
  class Media_Query_Expression final : public Expression {
    ADD_PROPERTY(Expression_Obj, feature)
    ADD_PROPERTY(Expression_Obj, value)
    ADD_PROPERTY(bool, is_interpolated)
  public:
    Media_Query_Expression(ParserState pstate, Expression_Obj f, Expression_Obj v, bool i = false);
    ATTACH_AST_OPERATIONS(Media_Query_Expression)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////
  // At root expressions (for use inside @at-root).
  /////////////////////////////////////////////////
  class At_Root_Query final : public Expression {
  private:
    ADD_PROPERTY(Expression_Obj, feature)
    ADD_PROPERTY(Expression_Obj, value)
  public:
    At_Root_Query(ParserState pstate, Expression_Obj f = {}, Expression_Obj v = {}, bool i = false);
    bool exclude(std::string str);
    ATTACH_AST_OPERATIONS(At_Root_Query)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////
  // At-root.
  ///////////
  class At_Root_Block final : public Has_Block {
    ADD_PROPERTY(At_Root_Query_Obj, expression)
  public:
    At_Root_Block(ParserState pstate, Block_Obj b = {}, At_Root_Query_Obj e = {});
    bool bubbles() override;
    bool exclude_node(Statement_Obj s);
    ATTACH_AST_OPERATIONS(At_Root_Block)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////////////
  // Individual parameter objects for mixins and functions.
  /////////////////////////////////////////////////////////
  class Parameter final : public AST_Node {
    ADD_CONSTREF(std::string, name)
    ADD_PROPERTY(Expression_Obj, default_value)
    ADD_PROPERTY(bool, is_rest_parameter)
  public:
    Parameter(ParserState pstate, std::string n, Expression_Obj def = {}, bool rest = false);
    ATTACH_AST_OPERATIONS(Parameter)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////////////////////////////
  // Parameter lists -- in their own class to facilitate context-sensitive
  // error checking (e.g., ensuring that all optional parameters follow all
  // required parameters).
  /////////////////////////////////////////////////////////////////////////
  class Parameters final : public AST_Node, public Vectorized<Parameter_Obj> {
    ADD_PROPERTY(bool, has_optional_parameters)
    ADD_PROPERTY(bool, has_rest_parameter)
  protected:
    void adjust_after_pushing(Parameter_Obj p) override;
  public:
    Parameters(ParserState pstate);
    ATTACH_AST_OPERATIONS(Parameters)
    ATTACH_CRTP_PERFORM_METHODS()
  };

}

#include "ast_values.hpp"
#include "ast_supports.hpp"
#include "ast_selectors.hpp"

#ifdef __clang__

// #pragma clang diagnostic pop
// #pragma clang diagnostic push

#endif

#endif
