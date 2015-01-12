#include "inspect.hpp"
#include "ast.hpp"
#include "context.hpp"
#include <cmath>
#include <string>
#include <iostream>
#include <iomanip>

namespace Sass {
  using namespace std;

  Inspect::Inspect(Context* ctx)
  : buffer(""),
    indentation(0),
    ctx(ctx),
    in_declaration(false),
    in_declaration_list(false)
  { }
  Inspect::~Inspect() { }

  // statements
  void Inspect::operator()(Block* block)
  {
    if (!block->is_root()) {
      append_to_buffer(" {" + ctx->linefeed);
      ++ indentation;
    }
    for (size_t i = 0, L = block->length(); i < L; ++i) {
      append_indent_to_buffer();
      (*block)[i]->perform(this);
      // extra newline at the end of top-level statements
      if (block->is_root()) append_to_buffer(ctx->linefeed);
      append_to_buffer(ctx->linefeed);
    }
    if (!block->is_root()) {
      -- indentation;
      append_indent_to_buffer();
      append_to_buffer("}");
    }
    // remove extra newline that gets added after the last top-level block
    if (block->is_root()) {
      size_t l = buffer.length();
      if (l > 2 && buffer[l-1] == '\n' && buffer[l-2] == '\n') {
        buffer.erase(l-1);
        if (ctx) ctx->source_map.remove_line();
        source_map.remove_line();
      }
    }
  }

  void Inspect::operator()(Ruleset* ruleset)
  {
    ruleset->selector()->perform(this);
    ruleset->block()->perform(this);
  }

  void Inspect::operator()(Propset* propset)
  {
    propset->property_fragment()->perform(this);
    append_to_buffer(": ");
    propset->block()->perform(this);
  }

  void Inspect::operator()(Bubble* bubble)
  {
    append_to_buffer("Bubble ( ");
    bubble->node()->perform(this);
    append_to_buffer(" )");
  }

  void Inspect::operator()(Media_Block* media_block)
  {
    append_to_buffer("@media", media_block, " ");
    media_block->media_queries()->perform(this);
    media_block->block()->perform(this);
  }

  void Inspect::operator()(Feature_Block* feature_block)
  {
    append_to_buffer("@supports", feature_block, " ");
    feature_block->feature_queries()->perform(this);
    feature_block->block()->perform(this);
  }

  void Inspect::operator()(At_Root_Block* at_root_block)
  {
    append_to_buffer("@at-root ", at_root_block, " ");
    if(at_root_block->expression()) at_root_block->expression()->perform(this);
    at_root_block->block()->perform(this);
  }

  void Inspect::operator()(At_Rule* at_rule)
  {
    append_to_buffer(at_rule->keyword());
    if (at_rule->selector()) {
      append_to_buffer(" ");
      at_rule->selector()->perform(this);
    }
    if (at_rule->block()) {
      at_rule->block()->perform(this);
    }
    else {
      append_to_buffer(";");
    }
  }

  void Inspect::operator()(Declaration* dec)
  {
    if (dec->value()->concrete_type() == Expression::NULL_VAL) return;
    in_declaration = true;
    if (ctx) ctx->source_map.add_open_mapping(dec->property());
    source_map.add_open_mapping(dec->property());
    dec->property()->perform(this);
    if (ctx) ctx->source_map.add_close_mapping(dec->property());
    source_map.add_close_mapping(dec->property());
    append_to_buffer(": ");
    if (ctx) ctx->source_map.add_open_mapping(dec->value());
    source_map.add_open_mapping(dec->value());
    dec->value()->perform(this);
    if (dec->is_important()) append_to_buffer(" !important");
    if (ctx) ctx->source_map.add_close_mapping(dec->value());
    source_map.add_close_mapping(dec->value());
    append_to_buffer(";");
    in_declaration = false;
  }

  void Inspect::operator()(Assignment* assn)
  {
    append_to_buffer(assn->variable());
    append_to_buffer(": ");
    assn->value()->perform(this);
    if (assn->is_guarded()) append_to_buffer(" !default");
    append_to_buffer(";");
  }

