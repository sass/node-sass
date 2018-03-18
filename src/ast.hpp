#ifndef SASS_AST_H
#define SASS_AST_H

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
  const double NUMBER_EPSILON = 0.00000000000001;

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
    enum Concrete_Type {
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
      NUM_TYPES
    };
  private:
    // expressions in some contexts shouldn't be evaluated
    ADD_PROPERTY(bool, is_delayed)
    ADD_PROPERTY(bool, is_expanded)
    ADD_PROPERTY(bool, is_interpolant)
    ADD_PROPERTY(Concrete_Type, concrete_type)
  public:
    Expression(ParserState pstate,
               bool d = false, bool e = false, bool i = false, Concrete_Type ct = NONE)
    : AST_Node(pstate),
      is_delayed_(d),
      is_expanded_(e),
      is_interpolant_(i),
      concrete_type_(ct)
    { }
    Expression(const Expression* ptr)
    : AST_Node(ptr),
      is_delayed_(ptr->is_delayed_),
      is_expanded_(ptr->is_expanded_),
      is_interpolant_(ptr->is_interpolant_),
      concrete_type_(ptr->concrete_type_)
    { }
    virtual operator bool() { return true; }
    virtual ~Expression() { }
    virtual std::string type() const { return ""; /* TODO: raise an error? */ }
    virtual bool is_invisible() const { return false; }
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
    bool has_duplicate_key() const         { return duplicate_key_ != 0; }
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
    enum Statement_Type {
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
    ADD_PROPERTY(Statement_Type, statement_type)
    ADD_PROPERTY(size_t, tabs)
    ADD_PROPERTY(bool, group_end)
  public:
    Statement(ParserState pstate, Statement_Type st = NONE, size_t t = 0)
    : AST_Node(pstate), statement_type_(st), tabs_(t), group_end_(false)
     { }
    Statement(const Statement* ptr)
    : AST_Node(ptr),
      statement_type_(ptr->statement_type_),
      tabs_(ptr->tabs_),
      group_end_(ptr->group_end_)
     { }
    virtual ~Statement() = 0;
    // needed for rearranging nested rulesets during CSS emission
    virtual bool   is_invisible() const { return false; }
    virtual bool   bubbles() { return false; }
    virtual bool has_content()
    {
      return statement_type_ == CONTENT;
    }
  };
  inline Statement::~Statement() { }

  ////////////////////////
  // Blocks of statements.
  ////////////////////////
  class Block final : public Statement, public Vectorized<Statement_Obj> {
    ADD_PROPERTY(bool, is_root)
    // needed for properly formatted CSS emission
  protected:
    void adjust_after_pushing(Statement_Obj s) override
    {
    }
  public:
    Block(ParserState pstate, size_t s = 0, bool r = false)
    : Statement(pstate),
      Vectorized<Statement_Obj>(s),
      is_root_(r)
    { }
    Block(const Block* ptr)
    : Statement(ptr),
      Vectorized<Statement_Obj>(*ptr),
      is_root_(ptr->is_root_)
    { }
    bool has_content() override
    {
      for (size_t i = 0, L = elements().size(); i < L; ++i) {
        if (elements()[i]->has_content()) return true;
      }
      return Statement::has_content();
    }
    ATTACH_AST_OPERATIONS(Block)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements that contain blocks of statements.
  ////////////////////////////////////////////////////////////////////////
  class Has_Block : public Statement {
    ADD_PROPERTY(Block_Obj, block)
  public:
    Has_Block(ParserState pstate, Block_Obj b)
    : Statement(pstate), block_(b)
    { }
    Has_Block(const Has_Block* ptr)
    : Statement(ptr), block_(ptr->block_)
    { }
    virtual bool has_content() override
    {
      return (block_ && block_->has_content()) || Statement::has_content();
    }
    virtual ~Has_Block() = 0;
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
    Ruleset(ParserState pstate, Selector_List_Obj s = {}, Block_Obj b = {})
    : Has_Block(pstate, b), selector_(s), is_root_(false)
    { statement_type(RULESET); }
    Ruleset(const Ruleset* ptr)
    : Has_Block(ptr),
      selector_(ptr->selector_),
      is_root_(ptr->is_root_)
    { statement_type(RULESET); }
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
    Bubble(ParserState pstate, Statement_Obj n, Statement_Obj g = {}, size_t t = 0)
    : Statement(pstate, Statement::BUBBLE, t), node_(n), group_end_(g == 0)
    { }
    Bubble(const Bubble* ptr)
    : Statement(ptr),
      node_(ptr->node_),
      group_end_(ptr->group_end_)
    { }
    bool bubbles() override { return true; }
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
    Trace(ParserState pstate, std::string n, Block_Obj b = {}, char type = 'm')
    : Has_Block(pstate, b), type_(type), name_(n)
    { }
    Trace(const Trace* ptr)
    : Has_Block(ptr),
      type_(ptr->type_),
      name_(ptr->name_)
    { }
    ATTACH_AST_OPERATIONS(Trace)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////
  // Media queries.
  /////////////////
  class Media_Block final : public Has_Block {
    ADD_PROPERTY(List_Obj, media_queries)
  public:
    Media_Block(ParserState pstate, List_Obj mqs, Block_Obj b)
    : Has_Block(pstate, b), media_queries_(mqs)
    { statement_type(MEDIA); }
    Media_Block(const Media_Block* ptr)
    : Has_Block(ptr), media_queries_(ptr->media_queries_)
    { statement_type(MEDIA); }
    bool bubbles() override { return true; }
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
    Directive(ParserState pstate, std::string kwd, Selector_List_Obj sel = {}, Block_Obj b = {}, Expression_Obj val = {})
    : Has_Block(pstate, b), keyword_(kwd), selector_(sel), value_(val) // set value manually if needed
    { statement_type(DIRECTIVE); }
    Directive(const Directive* ptr)
    : Has_Block(ptr),
      keyword_(ptr->keyword_),
      selector_(ptr->selector_),
      value_(ptr->value_) // set value manually if needed
    { statement_type(DIRECTIVE); }
    bool bubbles() override { return is_keyframes() || is_media(); }
    bool is_media() {
      return keyword_.compare("@-webkit-media") == 0 ||
             keyword_.compare("@-moz-media") == 0 ||
             keyword_.compare("@-o-media") == 0 ||
             keyword_.compare("@media") == 0;
    }
    bool is_keyframes() {
      return keyword_.compare("@-webkit-keyframes") == 0 ||
             keyword_.compare("@-moz-keyframes") == 0 ||
             keyword_.compare("@-o-keyframes") == 0 ||
             keyword_.compare("@keyframes") == 0;
    }
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
    Keyframe_Rule(ParserState pstate, Block_Obj b)
    : Has_Block(pstate, b), name_()
    { statement_type(KEYFRAMERULE); }
    Keyframe_Rule(const Keyframe_Rule* ptr)
    : Has_Block(ptr), name_(ptr->name_)
    { statement_type(KEYFRAMERULE); }
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
    Declaration(ParserState pstate,
                String_Obj prop, Expression_Obj val, bool i = false, bool c = false, Block_Obj b = {})
    : Has_Block(pstate, b), property_(prop), value_(val), is_important_(i), is_custom_property_(c), is_indented_(false)
    { statement_type(DECLARATION); }
    Declaration(const Declaration* ptr)
    : Has_Block(ptr),
      property_(ptr->property_),
      value_(ptr->value_),
      is_important_(ptr->is_important_),
      is_custom_property_(ptr->is_custom_property_),
      is_indented_(ptr->is_indented_)
    { statement_type(DECLARATION); }
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
    Assignment(ParserState pstate,
               std::string var, Expression_Obj val,
               bool is_default = false,
               bool is_global = false)
    : Statement(pstate), variable_(var), value_(val), is_default_(is_default), is_global_(is_global)
    { statement_type(ASSIGNMENT); }
    Assignment(const Assignment* ptr)
    : Statement(ptr),
      variable_(ptr->variable_),
      value_(ptr->value_),
      is_default_(ptr->is_default_),
      is_global_(ptr->is_global_)
    { statement_type(ASSIGNMENT); }
    ATTACH_AST_OPERATIONS(Assignment)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////////
  // Import directives. CSS and Sass import lists can be intermingled, so it's
  // necessary to store a list of each in an Import node.
  ////////////////////////////////////////////////////////////////////////////
  class Import final : public Statement {
    std::vector<Expression_Obj> urls_;
    std::vector<Include>     incs_;
    ADD_PROPERTY(List_Obj,      import_queries);
  public:
    Import(ParserState pstate)
    : Statement(pstate),
      urls_(std::vector<Expression_Obj>()),
      incs_(std::vector<Include>()),
      import_queries_()
    { statement_type(IMPORT); }
    Import(const Import* ptr)
    : Statement(ptr),
      urls_(ptr->urls_),
      incs_(ptr->incs_),
      import_queries_(ptr->import_queries_)
    { statement_type(IMPORT); }
    std::vector<Expression_Obj>& urls() { return urls_; }
    std::vector<Include>& incs() { return incs_; }
    ATTACH_AST_OPERATIONS(Import)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  // not yet resolved single import
  // so far we only know requested name
  class Import_Stub final : public Statement {
    Include resource_;
  public:
    std::string abs_path() { return resource_.abs_path; };
    std::string imp_path() { return resource_.imp_path; };
    Include resource() { return resource_; };

    Import_Stub(ParserState pstate, Include res)
    : Statement(pstate), resource_(res)
    { statement_type(IMPORT_STUB); }
    Import_Stub(const Import_Stub* ptr)
    : Statement(ptr), resource_(ptr->resource_)
    { statement_type(IMPORT_STUB); }
    ATTACH_AST_OPERATIONS(Import_Stub)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////
  // The Sass `@warn` directive.
  //////////////////////////////
  class Warning final : public Statement {
    ADD_PROPERTY(Expression_Obj, message)
  public:
    Warning(ParserState pstate, Expression_Obj msg)
    : Statement(pstate), message_(msg)
    { statement_type(WARNING); }
    Warning(const Warning* ptr)
    : Statement(ptr), message_(ptr->message_)
    { statement_type(WARNING); }
    ATTACH_AST_OPERATIONS(Warning)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////
  // The Sass `@error` directive.
  ///////////////////////////////
  class Error final : public Statement {
    ADD_PROPERTY(Expression_Obj, message)
  public:
    Error(ParserState pstate, Expression_Obj msg)
    : Statement(pstate), message_(msg)
    { statement_type(ERROR); }
    Error(const Error* ptr)
    : Statement(ptr), message_(ptr->message_)
    { statement_type(ERROR); }
    ATTACH_AST_OPERATIONS(Error)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////
  // The Sass `@debug` directive.
  ///////////////////////////////
  class Debug final : public Statement {
    ADD_PROPERTY(Expression_Obj, value)
  public:
    Debug(ParserState pstate, Expression_Obj val)
    : Statement(pstate), value_(val)
    { statement_type(DEBUGSTMT); }
    Debug(const Debug* ptr)
    : Statement(ptr), value_(ptr->value_)
    { statement_type(DEBUGSTMT); }
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
    Comment(ParserState pstate, String_Obj txt, bool is_important)
    : Statement(pstate), text_(txt), is_important_(is_important)
    { statement_type(COMMENT); }
    Comment(const Comment* ptr)
    : Statement(ptr),
      text_(ptr->text_),
      is_important_(ptr->is_important_)
    { statement_type(COMMENT); }
    bool is_invisible() const override
    { return /* is_important() == */ false; }
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
    If(ParserState pstate, Expression_Obj pred, Block_Obj con, Block_Obj alt = {})
    : Has_Block(pstate, con), predicate_(pred), alternative_(alt)
    { statement_type(IF); }
    If(const If* ptr)
    : Has_Block(ptr),
      predicate_(ptr->predicate_),
      alternative_(ptr->alternative_)
    { statement_type(IF); }
    bool has_content() override
    {
      return Has_Block::has_content() || (alternative_ && alternative_->has_content());
    }
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
    For(ParserState pstate,
        std::string var, Expression_Obj lo, Expression_Obj hi, Block_Obj b, bool inc)
    : Has_Block(pstate, b),
      variable_(var), lower_bound_(lo), upper_bound_(hi), is_inclusive_(inc)
    { statement_type(FOR); }
    For(const For* ptr)
    : Has_Block(ptr),
      variable_(ptr->variable_),
      lower_bound_(ptr->lower_bound_),
      upper_bound_(ptr->upper_bound_),
      is_inclusive_(ptr->is_inclusive_)
    { statement_type(FOR); }
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
    Each(ParserState pstate, std::vector<std::string> vars, Expression_Obj lst, Block_Obj b)
    : Has_Block(pstate, b), variables_(vars), list_(lst)
    { statement_type(EACH); }
    Each(const Each* ptr)
    : Has_Block(ptr), variables_(ptr->variables_), list_(ptr->list_)
    { statement_type(EACH); }
    ATTACH_AST_OPERATIONS(Each)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////
  // The Sass `@while` control directive.
  ///////////////////////////////////////
  class While final : public Has_Block {
    ADD_PROPERTY(Expression_Obj, predicate)
  public:
    While(ParserState pstate, Expression_Obj pred, Block_Obj b)
    : Has_Block(pstate, b), predicate_(pred)
    { statement_type(WHILE); }
    While(const While* ptr)
    : Has_Block(ptr), predicate_(ptr->predicate_)
    { statement_type(WHILE); }
    ATTACH_AST_OPERATIONS(While)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////////////////
  // The @return directive for use inside SassScript functions.
  /////////////////////////////////////////////////////////////
  class Return final : public Statement {
    ADD_PROPERTY(Expression_Obj, value)
  public:
    Return(ParserState pstate, Expression_Obj val)
    : Statement(pstate), value_(val)
    { statement_type(RETURN); }
    Return(const Return* ptr)
    : Statement(ptr), value_(ptr->value_)
    { statement_type(RETURN); }
    ATTACH_AST_OPERATIONS(Return)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////
  // The Sass `@extend` directive.
  ////////////////////////////////
  class Extension final : public Statement {
    ADD_PROPERTY(Selector_List_Obj, selector)
  public:
    Extension(ParserState pstate, Selector_List_Obj s)
    : Statement(pstate), selector_(s)
    { statement_type(EXTEND); }
    Extension(const Extension* ptr)
    : Statement(ptr), selector_(ptr->selector_)
    { statement_type(EXTEND); }
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
    Definition(const Definition* ptr)
    : Has_Block(ptr),
      name_(ptr->name_),
      parameters_(ptr->parameters_),
      environment_(ptr->environment_),
      type_(ptr->type_),
      native_function_(ptr->native_function_),
      c_function_(ptr->c_function_),
      cookie_(ptr->cookie_),
      is_overload_stub_(ptr->is_overload_stub_),
      signature_(ptr->signature_)
    { }

    Definition(ParserState pstate,
               std::string n,
               Parameters_Obj params,
               Block_Obj b,
               Type t)
    : Has_Block(pstate, b),
      name_(n),
      parameters_(params),
      environment_(0),
      type_(t),
      native_function_(0),
      c_function_(0),
      cookie_(0),
      is_overload_stub_(false),
      signature_(0)
    { }
    Definition(ParserState pstate,
               Signature sig,
               std::string n,
               Parameters_Obj params,
               Native_Function func_ptr,
               bool overload_stub = false)
    : Has_Block(pstate, {}),
      name_(n),
      parameters_(params),
      environment_(0),
      type_(FUNCTION),
      native_function_(func_ptr),
      c_function_(0),
      cookie_(0),
      is_overload_stub_(overload_stub),
      signature_(sig)
    { }
    Definition(ParserState pstate,
               Signature sig,
               std::string n,
               Parameters_Obj params,
               Sass_Function_Entry c_func)
    : Has_Block(pstate, {}),
      name_(n),
      parameters_(params),
      environment_(0),
      type_(FUNCTION),
      native_function_(0),
      c_function_(c_func),
      cookie_(sass_function_get_cookie(c_func)),
      is_overload_stub_(false),
      signature_(sig)
    { }
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
    Mixin_Call(ParserState pstate, std::string n, Arguments_Obj args, Parameters_Obj b_params = {}, Block_Obj b = {})
    : Has_Block(pstate, b), name_(n), arguments_(args), block_parameters_(b_params)
    { }
    Mixin_Call(const Mixin_Call* ptr)
    : Has_Block(ptr),
      name_(ptr->name_),
      arguments_(ptr->arguments_),
      block_parameters_(ptr->block_parameters_)
    { }
    ATTACH_AST_OPERATIONS(Mixin_Call)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////////////
  // The @content directive for mixin content blocks.
  ///////////////////////////////////////////////////
  class Content final : public Statement {
    ADD_PROPERTY(Arguments_Obj, arguments)
  public:
    Content(ParserState pstate, Arguments_Obj args)
    : Statement(pstate),
      arguments_(args)
    { statement_type(CONTENT); }
    Content(const Content* ptr)
    : Statement(ptr),
      arguments_(ptr->arguments_)
    { statement_type(CONTENT); }
    ATTACH_AST_OPERATIONS(Content)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  inline static const std::string sass_op_to_name(enum Sass_OP op) {
    switch (op) {
      case AND: return "and";
      case OR: return "or";
      case EQ: return "eq";
      case NEQ: return "neq";
      case GT: return "gt";
      case GTE: return "gte";
      case LT: return "lt";
      case LTE: return "lte";
      case ADD: return "plus";
      case SUB: return "sub";
      case MUL: return "times";
      case DIV: return "div";
      case MOD: return "mod";
      // this is only used internally!
      case NUM_OPS: return "[OPS]";
      default: return "invalid";
    }
  }

  inline static const std::string sass_op_separator(enum Sass_OP op) {
    switch (op) {
      case AND: return "&&";
      case OR: return "||";
      case EQ: return "==";
      case NEQ: return "!=";
      case GT: return ">";
      case GTE: return ">=";
      case LT: return "<";
      case LTE: return "<=";
      case ADD: return "+";
      case SUB: return "-";
      case MUL: return "*";
      case DIV: return "/";
      case MOD: return "%";
      // this is only used internally!
      case NUM_OPS: return "[OPS]";
      default: return "invalid";
    }
  }

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
    Unary_Expression(ParserState pstate, Type t, Expression_Obj o)
    : Expression(pstate), optype_(t), operand_(o), hash_(0)
    { }
    Unary_Expression(const Unary_Expression* ptr)
    : Expression(ptr),
      optype_(ptr->optype_),
      operand_(ptr->operand_),
      hash_(ptr->hash_)
    { }
    const std::string type_name() {
      switch (optype_) {
        case PLUS: return "plus";
        case MINUS: return "minus";
        case SLASH: return "slash";
        case NOT: return "not";
        default: return "invalid";
      }
    }
    bool operator==(const Expression& rhs) const override
    {
      try
      {
        Unary_Expression_Ptr_Const m = Cast<Unary_Expression>(&rhs);
        if (m == 0) return false;
        return type() == m->type() &&
               *operand() == *m->operand();
      }
      catch (std::bad_cast&)
      {
        return false;
      }
      catch (...) { throw; }
    }
    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_ = std::hash<size_t>()(optype_);
        hash_combine(hash_, operand()->hash());
      };
      return hash_;
    }
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
    Argument(ParserState pstate, Expression_Obj val, std::string n = "", bool rest = false, bool keyword = false)
    : Expression(pstate), value_(val), name_(n), is_rest_argument_(rest), is_keyword_argument_(keyword), hash_(0)
    {
      if (!name_.empty() && is_rest_argument_) {
        coreError("variable-length argument may not be passed by name", pstate_);
      }
    }
    Argument(const Argument* ptr)
    : Expression(ptr),
      value_(ptr->value_),
      name_(ptr->name_),
      is_rest_argument_(ptr->is_rest_argument_),
      is_keyword_argument_(ptr->is_keyword_argument_),
      hash_(ptr->hash_)
    {
      if (!name_.empty() && is_rest_argument_) {
        coreError("variable-length argument may not be passed by name", pstate_);
      }
    }

    void set_delayed(bool delayed) override;
    bool operator==(const Expression& rhs) const override
    {
      try
      {
        Argument_Ptr_Const m = Cast<Argument>(&rhs);
        if (!(m && name() == m->name())) return false;
        return *value() == *m->value();
      }
      catch (std::bad_cast&)
      {
        return false;
      }
      catch (...) { throw; }
    }

    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_ = std::hash<std::string>()(name());
        hash_combine(hash_, value()->hash());
      }
      return hash_;
    }

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
    Arguments(ParserState pstate)
    : Expression(pstate),
      Vectorized<Argument_Obj>(),
      has_named_arguments_(false),
      has_rest_argument_(false),
      has_keyword_argument_(false)
    { }
    Arguments(const Arguments* ptr)
    : Expression(ptr),
      Vectorized<Argument_Obj>(*ptr),
      has_named_arguments_(ptr->has_named_arguments_),
      has_rest_argument_(ptr->has_rest_argument_),
      has_keyword_argument_(ptr->has_keyword_argument_)
    { }

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
    Media_Query(ParserState pstate,
                String_Obj t = {}, size_t s = 0, bool n = false, bool r = false)
    : Expression(pstate), Vectorized<Media_Query_Expression_Obj>(s),
      media_type_(t), is_negated_(n), is_restricted_(r)
    { }
    Media_Query(const Media_Query* ptr)
    : Expression(ptr),
      Vectorized<Media_Query_Expression_Obj>(*ptr),
      media_type_(ptr->media_type_),
      is_negated_(ptr->is_negated_),
      is_restricted_(ptr->is_restricted_)
    { }
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
    Media_Query_Expression(ParserState pstate,
                           Expression_Obj f, Expression_Obj v, bool i = false)
    : Expression(pstate), feature_(f), value_(v), is_interpolated_(i)
    { }
    Media_Query_Expression(const Media_Query_Expression* ptr)
    : Expression(ptr),
      feature_(ptr->feature_),
      value_(ptr->value_),
      is_interpolated_(ptr->is_interpolated_)
    { }
    ATTACH_AST_OPERATIONS(Media_Query_Expression)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////
  // `@supports` rule.
  ////////////////////
  class Supports_Block final : public Has_Block {
    ADD_PROPERTY(Supports_Condition_Obj, condition)
  public:
    Supports_Block(ParserState pstate, Supports_Condition_Obj condition, Block_Obj block = {})
    : Has_Block(pstate, block), condition_(condition)
    { statement_type(SUPPORTS); }
    Supports_Block(const Supports_Block* ptr)
    : Has_Block(ptr), condition_(ptr->condition_)
    { statement_type(SUPPORTS); }
    bool bubbles() override { return true; }
    ATTACH_AST_OPERATIONS(Supports_Block)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////////////////////////////
  // The abstract superclass of all Supports conditions.
  //////////////////////////////////////////////////////
  class Supports_Condition : public Expression {
  public:
    Supports_Condition(ParserState pstate)
    : Expression(pstate)
    { }
    Supports_Condition(const Supports_Condition* ptr)
    : Expression(ptr)
    { }
    virtual bool needs_parens(Supports_Condition_Obj cond) const { return false; }
    ATTACH_AST_OPERATIONS(Supports_Condition)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////
  // An operator condition (e.g. `CONDITION1 and CONDITION2`).
  ////////////////////////////////////////////////////////////
  class Supports_Operator final : public Supports_Condition {
  public:
    enum Operand { AND, OR };
  private:
    ADD_PROPERTY(Supports_Condition_Obj, left);
    ADD_PROPERTY(Supports_Condition_Obj, right);
    ADD_PROPERTY(Operand, operand);
  public:
    Supports_Operator(ParserState pstate, Supports_Condition_Obj l, Supports_Condition_Obj r, Operand o)
    : Supports_Condition(pstate), left_(l), right_(r), operand_(o)
    { }
    Supports_Operator(const Supports_Operator* ptr)
    : Supports_Condition(ptr),
      left_(ptr->left_),
      right_(ptr->right_),
      operand_(ptr->operand_)
    { }
    bool needs_parens(Supports_Condition_Obj cond) const override;
    ATTACH_AST_OPERATIONS(Supports_Operator)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////////////////
  // A negation condition (`not CONDITION`).
  //////////////////////////////////////////
  class Supports_Negation final : public Supports_Condition {
  private:
    ADD_PROPERTY(Supports_Condition_Obj, condition);
  public:
    Supports_Negation(ParserState pstate, Supports_Condition_Obj c)
    : Supports_Condition(pstate), condition_(c)
    { }
    Supports_Negation(const Supports_Negation* ptr)
    : Supports_Condition(ptr), condition_(ptr->condition_)
    { }
    bool needs_parens(Supports_Condition_Obj cond) const override;
    ATTACH_AST_OPERATIONS(Supports_Negation)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////////
  // A declaration condition (e.g. `(feature: value)`).
  /////////////////////////////////////////////////////
  class Supports_Declaration final : public Supports_Condition {
  private:
    ADD_PROPERTY(Expression_Obj, feature);
    ADD_PROPERTY(Expression_Obj, value);
  public:
    Supports_Declaration(ParserState pstate, Expression_Obj f, Expression_Obj v)
    : Supports_Condition(pstate), feature_(f), value_(v)
    { }
    Supports_Declaration(const Supports_Declaration* ptr)
    : Supports_Condition(ptr),
      feature_(ptr->feature_),
      value_(ptr->value_)
    { }
    bool needs_parens(Supports_Condition_Obj cond) const override { return false; }
    ATTACH_AST_OPERATIONS(Supports_Declaration)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////////
  // An interpolation condition (e.g. `#{$var}`).
  ///////////////////////////////////////////////
  class Supports_Interpolation final : public Supports_Condition {
  private:
    ADD_PROPERTY(Expression_Obj, value);
  public:
    Supports_Interpolation(ParserState pstate, Expression_Obj v)
    : Supports_Condition(pstate), value_(v)
    { }
    Supports_Interpolation(const Supports_Interpolation* ptr)
    : Supports_Condition(ptr),
      value_(ptr->value_)
    { }
    bool needs_parens(Supports_Condition_Obj cond) const override { return false; }
    ATTACH_AST_OPERATIONS(Supports_Interpolation)
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
    At_Root_Query(ParserState pstate, Expression_Obj f = {}, Expression_Obj v = {}, bool i = false)
    : Expression(pstate), feature_(f), value_(v)
    { }
    At_Root_Query(const At_Root_Query* ptr)
    : Expression(ptr),
      feature_(ptr->feature_),
      value_(ptr->value_)
    { }
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
    At_Root_Block(ParserState pstate, Block_Obj b = {}, At_Root_Query_Obj e = {})
    : Has_Block(pstate, b), expression_(e)
    { statement_type(ATROOT); }
    At_Root_Block(const At_Root_Block* ptr)
    : Has_Block(ptr), expression_(ptr->expression_)
    { statement_type(ATROOT); }
    bool bubbles() override { return true; }
    bool exclude_node(Statement_Obj s) {
      if (expression() == 0)
      {
        return s->statement_type() == Statement::RULESET;
      }

      if (s->statement_type() == Statement::DIRECTIVE)
      {
        if (Directive_Obj dir = Cast<Directive>(s))
        {
          std::string keyword(dir->keyword());
          if (keyword.length() > 0) keyword.erase(0, 1);
          return expression()->exclude(keyword);
        }
      }
      if (s->statement_type() == Statement::MEDIA)
      {
        return expression()->exclude("media");
      }
      if (s->statement_type() == Statement::RULESET)
      {
        return expression()->exclude("rule");
      }
      if (s->statement_type() == Statement::SUPPORTS)
      {
        return expression()->exclude("supports");
      }
      if (Directive_Obj dir = Cast<Directive>(s))
      {
        if (dir->is_keyframes()) return expression()->exclude("keyframes");
      }
      return false;
    }
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
    Parameter(ParserState pstate,
              std::string n, Expression_Obj def = {}, bool rest = false)
    : AST_Node(pstate), name_(n), default_value_(def), is_rest_parameter_(rest)
    {
      // tried to come up with a spec test for this, but it does no longer
      // get  past the parser (it error out earlier). A spec test was added!
      // if (default_value_ && is_rest_parameter_) {
      //   error("variable-length parameter may not have a default value", pstate_);
      // }
    }
    Parameter(const Parameter* ptr)
    : AST_Node(ptr),
      name_(ptr->name_),
      default_value_(ptr->default_value_),
      is_rest_parameter_(ptr->is_rest_parameter_)
    {
      // tried to come up with a spec test for this, but it does no longer
      // get  past the parser (it error out earlier). A spec test was added!
      // if (default_value_ && is_rest_parameter_) {
      //   error("variable-length parameter may not have a default value", pstate_);
      // }
    }
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
    void adjust_after_pushing(Parameter_Obj p) override
    {
      if (p->default_value()) {
        if (has_rest_parameter()) {
          coreError("optional parameters may not be combined with variable-length parameters", p->pstate());
        }
        has_optional_parameters(true);
      }
      else if (p->is_rest_parameter()) {
        if (has_rest_parameter()) {
          coreError("functions and mixins cannot have more than one variable-length parameter", p->pstate());
        }
        has_rest_parameter(true);
      }
      else {
        if (has_rest_parameter()) {
          coreError("required parameters must precede variable-length parameters", p->pstate());
        }
        if (has_optional_parameters()) {
          coreError("required parameters must precede optional parameters", p->pstate());
        }
      }
    }
  public:
    Parameters(ParserState pstate)
    : AST_Node(pstate),
      Vectorized<Parameter_Obj>(),
      has_optional_parameters_(false),
      has_rest_parameter_(false)
    { }
    Parameters(const Parameters* ptr)
    : AST_Node(ptr),
      Vectorized<Parameter_Obj>(*ptr),
      has_optional_parameters_(ptr->has_optional_parameters_),
      has_rest_parameter_(ptr->has_rest_parameter_)
    { }
    ATTACH_AST_OPERATIONS(Parameters)
    ATTACH_CRTP_PERFORM_METHODS()
  };

}

#include "ast_values.hpp"
#include "ast_selectors.hpp"

#ifdef __clang__

// #pragma clang diagnostic pop
// #pragma clang diagnostic push

#endif

#endif
