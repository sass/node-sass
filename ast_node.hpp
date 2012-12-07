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
  // Assignments -- name and value.
  /////////////////////////////////
  class Assignment : public Statement {

    String* name_;
    Value*  value_;

  public:

    Assignment(string p, size_t l, String* n, Value* v)
    : Statement(p, l),
      name_(n),
      value_(v)
    { }

    String* name() const { return name_; }
    Value*  value()      { return value_; }

    String* name(String* n) { return name_ = n; }
    Value*  value(Value* v) { return value_ = v; }

  };

}