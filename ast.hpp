#define SASS_AST

#define ATTACH_OPERATIONS()\
virtual void perform(Operation<void>* op)      { (*op)(this); }\
virtual AST_Node* perform(Operation<AST_Node*>* op) { return (*op)(this); }

#define ADD_PROPERTY(type, name)\
private:\
  type name##_;\
public:\
  type name() const        { return name##_; }\
  type name(type name##__) { return name##_ = name##__; }\
private:

#include <string>
#include <sstream>
#include <vector>

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

namespace Sass {
  using namespace std;

  /////////////////////////////////////////////////////////////////////////////
  // Mixin class for AST nodes that should behave like vectors. Uses the
  // "Template Method" design pattern to allow subclasses to adjust their flags
  // when certain objects are pushed.
  /////////////////////////////////////////////////////////////////////////////
  template <typename T>
  class Vectorized {
    vector<T> elements_;
  protected:
    virtual void adjust_after_pushing(T element) { }
  public:
    Vectorized(size_t s = 0) : elements_(vector<T>())
    { elements_.reserve(s); }
    virtual ~Vectorized() = 0;
    size_t length() const   { return elements_.size(); }
    T& operator[](size_t i) { return elements_[i]; }
    Vectorized& operator<<(T element)
    {
      elements_.push_back(element);
      adjust_after_pushing(element);
      return *this;
    }
    Vectorized& operator+=(Vectorized* v)
    {
      for (size_t i = 0, L = v->length(); i < L; ++i) *this << (*v)[i];
      return *this;
    }
  };
  template <typename T>
  inline Vectorized<T>::~Vectorized() { }

  //////////////////////////////////////////////////////////
  // Abstract base class for all abstract syntax tree nodes.
  //////////////////////////////////////////////////////////
  class AST_Node {
    ADD_PROPERTY(string, path);
    ADD_PROPERTY(size_t, line);
  public:
    AST_Node(string p, size_t l) : path_(p), line_(l) { }
    virtual ~AST_Node() = 0;
    ATTACH_OPERATIONS();
  };
  inline AST_Node::~AST_Node() { }

  /////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements. This side of the AST hierarchy
  // represents elements in expansion contexts, which exist primarily to be
  // rewritten and macro-expanded.
  /////////////////////////////////////////////////////////////////////////
  class Statement : public AST_Node {
  public:
    Statement(string p, size_t l) : AST_Node(p, l) { }
    virtual ~Statement() = 0;
    // needed for rearranging nested rulesets during CSS emission
    virtual bool is_hoistable() { return false; }
  };
  inline Statement::~Statement() { }

  ////////////////////////
  // Blocks of statements.
  ////////////////////////
  class Block : public Statement, public Vectorized<Statement*> {
    ADD_PROPERTY(bool, is_root);
    // needed for properly formatted CSS emission
    ADD_PROPERTY(bool, has_hoistable);
    ADD_PROPERTY(bool, has_non_hoistable);
  protected:
    void adjust_after_pushing(Statement* s)
    {
      if (s->is_hoistable()) has_hoistable_     = true;
      else                   has_non_hoistable_ = false;
    };
  public:
    Block(string p, size_t l, size_t s = 0, bool r = false)
    : Statement(p, l),
      Vectorized(s),
      is_root_(r), has_hoistable_(false), has_non_hoistable_(false)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements that contain blocks of statements.
  ////////////////////////////////////////////////////////////////////////
  class Has_Block : public Statement {
    ADD_PROPERTY(Block*, block);
  public:
    Has_Block(string p, size_t l, Block* b)
    : Statement(p, l), block_(b)
    { }
    virtual ~Has_Block() = 0;
  };
  inline Has_Block::~Has_Block() { }

  /////////////////////////////////////////////////////////////////////////////
  // Rulesets (i.e., sets of styles headed by a selector and containing a block
  // of style declarations.
  /////////////////////////////////////////////////////////////////////////////
  class Selector;
  class Ruleset : public Has_Block {
    ADD_PROPERTY(Selector*, selector);
  public:
    Ruleset(string p, size_t l, Selector* s, Block* b)
    : Has_Block(p, l, b), selector_(s)
    { }
    // nested rulesets need to be hoisted out of their enclosing blocks
    bool is_hoistable() { return true; }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////////////////////////
  // Nested declaration sets (i.e., namespaced properties).
  /////////////////////////////////////////////////////////
  class String;
  class Propset : public Has_Block {
    ADD_PROPERTY(String*, property_fragment);
  public:
    Propset(string p, size_t l, String* pf, Block* b)
    : Has_Block(p, l, b), property_fragment_(pf)
    { }
    ATTACH_OPERATIONS();
  };

  /////////////////
  // Media queries.
  /////////////////
  class List;
  class Media_Query : public Has_Block {
    ADD_PROPERTY(List*, query_list);
  public:
    Media_Query(string p, size_t l, List* q, Block* b)
    : Has_Block(p, l, b), query_list_(q)
    { }
    ATTACH_OPERATIONS();
  };

  ///////////////////////////////////////////////////////////////////////
  // At-rules -- arbitrary directives beginning with "@" that may have an
  // optional statement block.
  ///////////////////////////////////////////////////////////////////////
  class At_Rule : public Has_Block {
    ADD_PROPERTY(string, keyword);
    ADD_PROPERTY(Selector*, selector);
  public:
    At_Rule(string p, size_t l, string kwd, Selector* sel, Block* b)
    : Has_Block(p, l, b), keyword_(kwd), selector_(sel)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////////////////
  // Declarations -- style rules consisting of a property name and values.
  ////////////////////////////////////////////////////////////////////////
  class Declaration : public Statement {
    ADD_PROPERTY(String*, property);
    ADD_PROPERTY(List*, values);
  public:
    Declaration(string p, size_t l, String* prop, List* vals)
    : Statement(p, l), property_(prop), values_(vals)
    { }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////
  // Assignments -- variable and value.
  /////////////////////////////////////
  class Variable;
  class Expression;
  class Assignment : public Statement {
    ADD_PROPERTY(string, variable);
    ADD_PROPERTY(Expression*, value);
    ADD_PROPERTY(bool, is_guarded);
  public:
    Assignment(string p, size_t l,
               string var, Expression* val, bool guarded = false)
    : Statement(p, l), variable_(var), value_(val), is_guarded_(guarded)
    { }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////////////////////////////////////////////
  // CSS import directives. CSS url imports need to be distinguished from Sass
  // file imports. T should be instantiated with Function_Call or String.
  /////////////////////////////////////////////////////////////////////////////
  template <typename T>
  class Import : public Statement {
    ADD_PROPERTY(T*, location);
  public:
    Import(string p, size_t l, T* loc)
    : Statement(p, l), location_(loc)
    { }
    ATTACH_OPERATIONS();
  };

  //////////////////////////////
  // The Sass `@warn` directive.
  //////////////////////////////
  class Warning : public Statement {
    ADD_PROPERTY(Expression*, message);
  public:
    Warning(string p, size_t l, Expression* msg)
    : Statement(p, l), message_(msg)
    { }
    ATTACH_OPERATIONS();
  };

  ///////////////////////////////////////////
  // CSS comments. These may be interpolated.
  ///////////////////////////////////////////
  class Comment : public Statement {
    ADD_PROPERTY(String*, text);
  public:
    Comment(string p, size_t l, String* txt)
    : Statement(p, l), text_(txt)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////
  // The Sass `@if` control directive.
  ////////////////////////////////////
  class If : public Statement {
    ADD_PROPERTY(Expression*, predicate);
    ADD_PROPERTY(Block*, consequent);
    ADD_PROPERTY(Block*, alternative);
  public:
    If(string p, size_t l, Expression* pred, Block* con, Block* alt = 0)
    : Statement(p, l), predicate_(pred), consequent_(con), alternative_(alt)
    { }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////
  // The Sass `@for` control directive.
  /////////////////////////////////////
  class For : public Has_Block {
    ADD_PROPERTY(string, variable);
    ADD_PROPERTY(Expression*, lower_bound);
    ADD_PROPERTY(Expression*, upper_bound);
    ADD_PROPERTY(bool, is_inclusive);
  public:
    For(string p, size_t l,
        string var, Expression* lo, Expression* hi, Block* b, bool inc)
    : Has_Block(p, l, b),
      variable_(var), lower_bound_(lo), upper_bound_(hi), is_inclusive_(inc)
    { }
    ATTACH_OPERATIONS();
  };

  //////////////////////////////////////
  // The Sass `@each` control directive.
  //////////////////////////////////////
  class Each : public Has_Block {
    ADD_PROPERTY(string, variable);
    ADD_PROPERTY(Expression*, list);
  public:
    Each(string p, size_t l, string var, Expression* lst, Block* b)
    : Has_Block(p, l, b), variable_(var), list_(lst)
    { }
    ATTACH_OPERATIONS();
  };

  ///////////////////////////////////////
  // The Sass `@while` control directive.
  ///////////////////////////////////////
  class While : public Has_Block {
    ADD_PROPERTY(Expression*, predicate);
  public:
    While(string p, size_t l, Expression* pred, Block* b)
    : Has_Block(p, l, b), predicate_(pred)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////
  // The Sass `@extend` directive.
  ////////////////////////////////
  class Extend : public Statement {
    ADD_PROPERTY(Selector*, selector);
  public:
    Extend(string p, size_t l, Selector* s)
    : Statement(p, l), selector_(s)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////////////////////
  // Definitions for both mixins and functions. Templatized to avoid type-tags
  // and unnecessary subclassing.
  ////////////////////////////////////////////////////////////////////////////
  class Parameters;
  enum Definition_Type { MIXIN, FUNCTION };
  template <Definition_Type t>
  class Definition : public Has_Block {
    ADD_PROPERTY(string, name);
    ADD_PROPERTY(Parameters*, parameters);
  public:
    Definition(string p, size_t l,
               string n, Parameters* params, Block* b)
    : Has_Block(p, l, b), name_(n), parameters_(params)
    { }
    ATTACH_OPERATIONS();
  };

  //////////////////////////////////////
  // Mixin calls (i.e., `@include ...`).
  //////////////////////////////////////
  class Arguments;
  class Mixin_Call : public Has_Block {
    ADD_PROPERTY(string, name);
    ADD_PROPERTY(Arguments*, arguments);
  public:
    Mixin_Call(string p, size_t l, string n, Arguments* args, Block* b = 0)
    : Has_Block(p, l, b), name_(n), arguments_(args)
    { }
    ATTACH_OPERATIONS();
  };

  //////////////////////////////////////////////////////////////////////
  // Abstract base class for expressions. This side of the AST hierarchy
  // represents elements in value contexts, which exist primarily to be
  // evaluated and returned.
  //////////////////////////////////////////////////////////////////////
  class Expression : public AST_Node {
    // expressions in some contexts shouldn't be evaluated
    ADD_PROPERTY(bool, is_delayed);
    // for media features, if I recall
    ADD_PROPERTY(bool, is_parenthesized);
  public:
    Expression(string p, size_t l)
    : AST_Node(p, l), is_delayed_(false), is_parenthesized_(false)
    { }
    virtual ~Expression() = 0;
    virtual string type() { return ""; /* TODO: raise an error */ }
  };
  inline Expression::~Expression() { }

  ///////////////////////////////////////////////////////////////////////
  // Lists of values, both comma- and space-separated (distinguished by a
  // type-tag.) Also used to represent variable-length argument lists.
  ///////////////////////////////////////////////////////////////////////
  class List : public Expression, public Vectorized<Expression*> {
  public:
    enum Separator { space, comma };
  private:
    ADD_PROPERTY(Separator, separator);
    ADD_PROPERTY(bool, is_arglist);
  public:
    List(string p, size_t l,
         size_t size = 0, Separator sep = space, bool argl = false)
    : Expression(p, l),
      Vectorized(size),
      separator_(sep), is_arglist_(argl)
    { }
    string type() { return is_arglist_ ? "arglist" : "list"; }
    ATTACH_OPERATIONS();
  };

  //////////////////////////////////////////////////////////////////////////
  // Binary expressions. Represents logical, relational, and arithmetic
  // operations. Templatized to avoid large switch statements and repetitive
  // subclassing.
  //////////////////////////////////////////////////////////////////////////
  enum Binary_Operator {
    AND, OR,                   // logical connectives
    EQ, NEQ, GT, GTE, LT, LTE, // arithmetic relations
    ADD, SUB, MUL, DIV         // arithmetic functions
  };
  template<Binary_Operator oper>
  class Binary_Expression : public Expression {
    ADD_PROPERTY(Expression*, left);
    ADD_PROPERTY(Expression*, right);
  public:
    Binary_Expression(string p, size_t l, Expression* lhs, Expression* rhs)
    : Expression(p, l), left_(lhs), right_(rhs)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////////////////////
  // Arithmetic negation (logical negation is just an ordinary function call).
  ////////////////////////////////////////////////////////////////////////////
  class Negation : public Expression {
    ADD_PROPERTY(Expression*, operand);
  public:
    Negation(string p, size_t l, Expression* o)
    : Expression(p, l), operand_(o)
    { }
    ATTACH_OPERATIONS();
  };

  //////////////////
  // Function calls.
  //////////////////
  class Function_Call : public Expression {
    ADD_PROPERTY(String*, name);
    ADD_PROPERTY(Arguments*, arguments);
  public:
    Function_Call(string p, size_t l, String* n, Arguments* args)
    : Expression(p, l), name_(n), arguments_(args)
    { }
    ATTACH_OPERATIONS();
  };

  ///////////////////////
  // Variable references.
  ///////////////////////
  class Variable : public Expression {
    ADD_PROPERTY(string, name);
  public:
    Variable(string p, size_t l, string n)
    : Expression(p, l), name_(n)
    { }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////////////////////////////////////////////
  // Textual (i.e., unevaluated) numeric data. Templated to avoid type-tags and
  // repetitive subclassing.
  /////////////////////////////////////////////////////////////////////////////
  enum Textual_Type { NUMBER, PERCENTAGE, DIMENSION, HEX };
  template <Textual_Type t>
  class Textual : public Expression {
    ADD_PROPERTY(string, value);
  public:
    Textual(string p, size_t l, string val)
    : Expression(p, l), value_(val)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////
  // Numbers, percentages, dimensions, and colors.
  ////////////////////////////////////////////////
  class Numeric : public Expression {
    ADD_PROPERTY(double, value);
  public:
    Numeric(string p, size_t l, double val) : Expression(p, l), value_(val) { }
    virtual ~Numeric() = 0;
    string type() { return "number"; }
  };
  inline Numeric::~Numeric() { }
  class Number : public Numeric {
    Number(string p, size_t l, double val) : Numeric(p, l, val) { }
    ATTACH_OPERATIONS();
  };
  class Percentage : public Numeric {
    Percentage(string p, size_t l, double val) : Numeric(p, l, val) { }
    ATTACH_OPERATIONS();
  };
  class Dimension : public Numeric {
    vector<string> numerator_units_;
    vector<string> denominator_units_;
  public:
    Dimension(string p, size_t l, double val, string unit)
    : Numeric(p, l, val),
      numerator_units_(vector<string>()),
      denominator_units_(vector<string>())
    { numerator_units_.push_back(unit); }
    vector<string>& numerator_units()   { return numerator_units_; }
    vector<string>& denominator_units() { return denominator_units_; }
    ATTACH_OPERATIONS();
  };

  //////////
  // Colors.
  //////////
  class Color : public Expression {
    ADD_PROPERTY(double, r);
    ADD_PROPERTY(double, g);
    ADD_PROPERTY(double, b);
    ADD_PROPERTY(double, a);
  public:
    Color(string p, size_t l, double r, double g, double b, double a = 1)
    : Expression(p, l), r_(r), g_(g), b_(b), a_(a)
    { }
    string type() { return "color"; }
    ATTACH_OPERATIONS();
  };

  ////////////
  // Booleans.
  ////////////
  class Boolean : public Expression {
    ADD_PROPERTY(bool, value);
  public:
    Boolean(string p, size_t l, bool val) : Expression(p, l), value_(val) { }
    string type() { return "bool"; }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////////////////
  // Abstract base class for Sass string values. Includes interpolated and
  // "flat" strings.
  ////////////////////////////////////////////////////////////////////////
  class String : public Expression {
  public:
    String(string p, size_t l) : Expression(p, l) { }
    virtual ~String() = 0;
  };
  inline String::~String() { };

  ///////////////////////////////////////////////////////////////////////
  // Interpolated strings. Meant to be reduced to flat strings during the
  // evaluation phase.
  ///////////////////////////////////////////////////////////////////////
  class Interpolation : public String, public Vectorized<Expression*> {
  public:
    Interpolation(string p, size_t l, size_t size = 0)
    : String(p, l), Vectorized(size)
    { }
    string type() { return "string"; }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////
  // Flat strings -- the lowest level of raw textual data.
  ////////////////////////////////////////////////////////
  class Flat_String : public String {
    ADD_PROPERTY(string, value);
  public:
    Flat_String(string p, size_t l, string val)
    : String(p, l), value_(val)
    { }
    Flat_String(string p, size_t l, const char* beg)
    : String(p, l), value_(string(beg))
    { }
    Flat_String(string p, size_t l, const char* beg, const char* end)
    : String(p, l), value_(string(beg, end-beg))
    { }
    string type() { return "string"; }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////
  // Media expressions (for use inside media queries).
  ////////////////////////////////////////////////////
  class Media_Expression : public Expression {
    ADD_PROPERTY(String*, feature);
    ADD_PROPERTY(Expression*, value);
  public:
    Media_Expression(string p, size_t l, String* f, Expression* v)
    : Expression(p, l), feature_(f), value_(v)
    { }
  };

  /////////////////////////////////////////////////////////
  // Individual parameter objects for mixins and functions.
  /////////////////////////////////////////////////////////
  class Parameter : public AST_Node {
    ADD_PROPERTY(string, name);
    ADD_PROPERTY(Expression*, default_value);
    ADD_PROPERTY(bool, is_rest_parameter);
  public:
    Parameter(string p, size_t l,
              string n, Expression* def = 0, bool rest = false)
    : AST_Node(p, l), name_(n), default_value_(def), is_rest_parameter_(rest)
    { /* TO-DO: error if default_value && is_packed */ }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////////////////////////////////////////
  // Parameter lists -- in their own class to facilitate context-sensitive
  // error checking (e.g., ensuring that all optional parameters follow all
  // required parameters).
  /////////////////////////////////////////////////////////////////////////
  class Parameters : public AST_Node, public Vectorized<Parameter*> {
    ADD_PROPERTY(bool, has_optional_parameters);
    ADD_PROPERTY(bool, has_rest_parameter);
  protected:
    void adjust_after_pushing(Parameter* p)
    {
      if (p->default_value()) {
        if (has_rest_parameter_)
          ; // error
        has_optional_parameters_ = true;
      }
      else if (p->is_rest_parameter()) {
        if (has_rest_parameter_)
          ; // error
        if (has_optional_parameters_)
          ; // different error
        has_rest_parameter_ = true;
      }
      else {
        if (has_rest_parameter_)
          ; // error
        if (has_optional_parameters_)
          ; // different error
      }
    }
  public:
    Parameters(string p, size_t l)
    : AST_Node(p, l),
      Vectorized(),
      has_optional_parameters_(false),
      has_rest_parameter_(false)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////
  // Individual argument objects for mixin and function calls.
  ////////////////////////////////////////////////////////////
  class Argument : public AST_Node {
    ADD_PROPERTY(Expression*, value);
    ADD_PROPERTY(string, name);
    ADD_PROPERTY(bool, is_rest_argument);
  public:
    Argument(string p, size_t l, Expression* val, string n = "", bool rest = false)
    : AST_Node(p, l), value_(val), name_(n), is_rest_argument_(rest)
    { if (name_ != "" && is_rest_argument_) { /* error */ } }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////////////////
  // Argument lists -- in their own class to facilitate context-sensitive
  // error checking (e.g., ensuring that all ordinal arguments precede all
  // named arguments).
  ////////////////////////////////////////////////////////////////////////
  class Arguments : public AST_Node, Vectorized<Argument*> {
    ADD_PROPERTY(bool, has_named_arguments);
    ADD_PROPERTY(bool, has_rest_argument);
  protected:
    void adjust_after_pushing(Argument* a)
    {
      if (!a->name().empty()) {
        if (has_rest_argument_)
          ; // error
        has_named_arguments_ = true;
      }
      else if (a->is_rest_argument()) {
        if (has_rest_argument_)
          ; // error
        if (has_named_arguments_)
          ; // different error
        has_rest_argument_ = true;
      }
      else {
        if (has_rest_argument_)
          ; // error
        if (has_named_arguments_)
          ; // error
      }
    }
  public:
    Arguments(string p, size_t l)
    : AST_Node(p, l),
      Vectorized(),
      has_named_arguments_(false),
      has_rest_argument_(false)
    { }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////////
  // Abstract base class for CSS selectors.
  /////////////////////////////////////////
  class Selector : public AST_Node {
  public:
    Selector(string p, size_t l) : AST_Node(p, l) { }
    virtual ~Selector() = 0;
  };
  inline Selector::~Selector() { }

  /////////////////////////////////////////////////////////////////////////
  // Interpolated selectors -- the interpolated String will be expanded and
  // re-parsed into a normal selector classure.
  /////////////////////////////////////////////////////////////////////////
  class Interpolated_Selector : Selector {
    ADD_PROPERTY(String*, contents);
  public:
    Interpolated_Selector(string p, size_t l, String* c)
    : Selector(p, l), contents_(c)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////
  // Abstract base class for simple selectors.
  ////////////////////////////////////////////
  class Simple_Base : public Selector {
  public:
    Simple_Base(string p, size_t l) : Selector(p, l) { }
    virtual ~Simple_Base() = 0;
  };
  inline Simple_Base::~Simple_Base() { }

  //////////////////////////////////////////////////////////////////////
  // Normal simple selectors (e.g., "div", ".foo", ":first-child", etc).
  //////////////////////////////////////////////////////////////////////
  class Simple_Selector : public Simple_Base {
    ADD_PROPERTY(string, contents);
  public:
    Simple_Selector(string p, size_t l, string c)
    : Simple_Base(p, l), contents_(c)
    { }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////
  // Parent references (i.e., the "&").
  /////////////////////////////////////
  class Reference_Selector : public Simple_Base {
    ADD_PROPERTY(Selector*, selector);
  public:
    Reference_Selector(string p, size_t l) : Simple_Base(p, l) { }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////////////////////////////////////////
  // Placeholder selectors (e.g., "%foo") for use in extend-only selectors.
  /////////////////////////////////////////////////////////////////////////
  class Placeholder_Selector : public Simple_Base {
    ADD_PROPERTY(string, name);
  public:
    Placeholder_Selector(string p, size_t l, string n)
    : Simple_Base(p, l), name_(n)
    { }
    ATTACH_OPERATIONS();
  };

  //////////////////////////////////////////////////////////////////
  // Pseudo selectors -- e.g., :first-child, :nth-of-type(...), etc.
  //////////////////////////////////////////////////////////////////
  class Pseudo_Selector : public Simple_Base {
    ADD_PROPERTY(string, name);
    ADD_PROPERTY(Expression*, expression);
  public:
    Pseudo_Selector(string p, size_t l, string n, Expression* expr = 0)
    : Simple_Base(p, l), name_(n), expression_(expr)
    { }
    ATTACH_OPERATIONS();
  };

  /////////////////////////////////////////////////
  // Negated selector -- e.g., :not(:first-of-type)
  /////////////////////////////////////////////////
  class Negated_Selector : public Simple_Base {
    ADD_PROPERTY(Simple_Base*, selector);
  public:
    Negated_Selector(string p, size_t l, Simple_Base* sel)
    : Simple_Base(p, l), selector_(sel)
    { }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////////////////////
  // Simple selector sequences. Maintains flags indicating whether it contains
  // any parent references or placeholders, to simplify expansion.
  ////////////////////////////////////////////////////////////////////////////
  class Selector_Sequence : public Selector, public Vectorized<Simple_Base*> {
    ADD_PROPERTY(bool, has_reference);
    ADD_PROPERTY(bool, has_placeholder);
  public:
    Selector_Sequence(string p, size_t l, size_t s = 0)
    : Selector(p, l),
      Vectorized(s),
      has_reference_(false),
      has_placeholder_(false)
    { }
    Selector_Sequence& operator<<(Reference_Selector* s)
    {
      has_reference_ = true;
      return (*this) << s;
    }
    Selector_Sequence& operator<<(Placeholder_Selector* p)
    {
      has_placeholder_ = true;
      return (*this) << p;
    }
    ATTACH_OPERATIONS();
  };

  ////////////////////////////////////////////////////////////////////////////
  // General selectors -- i.e., simple sequences combined with one of the four
  // CSS selector combinators (">", "+", "~", and whitespace). Essentially a
  // left-associative linked list.
  ////////////////////////////////////////////////////////////////////////////
  class Selector_Combination : public Selector {
  public:
    enum Combinator { ANCESTOR_OF, PARENT_OF, PRECEDES, ADJACENT_TO };
  private:
    ADD_PROPERTY(Combinator, combinator);
    ADD_PROPERTY(Selector_Combination*, context);
    ADD_PROPERTY(Selector_Sequence*, base);
    ADD_PROPERTY(bool, has_reference);
    ADD_PROPERTY(bool, has_placeholder);
  public:
    Selector_Combination(string p, size_t l,
                         Combinator c,
                         Selector_Combination* ctx,
                         Selector_Sequence* sel)
    : Selector(p, l),
      combinator_(c),
      context_(ctx),
      base_(sel),
      has_reference_(ctx && ctx->has_reference() ||
                     sel && sel->has_reference()),
      has_placeholder_(ctx && ctx->has_placeholder() ||
                       sel && sel->has_placeholder())
    { }
    ATTACH_OPERATIONS();
  };

  ///////////////////////////////////
  // Comma-separated selector groups.
  ///////////////////////////////////
  class Selector_Group
      : public Selector, public Vectorized<Selector_Combination*> {
    ADD_PROPERTY(bool, has_reference);
    ADD_PROPERTY(bool, has_placeholder);
  protected:
    void adjust_after_pushing(Selector_Combination* c)
    {
      has_reference_   |= c->has_reference();
      has_placeholder_ |= c->has_placeholder();
    }
  public:
    Selector_Group(string p, size_t l, size_t s = 0)
    : Selector(p, l),
      Vectorized(s),
      has_reference_(false),
      has_placeholder_(false)
    { }
    ATTACH_OPERATIONS();
  };
}