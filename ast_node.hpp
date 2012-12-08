#define SASS_AST_NODE

#include <string>
#include <vector>

namespace Sass {
  using namespace std;


  //////////////////////////////////////////////////////////
  // Abstract base class for all abstract syntax tree nodes.
  //////////////////////////////////////////////////////////
  class AST_Node {

    string path_;
    size_t line_;

  public:

    AST_Node(string p, size_t l) : path_(p), line_(l) { }
    virtual ~AST_Node() = 0;

    // accessors
    string path() const { return path_; }
    size_t line() const { return line_; }

    // mutators
    string path(string p) { path_ = p; return path_; }
    size_t line(size_t l) { line_ = l; return line_; }

    // for visitor objects
    // virtual void accept(Visitor* v) { v(this); }

  };


  /////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements. This side of the AST hierarchy
  // represents elements in expansion contexts, which exist primarily to be
  // rewritten and macro-expanded.
  /////////////////////////////////////////////////////////////////////////
  class Statement : public AST_Node {

  public:

    Statement(string p, size_t l) : AST_Node(p, l) { }
    virtual ~Statement() = 0;

  };


  ////////////////////////
  // Blocks of statements.
  ////////////////////////
  class Block : public Statement {

    vector<Statement*> statements_;
    bool               root_;

  public:

    Block(string p, size_t l, size_t s = 0, bool r = false)
    : Statement(p, l),
      statements_(vector<Statement*>()),
      root_(r)
    { statements_.reserve(s); }

    bool is_root() const { return root_; }
    bool is_root(bool r) { return root_ = r; }

    size_t size() const
    { return statements_.size(); }

    Statement*& operator[](size_t i)
    { return statements_[i]; }

    Block& operator<<(Statement* s)
    {
      statements_.push_back(s);
      return *this;
    }

    Block& operator+=(Block* b)
    {
      for (size_t i = 0, S = size(); i < S; ++i)
        statements_.push_back((*b)[i]);
      return *this;
    }

  };


  ////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements that contain blocks of statements.
  ////////////////////////////////////////////////////////////////////////
  class Has_Block : public Statement {
    Block* block_;
  public:
    Has_Block(string p, size_t l, Block* b)
    : Statement(p, l),
      block_(b)
    { }
    virtual ~Has_Block() = 0;
    Block* block() const   { return block_; }
    Block* block(Block* b) { return block_ = b; }
  };


  /////////////////////////////////////////////////////////////////////////////
  // Rulesets (i.e., sets of styles headed by a selector and containing a block
  // of style declarations.
  /////////////////////////////////////////////////////////////////////////////
  class Selector;
  class Ruleset : public Has_Block {

    Selector* selector_;

  public:

    Ruleset(string p, size_t l, Selector* s, Block* b)
    : Has_Block(p, l, b),
      selector_(s)
    { }

    Selector* selector() const      { return selector_; }
    Selector* selector(Selector* s) { return selector_ = s; }

  };


  //////////////////////////////////////////
  // Nested declaration sets (i.e., namespaced properties).
  //////////////////////////////////////////
  class String;
  class Propset : public Has_Block {

    String* property_fragment_;

  public:

    Propset(string p, size_t l, String* pf, Block* b)
    : Has_Block(p, l, b),
      property_fragment_(pf)
    { }

    String* property_fragment() const    { return property_fragment_; }
    String* property_fragment(String* p) { return property_fragment_ = p; }

  };


  /////////////////
  // Media queries.
  /////////////////
  class Value;
  class Media_Query : public Has_Block {

    Value* query_;

  public:

    Media_Query(string p, size_t l, Value* q, Block* b)
    : Has_Block(p, l, b),
      query_(q)
    { }

    Value* query() const   { return query_; }
    Value* query(Value* q) { return query_ = q; }

  };


  /////////////////////////////////////////////////////////////////////
  // At-rules -- arbitrary directives with an optional statement block.
  /////////////////////////////////////////////////////////////////////
  class At_Rule : public Ruleset {

    String* keyword_;

  public:

    At_Rule(string p, size_t l, String* k, Selector* s, Block* b)
    : Ruleset(p, l, s, b),
      keyword_(k)
    { }

    String* keyword() const    { return keyword_; }
    String* keyword(String* k) { return keyword_ = k; }

  };


  ////////////////////////////////////////////////////////////////////////
  // Declarations -- style rules consisting of a property name and values.
  ////////////////////////////////////////////////////////////////////////
  class List;
  class Declaration : public Statement {

