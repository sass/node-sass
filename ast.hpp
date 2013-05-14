#define SASS_AST

#include <string>
#include <vector>

namespace Sass {
  using namespace std;

  //////////////////////////////////////////////////////////
  // Abstract base class for all abstract syntax tree nodes.
  //////////////////////////////////////////////////////////
  struct AST_Node {
    string path;
    size_t line;

    AST_Node(string p, size_t l) : path(p), line(l) { }
    virtual ~AST_Node() = 0;
  };

  /////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements. This side of the AST hierarchy
  // represents elements in expansion contexts, which exist primarily to be
  // rewritten and macro-expanded.
  /////////////////////////////////////////////////////////////////////////
  struct Statement : public AST_Node {
    Statement(string p, size_t l) : AST_Node(p, l) { }
    virtual ~Statement() = 0;
  };

  ////////////////////////
  // Blocks of statements.
  ////////////////////////
  struct Block : public Statement {
    vector<Statement*> statements;
    bool               is_root;

    Block(string p, size_t l, size_t size = 0, bool root = false)
    : Statement(p, l), statements(vector<Statement*>()), is_root(root)
    { statements.reserve(size); }

    size_t length() const
    { return statements.size(); }
    Statement*& operator[](size_t i)
    { return statements[i]; }
    Block& operator<<(Statement* s)
    {
      statements.push_back(s);
      return *this;
    }
    Block& operator+=(Block* b)
    {
      for (size_t i = 0, L = length(); i < L; ++i)
        statements.push_back((*b)[i]);
      return *this;
    }
  };

  ////////////////////////////////////////////////////////////////////////
  // Abstract base class for statements that contain blocks of statements.
  ////////////////////////////////////////////////////////////////////////
  struct Has_Block : public Statement {
    Block* block;
    Has_Block(string p, size_t l, Block* b)
    : Statement(p, l), block(b)
    { }
    virtual ~Has_Block() = 0;
  };

  /////////////////////////////////////////////////////////////////////////////
  // Rulesets (i.e., sets of styles headed by a selector and containing a block
  // of style declarations.
  /////////////////////////////////////////////////////////////////////////////
  struct Selector;
  struct Ruleset : public Has_Block {
    Selector* selector;

    Ruleset(string p, size_t l, Selector* s, Block* b)
    : Has_Block(p, l, b), selector(s)
    { }
  };

  /////////////////////////////////////////////////////////
  // Nested declaration sets (i.e., namespaced properties).
  /////////////////////////////////////////////////////////
  struct String;
  struct Propset : public Has_Block {
    String* property_fragment;

    Propset(string p, size_t l, String* pf, Block* b)
    : Has_Block(p, l, b), property_fragment(pf)
    { }
  };

  /////////////////
  // Media queries.
  /////////////////
  struct Value;
  struct Media_Query : public Has_Block {
    Value* query;

    Media_Query(string p, size_t l, Value* q, Block* b)
    : Has_Block(p, l, b), query(q)
    { }
  };

  /////////////////////////////////////////////////////////////////////
  // At-rules -- arbitrary directives with an optional statement block.
  /////////////////////////////////////////////////////////////////////
  // TBD

  ////////////////////////////////////////////////////////////////////////
  // Declarations -- style rules consisting of a property name and values.
  ////////////////////////////////////////////////////////////////////////
  struct List;
  struct Declaration : public Statement {
    String* property;
    List*   values;

    Declaration(string p, size_t l, String* prop, List* vals)
    : Statement(p, l), property(prop), values(vals)
    { }
  };

  /////////////////////////////////////
  // Assignments -- variable and value.
  /////////////////////////////////////
  struct Variable;
  struct Assignment : public Statement {
    string variable;
    Value* value;
    bool   is_guarded;

    Assignment(string p, size_t l,
               string var, Value* val, bool guarded = false)
    : Statement(p, l), variable(var), value(val), is_guarded(guarded)
    { }
  };

  ////////////////////////
  // CSS import directives
  ////////////////////////
  struct Import : public Statement {
    String* location;

    Import(string p, size_t l, String* loc)
    : Statement(p, l), location(loc)
    { }
  };

  //////////////////////////////
  // The Sass `@warn` directive.
  //////////////////////////////
  struct Warning : public Statement {
    String* message;