  void Inspect::operator()(Import* import)
  {
    if (!import->urls().empty()) {
      append_to_buffer("@import", import, " ");
      import->urls().front()->perform(this);
      append_to_buffer(";");
      for (size_t i = 1, S = import->urls().size(); i < S; ++i) {
        append_to_buffer(ctx->linefeed);
        append_to_buffer("@import", import, " ");
        import->urls()[i]->perform(this);
        append_to_buffer(";");
      }
    }
  }

  void Inspect::operator()(Import_Stub* import)
  {
    append_to_buffer("@import", import, " ");
    append_to_buffer(import->file_name());
    append_to_buffer(";");
  }

  void Inspect::operator()(Warning* warning)
  {
    append_to_buffer("@warn", warning, " ");
    warning->message()->perform(this);
    append_to_buffer(";");
  }

  void Inspect::operator()(Error* error)
  {
    append_to_buffer("@error", error, " ");
    error->message()->perform(this);
    append_to_buffer(";");
  }

  void Inspect::operator()(Debug* debug)
  {
    append_to_buffer("@debug", debug, " ");
    debug->value()->perform(this);
    append_to_buffer(";");
  }

  void Inspect::operator()(Comment* comment)
  {
    comment->text()->perform(this);
  }

  void Inspect::operator()(If* cond)
  {
    append_to_buffer("@if", cond, " ");
    cond->predicate()->perform(this);
    cond->consequent()->perform(this);
    if (cond->alternative()) {
      append_to_buffer(ctx->linefeed);
      append_indent_to_buffer();
      append_to_buffer("else");
      cond->alternative()->perform(this);
    }
  }

  void Inspect::operator()(For* loop)
  {
    append_to_buffer("@for", loop, " ");
    append_to_buffer(loop->variable());
    append_to_buffer(" from ");
    loop->lower_bound()->perform(this);
    append_to_buffer((loop->is_inclusive() ? " through " : " to "));
    loop->upper_bound()->perform(this);
    loop->block()->perform(this);
  }

  void Inspect::operator()(Each* loop)
  {
    append_to_buffer("@each", loop, " ");
    append_to_buffer(loop->variables()[0]);
    for (size_t i = 1, L = loop->variables().size(); i < L; ++i) {
      append_to_buffer(", ");
      append_to_buffer(loop->variables()[i]);
    }
    append_to_buffer(" in ");
    loop->list()->perform(this);
    loop->block()->perform(this);
  }

  void Inspect::operator()(While* loop)
  {
    append_to_buffer("@while", loop, " ");
    loop->predicate()->perform(this);
    loop->block()->perform(this);
  }

  void Inspect::operator()(Return* ret)
  {
    append_to_buffer("@return", ret, " ");
    ret->value()->perform(this);
    append_to_buffer(";");
  }

  void Inspect::operator()(Extension* extend)
  {
    append_to_buffer("@extend", extend, " ");
    extend->selector()->perform(this);
    append_to_buffer(";");
  }

  void Inspect::operator()(Definition* def)
  {
    if (def->type() == Definition::MIXIN) {
      append_to_buffer("@mixin", def, " ");
    } else {
      append_to_buffer("@function", def, " ");
    }
    append_to_buffer(def->name());
    def->parameters()->perform(this);
    def->block()->perform(this);
  }

  void Inspect::operator()(Mixin_Call* call)
  {
    append_to_buffer("@include", call, " ");
    append_to_buffer(call->name());
    if (call->arguments()) {
      call->arguments()->perform(this);
    }
    if (call->block()) {
      append_to_buffer(" ");
      call->block()->perform(this);
    }
    if (!call->block()) append_to_buffer(";");
  }

  void Inspect::operator()(Content* content)
  {
    append_to_buffer("@content", content, ";");
  }

  void Inspect::operator()(Map* map)
  {
    if (map->empty()) return;
    if (map->is_invisible()) return;
    bool items_output = false;
    append_to_buffer("(");
    for (auto key : map->keys()) {
      if (key->is_invisible()) continue;
      if (map->at(key)->is_invisible()) continue;
      if (items_output) append_to_buffer(", ");
      key->perform(this);
      append_to_buffer(": ");
      map->at(key)->perform(this);
      items_output = true;
    }
    append_to_buffer(")");
  }