    String* property_;
    List*   values_;

  public:

    Declaration(string p, size_t l, String* prop, List* list)
    : Statement(p, l),
      property_(prop),
      values_(list)
    { }

    String* property() const { return property_; }
    List*   values() const   { return values_; }

    String* property(String* p) { return property_ = p; }
    List*   values(List* l)     { return values_ = l; }

  };


  /////////////////////////////////
  // Assignments -- variable and value.
  /////////////////////////////////
  class Variable;
  class Assignment : public Statement {

    Variable* variable_;
    Value*    value_;
    bool      guarded_;

  public:

    Assignment(string p, size_t l, Variable* var, Value* val, bool g)
    : Statement(p, l),
      variable_(var),
      value_(val),
      guarded_(g)
    { }

    Variable* variable() const   { return variable_; }
    Value*    value() const      { return value_; }
    bool      is_guarded() const { return guarded_; }

    Variable* name(Variable* var) { return variable_ = var; }
    Value*    value(Value* val)   { return value_ = val; }
    bool      is_guarded(bool g)  { return guarded_ = g; }

  };


  ////////////////////////
  // CSS import directives
  ////////////////////////
  class Import : public Statement {

    String* location_;

  public:

    Import(string p, size_t l, String* loc)
    : Statement(p, l),
      location_(loc)
    { }

    String* location() const    { return location_; }
    String* location(String* l) { return location_ = l; }

  };


  //////////////////////////////
  // The Sass `@warn` directive.
  //////////////////////////////
  class Warning : public Statement {

    String* message_;

  public:

    Warning(string p, size_t l, String* m)
    : Statement(p, l),
      message_(m)
    { }

    String* message() const    { return message_; }
    String* message(String* m) { return message_ = m; }

  };


  ///////////////////////////////////////////
  // CSS comments. These may be interpolated.
  ///////////////////////////////////////////
  class Comment : public Statement {

    String* content_;

  public:

    Comment(string p, size_t l, String* c)
    : Statement(p, l),
      content_(c)
    { }

    String* content() const    { return content_; }
    String* content(String* c) { return content_ = c; }

  };


  ////////////////////////////////////
  // The Sass `@if` control directive.
  ////////////////////////////////////
  class If : public Statement {

    Value* predicate_;
    Block* consequent_;
    Block* alternative_;

  public:

    If(string p, size_t l, Value* pred, Block* con, Block* alt = 0)
    : Statement(p, l),
      consequent_(con),
      alternative_(alt)
    { }

    Value* predicate() const   { return predicate_; }
    Block* consequent() const  { return consequent_; }
    Block* alternative() const { return alternative_; }

    Value* predicate(Value* p)   { return predicate_ = p; }
    Block* consequent(Block* c)  { return consequent_ = c; }
    Block* alternative(Block* a) { return alternative_ = a; }

  };


  /////////////////////////////////////
  // The Sass `@for` control directive.
  /////////////////////////////////////
  class For : public Has_Block {

    Variable* variable_;
    Value* lower_bound_;
    Value* upper_bound_;
    bool inclusive_;

  public:

    For(string p,
        size_t l,
        Variable* v,
        Value* lo,
        Value* hi,
        Block* b,
        bool i)
    : Has_Block(p, l, b),
      variable_(v),
      lower_bound_(lo),
      upper_bound_(hi),
      inclusive_(i)
    { }

    Variable* variable() const    { return variable_; }
    Value*    lower_bound() const { return lower_bound_; }
    Value*    upper_bound() const { return upper_bound_; }
    bool      inclusive() const   { return inclusive_; }

    Variable* variable(Variable* v)   { return variable_ = v; }
    Value*    lower_bound(Value* lo) { return lower_bound_ = lo; }
    Value*    upper_bound(Value* hi) { return upper_bound_ = hi; }
    bool      inclusive(bool i)      { return inclusive_ = i; }

  };


  //////////////////////////////////////
  // The Sass `@each` control directive.
  //////////////////////////////////////
  class Each : public Has_Block {

    Variable* variable_;
    Value*    list_;

  public:

    Each(string p, size_t l, Variable* v, Value* list, Block* b)
    : Has_Block(p, l, b),
      variable_(v),
      list_(list)
    { }

    Variable* variable() const { return variable_; }
    Value*    list() const     { return list_; }

    Variable* variable(Variable* v) { return variable_ = v; }
    Value*    list(Value* l)        { return list_ = l; }

  };


  ///////////////////////////////////////
  // The Sass `@while` control directive.
  ///////////////////////////////////////
  class While : public Has_Block {