    Warning(string p, size_t l, String* msg)
    : Statement(p, l), message(msg)
    { }
  };

  ///////////////////////////////////////////
  // CSS comments. These may be interpolated.
  ///////////////////////////////////////////
  struct Comment : public Statement {
    String* text;

    Comment(string p, size_t l, String* txt)
    : Statement(p, l), text(txt)
    { }
  };

  ////////////////////////////////////
  // The Sass `@if` control directive.
  ////////////////////////////////////
  struct If : public Statement {
    Value* predicate;
    Block* consequent;
    Block* alternative;

    If(string p, size_t l, Value* pred, Block* con, Block* alt = 0)
    : Statement(p, l), predicate(pred), consequent(con), alternative(alt)
    { }
  };

  /////////////////////////////////////
  // The Sass `@for` control directive.
  /////////////////////////////////////
  struct For : public Has_Block {
    string variable;
    Value* lower_bound;
    Value* upper_bound;
    bool   is_inclusive;

    For(string p, size_t l,
        string var, Value* lo, Value* hi, Block* b, bool inc)
    : Has_Block(p, l, b),
      variable(var), lower_bound(lo), upper_bound(hi), is_inclusive(inc)
    { }
  };

  //////////////////////////////////////
  // The Sass `@each` control directive.
  //////////////////////////////////////
  struct Each : public Has_Block {
    string variable;
    Value* list;

    Each(string p, size_t l, string var, Value* lst, Block* b)
    : Has_Block(p, l, b), variable(var), list(lst)
    { }
  };

  ///////////////////////////////////////
  // The Sass `@while` control directive.
  ///////////////////////////////////////
  struct While : public Has_Block {
    Value* predicate;

    While(string p, size_t l, Value* pred, Block* b)
    : Has_Block(p, l, b), predicate(pred)
    { }
  };

  ////////////////////////////////
  // The Sass `@extend` directive.
  ////////////////////////////////
  struct Extend : public Statement {
    Selector* selector;

    Extend(string p, size_t l, Selector* s)
    : Statement(p, l), selector(s)
    { }
  };

  ////////////////////////////////////////////////////////////////////////////
  // Definitions for both mixins and functions. Templatized to avoid type-tags
  // and unnecessary subclassing.
  ////////////////////////////////////////////////////////////////////////////
  struct Parameters;
  enum Definition_Type { MIXIN, FUNCTION };
  template <Definition_Type t>
  struct Definition : public Has_Block {
    string      name;
    Parameters* parameters;

    Definition(string p, size_t l,
               string n, Parameters* params, Block* b)
    : Has_Block(p, l, b), name(n), parameters(params)
    { }
  };

  //////////////////////////////////////
  // Mixin calls (i.e., `@include ...`).
  //////////////////////////////////////
  struct Arguments;
  struct Mixin_Call : public Has_Block {
    string name;
    Arguments* arguments;

    Mixin_Call(string p, size_t l, string n, Arguments* args, Block* b = 0)
    : Has_Block(p, l, b), name(n), arguments(args)
    { }
  };

  /////////////////////////////////////////////////////////////////////////////
  // Abstract base class for values. This side of the AST hierarchy represents
  // elements in evaluation contexts, which exist primarily to be evaluated and
  // returned.
  /////////////////////////////////////////////////////////////////////////////
  struct Value : public AST_Node {
    bool delayed;
    bool parenthesized;

    Value(string p, size_t l)
    : AST_Node(p, l), delayed(false), parenthesized(false)
    { }
    virtual ~Value() = 0;
  };

  ///////////////////////////////////////////////////////////////////////
  // Lists of values, both comma- and space-separated (distinguished by a
  // type-tag.) Also used to represent variable-length argument lists.
  ///////////////////////////////////////////////////////////////////////
  struct List : public Value {
    enum Separator { space, comma };
    vector<Value*> values;
    Separator      separator;
    bool           is_arglist;

    List(string p, size_t l,
         size_t size = 0, Separator sep = space, bool argl = false)
    : Value(p, l),
      values(vector<Value*>()), separator(sep), is_arglist(argl)
    { values.reserve(size); }