  void Inspect::operator()(List* list)
  {
    string sep(list->separator() == List::SPACE ? " " : ", ");
    if (list->empty()) return;
    bool items_output = false;
    in_declaration_list = in_declaration;
    for (size_t i = 0, L = list->length(); i < L; ++i) {
      Expression* list_item = (*list)[i];
      if (list_item->is_invisible()) {
        continue;
      }
      if (items_output) append_to_buffer(sep);
      list_item->perform(this);
      items_output = true;
    }
    in_declaration_list = false;
  }

  void Inspect::operator()(Binary_Expression* expr)
  {
    expr->left()->perform(this);
    switch (expr->type()) {
      case Binary_Expression::AND: append_to_buffer(" and "); break;
      case Binary_Expression::OR:  append_to_buffer(" or ");  break;
      case Binary_Expression::EQ:  append_to_buffer(" == ");  break;
      case Binary_Expression::NEQ: append_to_buffer(" != ");  break;
      case Binary_Expression::GT:  append_to_buffer(" > ");   break;
      case Binary_Expression::GTE: append_to_buffer(" >= ");  break;
      case Binary_Expression::LT:  append_to_buffer(" < ");   break;
      case Binary_Expression::LTE: append_to_buffer(" <= ");  break;
      case Binary_Expression::ADD: append_to_buffer(" + ");   break;
      case Binary_Expression::SUB: append_to_buffer(" - ");   break;
      case Binary_Expression::MUL: append_to_buffer(" * ");   break;
      case Binary_Expression::DIV: append_to_buffer("/");     break;
      case Binary_Expression::MOD: append_to_buffer(" % ");   break;
      default: break; // shouldn't get here
    }
    expr->right()->perform(this);
  }

  void Inspect::operator()(Unary_Expression* expr)
  {
    if (expr->type() == Unary_Expression::PLUS) append_to_buffer("+");
    else                                        append_to_buffer("-");
    expr->operand()->perform(this);
  }

  void Inspect::operator()(Function_Call* call)
  {
    append_to_buffer(call->name());
    call->arguments()->perform(this);
  }

  void Inspect::operator()(Function_Call_Schema* call)
  {
    call->name()->perform(this);
    call->arguments()->perform(this);
  }

  void Inspect::operator()(Variable* var)
  {
    append_to_buffer(var->name());
  }

  void Inspect::operator()(Textual* txt)
  {
    append_to_buffer(txt->value());
  }

  // helper functions for serializing numbers
  // string frac_to_string(double f, size_t p) {
  //   stringstream ss;
  //   ss.setf(ios::fixed, ios::floatfield);
  //   ss.precision(p);
  //   ss << f;
  //   string result(ss.str().substr(f < 0 ? 2 : 1));
  //   size_t i = result.size() - 1;
  //   while (result[i] == '0') --i;
  //   result = result.substr(0, i+1);
  //   return result;
  // }
  // string double_to_string(double d, size_t p) {
  //   stringstream ss;
  //   double ipart;
  //   double fpart = std::modf(d, &ipart);
  //   ss << ipart;
  //   if (fpart != 0) ss << frac_to_string(fpart, 5);
  //   return ss.str();
  // }

  void Inspect::operator()(Number* n)
  {
    stringstream ss;
    ss.precision(ctx ? ctx->precision : 5);
    ss << fixed << n->value();
    string d(ss.str());
    // store if the value did not equal zero
    // if after applying precsision, the value gets
    // truncated to zero, sass emits 0.0 instead of 0
    bool nonzero = n->value() != 0;
    for (size_t i = d.length()-1; d[i] == '0'; --i) {
      d.resize(d.length()-1);
    }
    if (d[d.length()-1] == '.') d.resize(d.length()-1);
    if (n->numerator_units().size() > 1 ||
        n->denominator_units().size() > 0 ||
        (n->numerator_units().size() && n->numerator_units()[0].find_first_of('/') != string::npos) ||
        (n->numerator_units().size() && n->numerator_units()[0].find_first_of('*') != string::npos)
    ) {
      error(d + n->unit() + " isn't a valid CSS value.", n->pstate());
    }
    if (!n->zero() && !in_declaration_list) {
      if (d.substr(0, 3) == "-0.") d.erase(1, 1);
      if (d.substr(0, 2) == "0.") d.erase(0, 1);
    }
    // remove the leading minus
    if (d == "-0") d.erase(0, 1);
    // use fractional output if we had
    // a value before it got truncated
    if (d == "0" && nonzero) d = "0.0";
    // append number and unit
    append_to_buffer(d);
    append_to_buffer(n->unit());
  }