    Value* predicate_;

  public:

    While(string p, size_t l, Value* pred, Block* b)
    : Has_Block(p, l, b),
      predicate_(pred)
    { }

    Value* predicate() const   { return predicate_; }
    Value* predicate(Value* p) { return predicate_ = p; }

  };


  ////////////////////////////////
  // The Sass `@extend` directive.
  ////////////////////////////////
  class Extend : public Statement {

    Selector* selector_;

  public:

    Extend(string p, size_t l, Selector* s)
    : Statement(p, l),
      selector_(s)
    { }

    Selector* selector() const      { return selector_; }
    Selector* selector(Selector* s) { return selector_ = s; }

  };


  ///////////////////////////////////////////////////////////////////////
  // Definitions for both mixins and functions (distinguished by a flag).
  ///////////////////////////////////////////////////////////////////////
  class Parameters;
  class Definition : public Has_Block {

    String* name_;
    Parameters* parameters_;
    bool mixin_;

  public:

    Definition(string p,
               size_t l,
               String* n,
               Parameters* parms,
               Block* b,
               bool m)
    : Has_Block(p, l, b),
      name_(n),
      parameters_(parms),
      mixin_(m)
    { }

    String*     name() const       { return name_; }
    Parameters* parameters() const { return parameters_; }
    bool        is_mixin() const   { return mixin_; }

    String*     name(String* n)           { return name_ = n; }
    Parameters* parameters(Parameters* p) { return parameters_ = p; }
    bool        is_mixin(bool m)          { return mixin_ = m; }

  };


  //////////////////////////////////////
  // Mixin calls (i.e., `@include ...`).
  //////////////////////////////////////
  class Arguments;
  class Mixin_Call : public Has_Block {

    String* name_;
    Arguments* arguments_;

  public:

    Mixin_Call(string p, size_t l, String* n, Arguments* a, Block* b)
    : Has_Block(p, l, b),
      name_(n),
      arguments_(a)
    { }

    String*    name() const      { return name_; }
    Arguments* arguments() const { return arguments_; }

    String*    name(String* n)         { return name_ = n; }
    Arguments* arguments(Arguments* p) { return arguments_ = p; }

  };


  ////////////////////////////////////////////////////////////////////////////
  // Abstract base class for values. This side of the AST hierarchy represents
  // elements in value contexts, which exist primarily to be evaluated and
  // returned.
  ////////////////////////////////////////////////////////////////////////////
  class Value : public AST_Node {
  public:
    Value(string p, size_t l) : AST_Node(p, l) { }
    virtual ~Value() = 0;
  };


  ///////////////////////////////////////////////////////////////////////
  // Lists of values, both comma- and space-separated (distinguished by a
  // flag.) Also used to represent variable-length argument lists.
  ///////////////////////////////////////////////////////////////////////
  class List : public Value {

    vector<Value*> values_;
    bool           comma_separated_;
    bool           arglist_;

  public:

    List(string p, size_t l, size_t s = 0, bool c = false, bool a = false)
    : Value(p, l),
      values_(vector<Value*>()),
      comma_separated_(c),
      arglist_(a)
    { values_.reserve(s); }

    bool is_comma_separated() const { return comma_separated_; }
    bool is_arglist() const         { return arglist_; }

    bool is_comma_separated(bool c) { return comma_separated_ = c; }
    bool is_arglist(bool a)         { return arglist_ = a; }

    size_t size() const
    { return values_.size(); }

    Value*& operator[](size_t i)
    { return values_[i]; }

    List& operator<<(Value* v)
    {
      values_.push_back(v);
      return *this;
    }

    List& operator+=(List* l)
    {
      for (size_t i = 0, S = size(); i < S; ++i)
        values_.push_back((*l)[i]);
      return *this;
    }

  };


  /////////////////////////////////////////////////////////////////////////////
  // Abstract base class for binary operations. Represents logical, relational,
  // and arithmetic operations.
  /////////////////////////////////////////////////////////////////////////////
  class Binary_Operation : Value {

    Value* left_;
    Value* right_;

  public:

    Binary_Operation(string p, size_t l, Value* lhs, Value* rhs)
    : Value(p, l),
      left_(lhs),
      right_(rhs)
    { }

    virtual ~Binary_Operation() = 0;

    Value* left() const  { return left_; }
    Value* right() const { return right_; }

    Value* left(Value* lhs)  { return left_ = lhs; }
    Value* right(Value* rhs) { return right_ = rhs; }

  };

}