    size_t length() const
    { return values.size(); }
    Value*& operator[](size_t i)
    { return values[i]; }
    List& operator<<(Value* v)
    {
      values.push_back(v);
      return *this;
    }
    List& operator+=(List* l)
    {
      for (size_t i = 0, L = length(); i < L; ++i)
        values.push_back((*l)[i]);
      return *this;
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  // Abstract base class for binary operations. Represents logical, relational,
  // and arithmetic operations. Templatized to avoid large switch statements
  // and repetitive subclassing.
  /////////////////////////////////////////////////////////////////////////////
  enum Binary_Operator {
    AND, OR,                   // logical connectives
    EQ, NEQ, GT, GTE, LT, LTE, // arithmetic relations
    ADD, SUB, MUL, DIV         // arithmetic functions
  };
  template<Binary_Operator op>
  struct Binary_Expression : public Value {
    Value* left;
    Value* right;

    Binary_Expression(string p, size_t l, Value* lhs, Value* rhs)
    : Value(p, l), left(lhs), right(rhs)
    { }
  };

  ////////////////////////////////////////////////////////////////////////////
  // Arithmetic negation (logical negation is just an ordinary function call).
  ////////////////////////////////////////////////////////////////////////////
  struct Negation : public Value {
    Value* operand;
    Negation(string p, size_t l, Value* o)
    : Value(p, l), operand(o)
    { }
  };

  //////////////////
  // Function calls.
  //////////////////
  struct Function_Call : public Value {
    String*    name;
    Arguments* arguments;

    Function_Call(string p, size_t l, String* n, Arguments* args)
    : Value(p, l), name(n), arguments(args)
    { }
  };

  ///////////////////////
  // Variable references.
  ///////////////////////
  struct Variable : public Value {
    string name;

    Variable(string p, size_t l, string n)
    : Value(p, l), name(n)
    { }
  };

  ///////////////////////////////////////////////////////////////////////////
  // Textual (i.e., unevaluated) numeric data. Distinguished with a type-tag.
  ///////////////////////////////////////////////////////////////////////////
  struct Textual_Numeric : public Value {
    enum Type { number, percentage, dimension, hex };
    string value;
    Type type;

    Textual_Numeric(string p, size_t l, Type t, string val)
    : Value(p, l), value(val), type(t)
    { }
  };

  ////////////////////////////////////////////////
  // Numbers, percentages, dimensions, and colors.
  ////////////////////////////////////////////////
  struct Number : public Value {
    double value;
    Number(string p, size_t l, double val) : Value(p, l), value(val) { }
  };
  struct Percentage : public Value {
    double value;
    Percentage(string p, size_t l, double val) : Value(p, l), value(val) { }
  };
  struct Dimension : public Value {
    double value;
    vector<string> numerator_units;
    vector<string> denominator_units;
    Dimension(string p, size_t l, double val, string unit)
    : Value(p, l),
      value(val),
      numerator_units(vector<string>()),
      denominator_units(vector<string>())
    { numerator_units.push_back(unit); }
  };

  //////////
  // Colors.
  //////////
  struct Color : public Value {
    double r, g, b, a;
    Color(string p, size_t l, double r, double g, double b, double a = 1)
    : Value(p, l), r(r), g(g), b(b), a(a)
    { }
  };

  ////////////
  // Booleans.
  ////////////
  struct Boolean : public Value {
    bool value;
    Boolean(string p, size_t l, bool val) : Value(p, l), value(val) { }
  };

  ////////////////////////////////////////////////////////////////////////
  // Sass strings -- includes quoted strings, as well as all other literal
  // textual data (identifiers, interpolations, concatenations etc).
  ////////////////////////////////////////////////////////////////////////
  struct String : public Value {
    vector<Value*> fragments;
    bool is_quoted, is_interpolated;

    String(string p, size_t l, size_t size = 0, bool q = false, bool i = false)
    : Value(p, l),
      fragments(vector<Value*>()), is_quoted(q), is_interpolated(i)
    { fragments.reserve(size); }

    size_t length() const
    { return fragments.size(); }
    Value*& operator[](size_t i)
    { return fragments[i]; }
    String& operator<<(Value* v)
    {
      fragments.push_back(v);
      return *this;
    }
    String& operator+=(String* s)
    {
      for (size_t i = 0, L = length(); i < L; ++i)
        fragments.push_back((*s)[i]);
      return *this;
    }
  };

  ///////////////////////////////////////////////////////
  // Sass tokens -- the lowest level of raw textual data.
  ///////////////////////////////////////////////////////
  struct Token : public Value {
    string value;

    Token(string p, size_t l, string val) : Value(p, l), value(val) { }
    Token(string p, size_t l, const char* beg, const char* end)
    : Value(p, l), value(string(beg, end-beg))
    { }
  };

  /////////////////////////////////////////////////////////
  // Individual parameter objects for mixins and functions.
  /////////////////////////////////////////////////////////
  struct Parameter : public AST_Node {
    string name;
    Value* default_value;
    bool is_rest_parameter;

    Parameter(string p, size_t l,
              string n, Value* def = 0, bool rest = false)
    : AST_Node(p, l), name(n), default_value(def), is_rest_parameter(rest)
    { /* TO-DO: error if default_value && is_packed */ }
  };

  /////////////////////////////////////////////////////////////////////////
  // Parameter lists -- in their own class to facilitate context-sensitive
  // error checking (e.g., ensuring that all optional parameters follow all
  // required parameters).
  /////////////////////////////////////////////////////////////////////////
  struct Parameters : public AST_Node {
    vector<Parameter*> list;
    bool has_optional_parameters, has_rest_parameter;

    Parameters(string p, size_t l)
    : AST_Node(p, l),
      has_optional_parameters(false), has_rest_parameter(false)
    { }

    size_t length() { return list.size(); }
    Parameter*& operator[](size_t i) { return list[i]; }

    Parameters& operator<<(Parameter* p)
    {
      if (p->default_value) {
        if (has_rest_parameter)
          ; // error
        has_optional_parameters = true;
      }
      else if (p->is_rest_parameter) {
        if (has_rest_parameter)
          ; // error
        if (has_optional_parameters)
          ; // different error
        has_rest_parameter = true;
      }
      else {
        if (has_rest_parameter)
          ; // error
        if (has_optional_parameters)
          ; // different error
      }
      list.push_back(p);
      return *this;
    }
  };

  ////////////////////////////////////////////////////////////
  // Individual argument objects for mixin and function calls.
  ////////////////////////////////////////////////////////////
  struct Argument : public AST_Node {
    Value* value;
    string name;
    bool is_rest_argument;

    Argument(string p, size_t l, Value* val, string n = "", bool rest = false)
    : AST_Node(p, l), value(val), name(n), is_rest_argument(rest)
    { if (name != "" && is_rest_argument) { /* error */ } }
  };

  ////////////////////////////////////////////////////////////////////////
  // Argument lists -- in their own class to facilitate context-sensitive
  // error checking (e.g., ensuring that all ordinal arguments precede all
  // named arguments).
  ////////////////////////////////////////////////////////////////////////
  struct Arguments : public AST_Node {
    vector<Argument*> list;
    bool has_named_arguments, has_rest_argument;

    Arguments(string p, size_t l)
    : AST_Node(p, l),
      has_named_arguments(false), has_rest_argument(false)
    { }

    size_t length() { return list.size(); }
    Argument*& operator[](size_t i) { return list[i]; }

    Arguments& operator<<(Argument* a)
    {
      if (!a->name.empty()) {
        if (has_rest_argument)
          ; // error
        has_named_arguments = true;
      }
      else if (a->is_rest_argument) {
        if (has_rest_argument)
          ; // error
        if (has_named_arguments)
          ; // different error
        has_rest_argument = true;
      }
      else {
        if (has_rest_argument)
          ; // error
        if (has_named_arguments)
          ; // error
      }
      list.push_back(a);
      return *this;
    }
  };

  /////////////////////////////////////////
  // Abstract base class for CSS selectors.
  /////////////////////////////////////////
  struct Selector : public AST_Node {
    Selector(string p, size_t l) : AST_Node(p, l) { }
    virtual ~Selector() = 0;
  };

  /////////////////////////////////////////////////////////////////////////
  // Interpolated selectors -- the interpolated String will be expanded and
  // re-parsed into a normal selector structure.
  /////////////////////////////////////////////////////////////////////////
  struct Interpolated : Selector {
    String* selector;

    Interpolated(string p, size_t l, String* cont)
    : Selector(p, l), selector(cont)
    { }
  };

  ////////////////////////////////////////////
  // Abstract base class for atomic selectors.
  ////////////////////////////////////////////
  struct Simple_Base : public Selector {
    Simple_Base(string p, size_t l) : Selector(p, l) { }
  };

  //////////////////////////////////////////////////////////////////////
  // Normal simple selectors (e.g., "div", ".foo", ":first-child", etc).
  //////////////////////////////////////////////////////////////////////
  struct Simple : Simple_Base {
    string selector;

    Simple(string p, size_t l, string cont)
    : Simple_Base(p, l), selector(cont)
    { }
  };

  /////////////////////////////////////
  // Parent references (i.e., the "&").
  /////////////////////////////////////
  struct Reference : Simple_Base {
    Reference(string p, size_t l) : Simple_Base(p, l) { }
  };

  /////////////////////////////////////////////////////////////////////////
  // Placeholder selectors (e.g., "%foo") for use in extend-only selectors.
  /////////////////////////////////////////////////////////////////////////
  struct Placeholder : Simple_Base {
    Placeholder(string p, size_t l) : Simple_Base(p, l) { }
  };

  ////////////////////////////////////////////////////////////////////////////
  // Simple selector sequences. Maintains flags indicating whether it contains
  // any parent references or placeholders, to simplify expansion.
  ////////////////////////////////////////////////////////////////////////////
  struct Sequence : Selector {
    vector<Simple*> selectors;
    bool            has_reference;
    bool            has_placeholder;

    Sequence(string p, size_t l, size_t s)
    : Selector(p, l),
      selectors(vector<Simple*>()), has_reference(false), has_placeholder(false)
    { selectors.reserve(s); }

    size_t length()
    { return selectors.size(); }
    Simple*& operator[](size_t i)
    { return selectors[i]; }
    Sequence& operator<<(Simple* s)
    {
      selectors.push_back(s);
      return *this;
    }
    Sequence& operator<<(Reference* s)
    {
      has_reference = true;
      return (*this) << s;
    }
    Sequence& operator<<(Placeholder* p)
    {
      has_placeholder = true;
      return (*this) << p;
    }
    Sequence& operator+=(Sequence* seq)
    {
      for (size_t i = 0, L = length(); i < L; ++i) (*this) << ((*seq)[i]);
      return *this;
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  // General selectors -- i.e., simple sequences combined with one of the four
  // CSS selector combinators (">", "+", "~", and whitespace). Isomorphic to a
  // left-associative linked list.
  ////////////////////////////////////////////////////////////////////////////
  struct Combination : Selector {
    enum Combinator { ANCESTOR_OF, PARENT_OF, PRECEDES, ADJACENT_TO };
    Combinator   combinator;
    Combination* context;
    Sequence*    selector;
    bool         has_reference;
    bool         has_placeholder;

    Combination(string p, size_t l,
                Combinator c, Combination* ctx, Sequence* sel)
    : Selector(p, l),
      combinator(c),
      context(ctx),
      selector(sel),
      has_reference(ctx && ctx->has_reference ||
                    sel && sel->has_reference),
      has_placeholder(ctx && ctx->has_placeholder ||
                      sel && sel->has_placeholder)
    { }
  };

  // Probably gonna' have to scrap these, since everything else is geared
  // towards pointer semantics.
  Combination operator>(Combination c, Sequence s)
  { return Combination(c.path, c.line, Combination::PARENT_OF, &c, &s); }
  // No such thing as `operator[whitespace]`, so `>>` seems reasonable.
  Combination operator>>(Combination c, Sequence s)
  { return Combination(c.path, c.line, Combination::ANCESTOR_OF, &c, &s); }
  // Can't use `operator~` because it's unary!
  Combination operator*(Combination c, Sequence s)
  { return Combination(c.path, c.line, Combination::PRECEDES, &c, &s); }
  Combination operator+(Combination c, Sequence s)
  { return Combination(c.path, c.line, Combination::ADJACENT_TO, &c, &s); }

  ///////////////////////////////////
  // Comma-separated selector groups.
  ///////////////////////////////////
  struct Group : Selector {
    vector<Combination*> selectors;
    bool                 has_reference;
    bool                 has_placeholder;
  };


}