  // helper function for serializing colors
  template <size_t range>
  static double cap_channel(double c) {
    if      (c > range) return range;
    else if (c < 0)     return 0;
    else                return c;
  }

  void Inspect::operator()(Color* c)
  {
    stringstream ss;
    double r = round(cap_channel<0xff>(c->r()));
    double g = round(cap_channel<0xff>(c->g()));
    double b = round(cap_channel<0xff>(c->b()));
    double a = cap_channel<1>   (c->a());

    // retain the originally specified color definition if unchanged
    if (!c->disp().empty()) {
      ss << c->disp();
    }
    else if (r == 0 && g == 0 && b == 0 && a == 0) {
        ss << "transparent";
    }
    else if (a >= 1) {
      // see if it's a named color
      int numval = r * 0x10000;
      numval += g * 0x100;
      numval += b;
      if (ctx && ctx->colors_to_names.count(numval)) {
        ss << ctx->colors_to_names[numval];
      }
      else {
        // otherwise output the hex triplet
        ss << '#' << setw(2) << setfill('0');
        ss << hex << setw(2) << static_cast<unsigned long>(r);
        ss << hex << setw(2) << static_cast<unsigned long>(g);
        ss << hex << setw(2) << static_cast<unsigned long>(b);
      }
    }
    else {
      ss << "rgba(";
      ss << static_cast<unsigned long>(r) << ", ";
      ss << static_cast<unsigned long>(g) << ", ";
      ss << static_cast<unsigned long>(b) << ", ";
      ss << a << ')';
    }
    append_to_buffer(ss.str());
  }

  void Inspect::operator()(Boolean* b)
  {
    append_to_buffer(b->value() ? "true" : "false");
  }

  void Inspect::operator()(String_Schema* ss)
  {
    // Evaluation should turn these into String_Constants, so this method is
    // only for inspection purposes.
    for (size_t i = 0, L = ss->length(); i < L; ++i) {
      if ((*ss)[i]->is_interpolant()) append_to_buffer("#{");
      (*ss)[i]->perform(this);
      if ((*ss)[i]->is_interpolant()) append_to_buffer("}");
    }
  }

  void Inspect::operator()(String_Constant* s)
  {
    append_to_buffer(s->needs_unquoting() ? unquote(s->value()) : s->value(), s);
  }

  void Inspect::operator()(Feature_Query* fq)
  {
    size_t i = 0;
    (*fq)[i++]->perform(this);
    for (size_t L = fq->length(); i < L; ++i) {
      (*fq)[i]->perform(this);
    }
  }

  void Inspect::operator()(Feature_Query_Condition* fqc)
  {
    if (fqc->operand() == Feature_Query_Condition::AND)
      append_to_buffer(" and ");
    else if (fqc->operand() == Feature_Query_Condition::OR)
      append_to_buffer(" or ");
    else if (fqc->operand() == Feature_Query_Condition::NOT)
      append_to_buffer(" not ");

    if (!fqc->is_root()) append_to_buffer("(");

    if (!fqc->length()) {
      fqc->feature()->perform(this);
      append_to_buffer(": ");
      fqc->value()->perform(this);
    }
    // else
    for (size_t i = 0, L = fqc->length(); i < L; ++i)
      (*fqc)[i]->perform(this);

    if (!fqc->is_root()) append_to_buffer(")");
  }

  void Inspect::operator()(Media_Query* mq)
  {
    size_t i = 0;
    if (mq->media_type()) {
      if      (mq->is_negated())    append_to_buffer("not ");
      else if (mq->is_restricted()) append_to_buffer("only ");
      mq->media_type()->perform(this);
    }
    else {
      (*mq)[i++]->perform(this);
    }
    for (size_t L = mq->length(); i < L; ++i) {
      append_to_buffer(" and ");
      (*mq)[i]->perform(this);
    }
  }

