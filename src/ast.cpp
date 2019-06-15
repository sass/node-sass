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

  const char* sass_op_to_name(enum Sass_OP op) {
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
      case SUB: return "minus";
      case MUL: return "times";
      case DIV: return "div";
      case MOD: return "mod";
      // this is only used internally!
      case NUM_OPS: return "[OPS]";
      default: return "invalid";
    }
  }

  const char* sass_op_separator(enum Sass_OP op) {
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

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void AST_Node::update_pstate(const ParserState& pstate)
  {
    pstate_.offset += pstate - pstate_ + pstate.offset;
  }

  const std::string AST_Node::to_string(Sass_Inspect_Options opt) const
  {
    Sass_Output_Options out(opt);
    Emitter emitter(out);
    Inspect i(emitter);
    i.in_declaration = true;
    // ToDo: inspect should be const
    const_cast<AST_Node*>(this)->perform(&i);
    return i.get_buffer();
  }

  const std::string AST_Node::to_string() const
  {
    return to_string({ NESTED, 5 });
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Expression_Obj Hashed::at(Expression_Obj k) const
  {
    if (elements_.count(k))
    { return elements_.at(k); }
    else { return {}; }
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Statement::Statement(ParserState pstate, Type st, size_t t)
  : AST_Node(pstate), statement_type_(st), tabs_(t), group_end_(false)
  { }
  Statement::Statement(const Statement* ptr)
  : AST_Node(ptr),
    statement_type_(ptr->statement_type_),
    tabs_(ptr->tabs_),
    group_end_(ptr->group_end_)
  { }

  bool Statement::bubbles()
  {
    return false;
  }

  bool Statement::has_content()
  {
    return statement_type_ == CONTENT;
  }

  bool Statement::is_invisible() const
  {
    return false;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Block::Block(ParserState pstate, size_t s, bool r)
  : Statement(pstate),
    Vectorized<Statement_Obj>(s),
    is_root_(r)
  { }
  Block::Block(const Block* ptr)
  : Statement(ptr),
    Vectorized<Statement_Obj>(*ptr),
    is_root_(ptr->is_root_)
  { }

  bool Block::has_content()
  {
    for (size_t i = 0, L = elements().size(); i < L; ++i) {
      if (elements()[i]->has_content()) return true;
    }
    return Statement::has_content();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Has_Block::Has_Block(ParserState pstate, Block_Obj b)
  : Statement(pstate), block_(b)
  { }
  Has_Block::Has_Block(const Has_Block* ptr)
  : Statement(ptr), block_(ptr->block_)
  { }

  bool Has_Block::has_content()
  {
    return (block_ && block_->has_content()) || Statement::has_content();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Ruleset::Ruleset(ParserState pstate, Selector_List_Obj s, Block_Obj b)
  : Has_Block(pstate, b), selector_(s), is_root_(false)
  { statement_type(RULESET); }
  Ruleset::Ruleset(const Ruleset* ptr)
  : Has_Block(ptr),
    selector_(ptr->selector_),
    is_root_(ptr->is_root_)
  { statement_type(RULESET); }

  bool Ruleset::is_invisible() const {
    if (Selector_List* sl = Cast<Selector_List>(selector())) {
      for (size_t i = 0, L = sl->length(); i < L; ++i)
        if (!(*sl)[i]->has_placeholder()) return false;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Bubble::Bubble(ParserState pstate, Statement_Obj n, Statement_Obj g, size_t t)
  : Statement(pstate, Statement::BUBBLE, t), node_(n), group_end_(g == nullptr)
  { }
  Bubble::Bubble(const Bubble* ptr)
  : Statement(ptr),
    node_(ptr->node_),
    group_end_(ptr->group_end_)
  { }

  bool Bubble::bubbles()
  {
    return true;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Trace::Trace(ParserState pstate, std::string n, Block_Obj b, char type)
  : Has_Block(pstate, b), type_(type), name_(n)
  { }
  Trace::Trace(const Trace* ptr)
  : Has_Block(ptr),
    type_(ptr->type_),
    name_(ptr->name_)
  { }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Media_Block::Media_Block(ParserState pstate, List_Obj mqs, Block_Obj b)
  : Has_Block(pstate, b), media_queries_(mqs)
  { statement_type(MEDIA); }
  Media_Block::Media_Block(const Media_Block* ptr)
  : Has_Block(ptr), media_queries_(ptr->media_queries_)
  { statement_type(MEDIA); }

  bool Media_Block::is_invisible() const {
    for (size_t i = 0, L = block()->length(); i < L; ++i) {
      Statement_Obj stm = block()->at(i);
      if (!stm->is_invisible()) return false;
    }
    return true;
  }

  bool Media_Block::bubbles()
  {
    return true;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Directive::Directive(ParserState pstate, std::string kwd, Selector_List_Obj sel, Block_Obj b, Expression_Obj val)
  : Has_Block(pstate, b), keyword_(kwd), selector_(sel), value_(val) // set value manually if needed
  { statement_type(DIRECTIVE); }
  Directive::Directive(const Directive* ptr)
  : Has_Block(ptr),
    keyword_(ptr->keyword_),
    selector_(ptr->selector_),
    value_(ptr->value_) // set value manually if needed
  { statement_type(DIRECTIVE); }

  bool Directive::bubbles() { return is_keyframes() || is_media(); }

  bool Directive::is_media() {
    return keyword_.compare("@-webkit-media") == 0 ||
            keyword_.compare("@-moz-media") == 0 ||
            keyword_.compare("@-o-media") == 0 ||
            keyword_.compare("@media") == 0;
  }
  bool Directive::is_keyframes() {
    return keyword_.compare("@-webkit-keyframes") == 0 ||
            keyword_.compare("@-moz-keyframes") == 0 ||
            keyword_.compare("@-o-keyframes") == 0 ||
            keyword_.compare("@keyframes") == 0;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Keyframe_Rule::Keyframe_Rule(ParserState pstate, Block_Obj b)
  : Has_Block(pstate, b), name_()
  { statement_type(KEYFRAMERULE); }
  Keyframe_Rule::Keyframe_Rule(const Keyframe_Rule* ptr)
  : Has_Block(ptr), name_(ptr->name_)
  { statement_type(KEYFRAMERULE); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Declaration::Declaration(ParserState pstate, String_Obj prop, Expression_Obj val, bool i, bool c, Block_Obj b)
  : Has_Block(pstate, b), property_(prop), value_(val), is_important_(i), is_custom_property_(c), is_indented_(false)
  { statement_type(DECLARATION); }
  Declaration::Declaration(const Declaration* ptr)
  : Has_Block(ptr),
    property_(ptr->property_),
    value_(ptr->value_),
    is_important_(ptr->is_important_),
    is_custom_property_(ptr->is_custom_property_),
    is_indented_(ptr->is_indented_)
  { statement_type(DECLARATION); }

  bool Declaration::is_invisible() const
  {
    if (is_custom_property()) return false;
    return !(value_ && !Cast<Null>(value_));
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Assignment::Assignment(ParserState pstate, std::string var, Expression_Obj val, bool is_default, bool is_global)
  : Statement(pstate), variable_(var), value_(val), is_default_(is_default), is_global_(is_global)
  { statement_type(ASSIGNMENT); }
  Assignment::Assignment(const Assignment* ptr)
  : Statement(ptr),
    variable_(ptr->variable_),
    value_(ptr->value_),
    is_default_(ptr->is_default_),
    is_global_(ptr->is_global_)
  { statement_type(ASSIGNMENT); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Import::Import(ParserState pstate)
  : Statement(pstate),
    urls_(std::vector<Expression_Obj>()),
    incs_(std::vector<Include>()),
    import_queries_()
  { statement_type(IMPORT); }
  Import::Import(const Import* ptr)
  : Statement(ptr),
    urls_(ptr->urls_),
    incs_(ptr->incs_),
    import_queries_(ptr->import_queries_)
  { statement_type(IMPORT); }

  std::vector<Include>& Import::incs() { return incs_; }
  std::vector<Expression_Obj>& Import::urls() { return urls_; }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Import_Stub::Import_Stub(ParserState pstate, Include res)
  : Statement(pstate), resource_(res)
  { statement_type(IMPORT_STUB); }
  Import_Stub::Import_Stub(const Import_Stub* ptr)
  : Statement(ptr), resource_(ptr->resource_)
  { statement_type(IMPORT_STUB); }
  Include Import_Stub::resource() { return resource_; };
  std::string Import_Stub::imp_path() { return resource_.imp_path; };
  std::string Import_Stub::abs_path() { return resource_.abs_path; };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Warning::Warning(ParserState pstate, Expression_Obj msg)
  : Statement(pstate), message_(msg)
  { statement_type(WARNING); }
  Warning::Warning(const Warning* ptr)
  : Statement(ptr), message_(ptr->message_)
  { statement_type(WARNING); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Error::Error(ParserState pstate, Expression_Obj msg)
  : Statement(pstate), message_(msg)
  { statement_type(ERROR); }
  Error::Error(const Error* ptr)
  : Statement(ptr), message_(ptr->message_)
  { statement_type(ERROR); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Debug::Debug(ParserState pstate, Expression_Obj val)
  : Statement(pstate), value_(val)
  { statement_type(DEBUGSTMT); }
  Debug::Debug(const Debug* ptr)
  : Statement(ptr), value_(ptr->value_)
  { statement_type(DEBUGSTMT); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Comment::Comment(ParserState pstate, String_Obj txt, bool is_important)
  : Statement(pstate), text_(txt), is_important_(is_important)
  { statement_type(COMMENT); }
  Comment::Comment(const Comment* ptr)
  : Statement(ptr),
    text_(ptr->text_),
    is_important_(ptr->is_important_)
  { statement_type(COMMENT); }

  bool Comment::is_invisible() const
  {
    return false;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  If::If(ParserState pstate, Expression_Obj pred, Block_Obj con, Block_Obj alt)
  : Has_Block(pstate, con), predicate_(pred), alternative_(alt)
  { statement_type(IF); }
  If::If(const If* ptr)
  : Has_Block(ptr),
    predicate_(ptr->predicate_),
    alternative_(ptr->alternative_)
  { statement_type(IF); }

  bool If::has_content()
  {
    return Has_Block::has_content() || (alternative_ && alternative_->has_content());
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  For::For(ParserState pstate,
      std::string var, Expression_Obj lo, Expression_Obj hi, Block_Obj b, bool inc)
  : Has_Block(pstate, b),
    variable_(var), lower_bound_(lo), upper_bound_(hi), is_inclusive_(inc)
  { statement_type(FOR); }
  For::For(const For* ptr)
  : Has_Block(ptr),
    variable_(ptr->variable_),
    lower_bound_(ptr->lower_bound_),
    upper_bound_(ptr->upper_bound_),
    is_inclusive_(ptr->is_inclusive_)
  { statement_type(FOR); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Each::Each(ParserState pstate, std::vector<std::string> vars, Expression_Obj lst, Block_Obj b)
  : Has_Block(pstate, b), variables_(vars), list_(lst)
  { statement_type(EACH); }
  Each::Each(const Each* ptr)
  : Has_Block(ptr), variables_(ptr->variables_), list_(ptr->list_)
  { statement_type(EACH); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  While::While(ParserState pstate, Expression_Obj pred, Block_Obj b)
  : Has_Block(pstate, b), predicate_(pred)
  { statement_type(WHILE); }
  While::While(const While* ptr)
  : Has_Block(ptr), predicate_(ptr->predicate_)
  { statement_type(WHILE); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Return::Return(ParserState pstate, Expression_Obj val)
  : Statement(pstate), value_(val)
  { statement_type(RETURN); }
  Return::Return(const Return* ptr)
  : Statement(ptr), value_(ptr->value_)
  { statement_type(RETURN); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Extension::Extension(ParserState pstate, Selector_List_Obj s)
  : Statement(pstate), selector_(s)
  { statement_type(EXTEND); }
  Extension::Extension(const Extension* ptr)
  : Statement(ptr), selector_(ptr->selector_)
  { statement_type(EXTEND); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Definition::Definition(const Definition* ptr)
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

  Definition::Definition(ParserState pstate,
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

  Definition::Definition(ParserState pstate,
              Signature sig,
              std::string n,
              Parameters_Obj params,
              Native_Function func_ptr,
              bool overload_stub)
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

  Definition::Definition(ParserState pstate,
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

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Mixin_Call::Mixin_Call(ParserState pstate, std::string n, Arguments_Obj args, Parameters_Obj b_params, Block_Obj b)
  : Has_Block(pstate, b), name_(n), arguments_(args), block_parameters_(b_params)
  { }
  Mixin_Call::Mixin_Call(const Mixin_Call* ptr)
  : Has_Block(ptr),
    name_(ptr->name_),
    arguments_(ptr->arguments_),
    block_parameters_(ptr->block_parameters_)
  { }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Content::Content(ParserState pstate, Arguments_Obj args)
  : Statement(pstate),
    arguments_(args)
  { statement_type(CONTENT); }
  Content::Content(const Content* ptr)
  : Statement(ptr),
    arguments_(ptr->arguments_)
  { statement_type(CONTENT); }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Expression::Expression(ParserState pstate, bool d, bool e, bool i, Type ct)
  : AST_Node(pstate),
    is_delayed_(d),
    is_expanded_(e),
    is_interpolant_(i),
    concrete_type_(ct)
  { }

  Expression::Expression(const Expression* ptr)
  : AST_Node(ptr),
    is_delayed_(ptr->is_delayed_),
    is_expanded_(ptr->is_expanded_),
    is_interpolant_(ptr->is_interpolant_),
    concrete_type_(ptr->concrete_type_)
  { }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Unary_Expression::Unary_Expression(ParserState pstate, Type t, Expression_Obj o)
  : Expression(pstate), optype_(t), operand_(o), hash_(0)
  { }
  Unary_Expression::Unary_Expression(const Unary_Expression* ptr)
  : Expression(ptr),
    optype_(ptr->optype_),
    operand_(ptr->operand_),
    hash_(ptr->hash_)
  { }
  const std::string Unary_Expression::type_name() {
    switch (optype_) {
      case PLUS: return "plus";
      case MINUS: return "minus";
      case SLASH: return "slash";
      case NOT: return "not";
      default: return "invalid";
    }
  }
  bool Unary_Expression::operator==(const Expression& rhs) const
  {
    try
    {
      const Unary_Expression* m = Cast<Unary_Expression>(&rhs);
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
  size_t Unary_Expression::hash() const
  {
    if (hash_ == 0) {
      hash_ = std::hash<size_t>()(optype_);
      hash_combine(hash_, operand()->hash());
    };
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Argument::Argument(ParserState pstate, Expression_Obj val, std::string n, bool rest, bool keyword)
  : Expression(pstate), value_(val), name_(n), is_rest_argument_(rest), is_keyword_argument_(keyword), hash_(0)
  {
    if (!name_.empty() && is_rest_argument_) {
      coreError("variable-length argument may not be passed by name", pstate_);
    }
  }
  Argument::Argument(const Argument* ptr)
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

  void Argument::set_delayed(bool delayed)
  {
    if (value_) value_->set_delayed(delayed);
    is_delayed(delayed);
  }

  bool Argument::operator==(const Expression& rhs) const
  {
    try
    {
      const Argument* m = Cast<Argument>(&rhs);
      if (!(m && name() == m->name())) return false;
      return *value() == *m->value();
    }
    catch (std::bad_cast&)
    {
      return false;
    }
    catch (...) { throw; }
  }

  size_t Argument::hash() const
  {
    if (hash_ == 0) {
      hash_ = std::hash<std::string>()(name());
      hash_combine(hash_, value()->hash());
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Arguments::Arguments(ParserState pstate)
  : Expression(pstate),
    Vectorized<Argument_Obj>(),
    has_named_arguments_(false),
    has_rest_argument_(false),
    has_keyword_argument_(false)
  { }
  Arguments::Arguments(const Arguments* ptr)
  : Expression(ptr),
    Vectorized<Argument_Obj>(*ptr),
    has_named_arguments_(ptr->has_named_arguments_),
    has_rest_argument_(ptr->has_rest_argument_),
    has_keyword_argument_(ptr->has_keyword_argument_)
  { }

  void Arguments::set_delayed(bool delayed)
  {
    for (Argument_Obj arg : elements()) {
      if (arg) arg->set_delayed(delayed);
    }
    is_delayed(delayed);
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

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Media_Query::Media_Query(ParserState pstate, String_Obj t, size_t s, bool n, bool r)
  : Expression(pstate), Vectorized<Media_Query_Expression_Obj>(s),
    media_type_(t), is_negated_(n), is_restricted_(r)
  { }
  Media_Query::Media_Query(const Media_Query* ptr)
  : Expression(ptr),
    Vectorized<Media_Query_Expression_Obj>(*ptr),
    media_type_(ptr->media_type_),
    is_negated_(ptr->is_negated_),
    is_restricted_(ptr->is_restricted_)
  { }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Media_Query_Expression::Media_Query_Expression(ParserState pstate,
                          Expression_Obj f, Expression_Obj v, bool i)
  : Expression(pstate), feature_(f), value_(v), is_interpolated_(i)
  { }
  Media_Query_Expression::Media_Query_Expression(const Media_Query_Expression* ptr)
  : Expression(ptr),
    feature_(ptr->feature_),
    value_(ptr->value_),
    is_interpolated_(ptr->is_interpolated_)
  { }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  At_Root_Query::At_Root_Query(ParserState pstate, Expression_Obj f, Expression_Obj v, bool i)
  : Expression(pstate), feature_(f), value_(v)
  { }
  At_Root_Query::At_Root_Query(const At_Root_Query* ptr)
  : Expression(ptr),
    feature_(ptr->feature_),
    value_(ptr->value_)
  { }

  bool At_Root_Query::exclude(std::string str)
  {
    bool with = feature() && unquote(feature()->to_string()).compare("with") == 0;
    List* l = static_cast<List*>(value().ptr());
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

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  At_Root_Block::At_Root_Block(ParserState pstate, Block_Obj b, At_Root_Query_Obj e)
  : Has_Block(pstate, b), expression_(e)
  { statement_type(ATROOT); }
  At_Root_Block::At_Root_Block(const At_Root_Block* ptr)
  : Has_Block(ptr), expression_(ptr->expression_)
  { statement_type(ATROOT); }

  bool At_Root_Block::bubbles() {
    return true;
  }

  bool At_Root_Block::exclude_node(Statement_Obj s) {
    if (expression() == nullptr)
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

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Parameter::Parameter(ParserState pstate, std::string n, Expression_Obj def, bool rest)
  : AST_Node(pstate), name_(n), default_value_(def), is_rest_parameter_(rest)
  { }
  Parameter::Parameter(const Parameter* ptr)
  : AST_Node(ptr),
    name_(ptr->name_),
    default_value_(ptr->default_value_),
    is_rest_parameter_(ptr->is_rest_parameter_)
  { }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Parameters::Parameters(ParserState pstate)
  : AST_Node(pstate),
    Vectorized<Parameter_Obj>(),
    has_optional_parameters_(false),
    has_rest_parameter_(false)
  { }
  Parameters::Parameters(const Parameters* ptr)
  : AST_Node(ptr),
    Vectorized<Parameter_Obj>(*ptr),
    has_optional_parameters_(ptr->has_optional_parameters_),
    has_rest_parameter_(ptr->has_rest_parameter_)
  { }

  void Parameters::adjust_after_pushing(Parameter_Obj p)
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

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  IMPLEMENT_AST_OPERATORS(Ruleset);
  IMPLEMENT_AST_OPERATORS(Media_Block);
  IMPLEMENT_AST_OPERATORS(Import);
  IMPLEMENT_AST_OPERATORS(Import_Stub);
  IMPLEMENT_AST_OPERATORS(Directive);
  IMPLEMENT_AST_OPERATORS(At_Root_Block);
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
  IMPLEMENT_AST_OPERATORS(Comment);
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

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
