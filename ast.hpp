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

    size_t size() const
    { return statements.size(); }
    Statement*& at(size_t i)
    { return statements.at(i); }
    Block& push(Statement* s)
    {
      statements.push_back(s);
      return *this;
    }
    Block& append(Block* b)
    {
      for (size_t i = 0, S = size(); i < S; ++i) push(b->at(i));
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
    Variable* variable;
    Value*    value;
    bool      is_guarded;

    Assignment(string p, size_t l,
               Variable* var, Value* val, bool guarded = false)
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
    Variable* variable;
    Value*    lower_bound;
    Value*    upper_bound;
    bool      is_inclusive;

    For(string p, size_t l,
        Variable* var, Value* lo, Value* hi, Block* b, bool inc)
    : Has_Block(p, l, b),
      variable(var), lower_bound(lo), upper_bound(hi), is_inclusive(inc)
    { }
  };

  //////////////////////////////////////
  // The Sass `@each` control directive.
  //////////////////////////////////////
  struct Each : public Has_Block {
    Variable* variable;
    Value*    list;

    Each(string p, size_t l, Variable* var, Value* lst, Block* b)
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

  ///////////////////////////////////////////////////////////////////////////
  // Definitions for both mixins and functions (distinguished by a type-tag).
  ///////////////////////////////////////////////////////////////////////////
  struct Parameters;
  struct Definition : public Has_Block {
    enum Type { mixin, function };
    String*     name;
    Parameters* parameters;
    Type        type;

    Definition(string p, size_t l,
               String* n, Parameters* params, Block* b, Type t)
    : Has_Block(p, l, b), name(n), parameters(params), type(t)
    { }
  };

  //////////////////////////////////////
  // Mixin calls (i.e., `@include ...`).
  //////////////////////////////////////
  struct Arguments;
  struct Mixin_Call : public Has_Block {
    String* name;
    Arguments* arguments;

    Mixin_Call(string p, size_t l, String* n, Arguments* args, Block* b = 0)
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

    size_t size() const
    { return values.size(); }
    Value*& at(size_t i)
    { return values.at(i); }
    List& push(Value* v)
    {
      values.push_back(v);
      return *this;
    }
    List& append(List* l)
    {
      for (size_t i = 0, S = size(); i < S; ++i) push(l->at(i));
      return *this;
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  // Abstract base class for binary operations. Represents logical, relational,
  // and arithmetic operations. Subclassed to avoid large switch statements.
  /////////////////////////////////////////////////////////////////////////////
  struct Binary_Operation : public Value {
    Value* left;
    Value* right;

    Binary_Operation(string p, size_t l, Value* lhs, Value* rhs)
    : Value(p, l), left(lhs), right(rhs)
    { }
  };

  /////////////////////////////////
  // Conjunctions and disjunctions.
  /////////////////////////////////
  struct Conjunction : public Binary_Operation {
    Conjunction(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Disjunction : public Binary_Operation {
    Disjunction(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };

  ////////////////////////////////
  // Numeric relational operators.
  ////////////////////////////////
  struct Equal : public Binary_Operation {
    Equal(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Not_Equal : public Binary_Operation {
    Not_Equal(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Greater : public Binary_Operation {
    Greater(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Greater_Equal : public Binary_Operation {
    Greater_Equal(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Less : public Binary_Operation {
    Less(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Less_Equal : public Binary_Operation {
    Less_Equal(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };

  ////////////////////////
  // Arithmetic operators.
  ////////////////////////
  struct Addition : public Binary_Operation {
    Addition(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Subtraction : public Binary_Operation {
    Subtraction(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Multiplication : public Binary_Operation {
    Multiplication(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
    { }
  };
  struct Division : public Binary_Operation {
    Division(string p, size_t l, Value* lhs, Value* rhs)
    : Binary_Operation(p, l, lhs, rhs)
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
    String* name;

    Variable(string p, size_t l, String* n)
    : Value(p, l), name(n)
    { }
  };

  ///////////////////////////////////////////////////////////////////////////
  // Textual (i.e., unevaluated) numeric data. Distinguished with a type-tag.
  ///////////////////////////////////////////////////////////////////////////
  struct Textual_Numeric : public Value {
    enum Type { number, percentage, dimension, hex };
    Type type;

    Textual_Numeric(string p, size_t l, Type t)
    : Value(p, l), type(t)
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
  struct Token;
  struct Dimension : public Value {
    double value;
    vector<Token*> numerator_units;
    vector<Token*> denominator_units;
    Dimension(string p, size_t l, double val, Token* unit)
    : Value(p, l),
      value(val),
      numerator_units(vector<Token*>()),
      denominator_units(vector<Token*>())
    { numerator_units.push_back(unit); }
  };
  struct Color : public Value {
    double r, g, b, a;
    Color(string p, size_t l, double r, double g, double b, double a = 0)
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

    size_t size() const
    { return fragments.size(); }
    Value*& at(size_t i)
    { return fragments.at(i); }
    String& push(Value* v)
    {
      fragments.push_back(v);
      return *this;
    }
    String& append(String* l)
    {
      for (size_t i = 0, S = size(); i < S; ++i) push(l->at(i));
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


}