  void Inspect::operator()(Media_Query_Expression* mqe)
  {
    if (mqe->is_interpolated()) {
      if (ctx) ctx->source_map.add_open_mapping(mqe->feature());
      source_map.add_open_mapping(mqe->feature());
      mqe->feature()->perform(this);
      if (ctx) ctx->source_map.add_close_mapping(mqe->feature());
      source_map.add_close_mapping(mqe->feature());
    }
    else {
      append_to_buffer("(");
      if (ctx) ctx->source_map.add_open_mapping(mqe->feature());
      source_map.add_open_mapping(mqe->feature());
      mqe->feature()->perform(this);
      if (ctx) ctx->source_map.add_close_mapping(mqe->feature());
      source_map.add_close_mapping(mqe->feature());
      if (mqe->value()) {
        append_to_buffer(": ");
        if (ctx) ctx->source_map.add_open_mapping(mqe->value());
        source_map.add_open_mapping(mqe->value());
        mqe->value()->perform(this);
        if (ctx) ctx->source_map.add_close_mapping(mqe->value());
        source_map.add_close_mapping(mqe->value());
      }
      append_to_buffer(")");
    }
  }

  void Inspect::operator()(At_Root_Expression* ae)
  {
    if (ae->is_interpolated()) {
      ae->feature()->perform(this);
    }
    else {
      append_to_buffer("(");
      ae->feature()->perform(this);
      if (ae->value()) {
        append_to_buffer(": ");
        ae->value()->perform(this);
      }
      append_to_buffer(")");
    }
  }

  void Inspect::operator()(Null* n)
  {
    append_to_buffer("null");
  }

  // parameters and arguments
  void Inspect::operator()(Parameter* p)
  {
    append_to_buffer(p->name());
    if (p->default_value()) {
      append_to_buffer(": ");
      p->default_value()->perform(this);
    }
    else if (p->is_rest_parameter()) {
      append_to_buffer("...");
    }
  }

  void Inspect::operator()(Parameters* p)
  {
    append_to_buffer("(");
    if (!p->empty()) {
      (*p)[0]->perform(this);
      for (size_t i = 1, L = p->length(); i < L; ++i) {
        append_to_buffer(", ");
        (*p)[i]->perform(this);
      }
    }
    append_to_buffer(")");
  }

  void Inspect::operator()(Argument* a)
  {
    if (!a->name().empty()) {
      append_to_buffer(a->name());
      append_to_buffer(": ");
    }
    // Special case: argument nulls can be ignored
    if (a->value()->concrete_type() == Expression::NULL_VAL) {
      return;
    }
    if (a->value()->concrete_type() == Expression::STRING) {
      String_Constant* s = static_cast<String_Constant*>(a->value());
      if (s->is_quoted()) s->value(quote(unquote(s->value()), String_Constant::double_quote()));
      s->perform(this);
    } else a->value()->perform(this);
    if (a->is_rest_argument()) {
      append_to_buffer("...");
    }
  }

  void Inspect::operator()(Arguments* a)
  {
    append_to_buffer("(");
    if (!a->empty()) {
      (*a)[0]->perform(this);
      for (size_t i = 1, L = a->length(); i < L; ++i) {
        append_to_buffer(", ");
        (*a)[i]->perform(this);
      }
    }
    append_to_buffer(")");
  }

  // selectors
  void Inspect::operator()(Selector_Schema* s)
  {
    s->contents()->perform(this);
  }

  void Inspect::operator()(Selector_Reference* ref)
  {
    if (ref->selector()) ref->selector()->perform(this);
    else                 append_to_buffer("&");
  }

  void Inspect::operator()(Selector_Placeholder* s)
  {
    append_to_buffer(s->name(), s);
  }

  void Inspect::operator()(Type_Selector* s)
  {
    append_to_buffer(s->name(), s);
  }

  void Inspect::operator()(Selector_Qualifier* s)
  {
    append_to_buffer(s->name(), s);
  }

  void Inspect::operator()(Attribute_Selector* s)
  {
    append_to_buffer("[");
    if (ctx) ctx->source_map.add_open_mapping(s);
    source_map.add_open_mapping(s);
    append_to_buffer(s->name());
    if (!s->matcher().empty()) {
      append_to_buffer(s->matcher());
      if (s->value()) {
        s->value()->perform(this);
      }
      // append_to_buffer(s->value());
    }
    if (ctx) ctx->source_map.add_close_mapping(s);
    source_map.add_close_mapping(s);
    append_to_buffer("]");
  }

  void Inspect::operator()(Pseudo_Selector* s)
  {
    append_to_buffer(s->name(), s);
    if (s->expression()) {
      s->expression()->perform(this);
      append_to_buffer(")");
    }
  }

  void Inspect::operator()(Wrapped_Selector* s)
  {
    append_to_buffer(s->name(), s);
    s->selector()->perform(this);
    append_to_buffer(")");
  }

  void Inspect::operator()(Compound_Selector* s)
  {
    for (size_t i = 0, L = s->length(); i < L; ++i) {
      (*s)[i]->perform(this);
    }
  }

  void Inspect::operator()(Complex_Selector* c)
  {
    Compound_Selector*           head = c->head();
    Complex_Selector*            tail = c->tail();
    Complex_Selector::Combinator comb = c->combinator();
    if (head && !head->is_empty_reference()) head->perform(this);
    if (head && !head->is_empty_reference() && tail) append_to_buffer(" ");
    switch (comb) {
      case Complex_Selector::ANCESTOR_OF:                                        break;
      case Complex_Selector::PARENT_OF:   append_to_buffer(">"); break;
      case Complex_Selector::PRECEDES:    append_to_buffer("~"); break;
      case Complex_Selector::ADJACENT_TO: append_to_buffer("+"); break;
    }
    if (tail && comb != Complex_Selector::ANCESTOR_OF) {
      append_to_buffer(" ");
    }
    if (tail) tail->perform(this);
  }

  void Inspect::operator()(Selector_List* g)
  {
    if (g->empty()) return;
    if (ctx) ctx->source_map.add_open_mapping((*g)[0]);
    source_map.add_open_mapping((*g)[0]);
    (*g)[0]->perform(this);
    if (ctx) ctx->source_map.add_close_mapping((*g)[0]);
    source_map.add_close_mapping((*g)[0]);
    for (size_t i = 1, L = g->length(); i < L; ++i) {
      append_to_buffer(", ");
      if (ctx) ctx->source_map.add_open_mapping((*g)[i]);
      source_map.add_open_mapping((*g)[i]);
      (*g)[i]->perform(this);
      if (ctx) ctx->source_map.add_close_mapping((*g)[i]);
      source_map.add_close_mapping((*g)[i]);
    }
  }

  inline void Inspect::fallback_impl(AST_Node* n)
  { }

  void Inspect::append_indent_to_buffer()
  {
    string indent = "";
    for (size_t i = 0; i < indentation; i++)
      indent += ctx->indent;
    append_to_buffer(indent);
  }

  string unquote(const string& s)
  {
    if (s.empty()) return "";
    if (s.length() == 1) {
      if (s[0] == '"' || s[0] == '\'') return "";
    }
    char q;
    if      (*s.begin() == '"'  && *s.rbegin() == '"')  q = '"';
    else if (*s.begin() == '\'' && *s.rbegin() == '\'') q = '\'';
    else                                                return s;
    string t;
    t.reserve(s.length()-2);
    for (size_t i = 1, L = s.length()-1; i < L; ++i) {
      // if we see a quote, we need to remove the preceding backslash from t
      if (s[i-1] == '\\' && s[i] == q) t.erase(t.length()-1);
      t.push_back(s[i]);
    }
    return t;
  }

  string quote(const string& s, char q)
  {
    if (s.empty()) return string(2, q);
    if (!q || s[0] == '"' || s[0] == '\'') return s;
    string t;
    t.reserve(s.length()+2);
    t.push_back(q);
    for (size_t i = 0, L = s.length(); i < L; ++i) {
      if (s[i] == q) t.push_back('\\');
      t.push_back(s[i]);
    }
    t.push_back(q);
    return t;
  }

  void Inspect::append_to_buffer(const string& text)
  {
    buffer += text;
    if (ctx) ctx->source_map.update_column(text);
    source_map.update_column(text);
  }

  void Inspect::append_to_buffer(const string& text, AST_Node* node)
  {
    if (ctx) ctx->source_map.add_open_mapping(node);
    source_map.add_open_mapping(node);
    append_to_buffer(text);
    if (ctx) ctx->source_map.add_close_mapping(node);
    source_map.add_close_mapping(node);
  }

  void Inspect::append_to_buffer(const string& text, AST_Node* node, const string& tail)
  {
    append_to_buffer(text, node);
    append_to_buffer(tail);
  }

}
