#ifndef SASS_DEBUGGER_H
#define SASS_DEBUGGER_H

#include <string>
#include "ast_fwd_decl.hpp"

using namespace std;
using namespace Sass;

inline string str_replace(std::string str, const std::string& oldStr, const std::string& newStr)
{
  size_t pos = 0;
  while((pos = str.find(oldStr, pos)) != std::string::npos)
  {
     str.replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
  return str;
}

inline string prettyprint(const string& str) {
  string clean = str_replace(str, "\n", "\\n");
  clean = str_replace(clean, "	", "\\t");
  clean = str_replace(clean, "\r", "\\r");
  return clean;
}

inline void debug_ast(AST_Node* node, string ind = "", Env* env = 0)
{

  if (ind == "") cerr << "####################################################################\n";
  if (dynamic_cast<Bubble*>(node)) {
    Bubble* bubble = dynamic_cast<Bubble*>(node);
    cerr << ind << "Bubble " << bubble << " " << bubble->tabs() << endl;
  } else if (dynamic_cast<At_Root_Block*>(node)) {
    At_Root_Block* root_block = dynamic_cast<At_Root_Block*>(node);
    cerr << ind << "At_Root_Block " << root_block << " " << root_block->tabs() << endl;
    if (root_block->block()) for(auto i : root_block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Selector_List*>(node)) {
    Selector_List* selector = dynamic_cast<Selector_List*>(node);

    cerr << ind << "Selector_List " << selector
      << " [block:" << selector->last_block() << "]"
      << (selector->last_block() && selector->last_block()->is_root() ? " [root]" : "")
      << " [@media:" << selector->media_block() << "]"
      << (selector->is_optional() ? " [is_optional]": " -")
      << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -")
    << endl;

    for(auto i : selector->elements()) { debug_ast(i, ind + " ", env); }

//  } else if (dynamic_cast<Expression*>(node)) {
//    Expression* expression = dynamic_cast<Expression*>(node);
//    cerr << ind << "Expression " << expression << " " << expression->concrete_type() << endl;

  } else if (dynamic_cast<Parent_Selector*>(node)) {
    Parent_Selector* selector = dynamic_cast<Parent_Selector*>(node);
    cerr << ind << "Parent_Selector " << selector;
    cerr << " <" << prettyprint(selector->pstate().token.ws_before()) << ">" << endl;
    debug_ast(selector->selector(), ind + "->", env);

  } else if (dynamic_cast<Complex_Selector*>(node)) {
    Complex_Selector* selector = dynamic_cast<Complex_Selector*>(node);
    cerr << ind << "Complex_Selector " << selector
      << " [block:" << selector->last_block() << "]"
      << (selector->last_block() && selector->last_block()->is_root() ? " [root]" : "")
      << " [@media:" << selector->media_block() << "]"
      << (selector->is_optional() ? " [is_optional]": " -")
      << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -") << " -> ";
      switch (selector->combinator()) {
        case Complex_Selector::PARENT_OF:   cerr << "{>}"; break;
        case Complex_Selector::PRECEDES:    cerr << "{~}"; break;
        case Complex_Selector::ADJACENT_TO: cerr << "{+}"; break;
        case Complex_Selector::ANCESTOR_OF: cerr << "{ }"; break;
      }
    cerr << " <" << prettyprint(selector->pstate().token.ws_before()) << ">" << endl;
    debug_ast(selector->head(), ind + " ", env);
    debug_ast(selector->tail(), ind + "-", env);
  } else if (dynamic_cast<Compound_Selector*>(node)) {
    Compound_Selector* selector = dynamic_cast<Compound_Selector*>(node);
    cerr << ind << "Compound_Selector " << selector
      << " [block:" << selector->last_block() << "]"
      << (selector->last_block() && selector->last_block()->is_root() ? " [root]" : "")
      << " [@media:" << selector->media_block() << "]"
      << (selector->is_optional() ? " [is_optional]": " -")
      << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -") <<
      " <" << prettyprint(selector->pstate().token.ws_before()) << ">" << endl;
    for(auto i : selector->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Propset*>(node)) {
    Propset* selector = dynamic_cast<Propset*>(node);
    cerr << ind << "Propset " << selector << " " << selector->tabs() << endl;
    if (selector->block()) for(auto i : selector->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Wrapped_Selector*>(node)) {
    Wrapped_Selector* selector = dynamic_cast<Wrapped_Selector*>(node);
    cerr << ind << "Wrapped_Selector " << selector << " <<" << selector->name() << ">>" << (selector->has_line_break() ? " [line-break]": " -") << (selector->has_line_feed() ? " [line-feed]": " -") << endl;
    debug_ast(selector->selector(), ind + " () ", env);
  } else if (dynamic_cast<Pseudo_Selector*>(node)) {
    Pseudo_Selector* selector = dynamic_cast<Pseudo_Selector*>(node);
    cerr << ind << "Pseudo_Selector " << selector << " <<" << selector->name() << ">>" << (selector->has_line_break() ? " [line-break]": " -") << (selector->has_line_feed() ? " [line-feed]": " -") << endl;
    debug_ast(selector->expression(), ind + " <= ", env);
  } else if (dynamic_cast<Attribute_Selector*>(node)) {
    Attribute_Selector* selector = dynamic_cast<Attribute_Selector*>(node);
    cerr << ind << "Attribute_Selector " << selector << " <<" << selector->name() << ">>" << (selector->has_line_break() ? " [line-break]": " -") << (selector->has_line_feed() ? " [line-feed]": " -") << endl;
    debug_ast(selector->value(), ind + "[" + selector->matcher() + "] ", env);
  } else if (dynamic_cast<Selector_Qualifier*>(node)) {
    Selector_Qualifier* selector = dynamic_cast<Selector_Qualifier*>(node);
    cerr << ind << "Selector_Qualifier " << selector << " <<" << selector->name() << ">>" << (selector->has_line_break() ? " [line-break]": " -") << (selector->has_line_feed() ? " [line-feed]": " -") << endl;
  } else if (dynamic_cast<Type_Selector*>(node)) {
    Type_Selector* selector = dynamic_cast<Type_Selector*>(node);
    cerr << ind << "Type_Selector " << selector << " <<" << selector->name() << ">>" << (selector->has_line_break() ? " [line-break]": " -") <<
      " <" << prettyprint(selector->pstate().token.ws_before()) << ">" << endl;
  } else if (dynamic_cast<Selector_Placeholder*>(node)) {

    Selector_Placeholder* selector = dynamic_cast<Selector_Placeholder*>(node);
    cerr << ind << "Selector_Placeholder [" << selector->name() << "] " << selector
      << " [block:" << selector->last_block() << "]"
      << " [@media:" << selector->media_block() << "]"
      << (selector->is_optional() ? " [is_optional]": " -")
      << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -")
    << endl;

  } else if (dynamic_cast<Selector_Reference*>(node)) {
    Selector_Reference* selector = dynamic_cast<Selector_Reference*>(node);
    cerr << ind << "Selector_Reference " << selector << " @ref " << selector->selector() << endl;
  } else if (dynamic_cast<Simple_Selector*>(node)) {
    Simple_Selector* selector = dynamic_cast<Simple_Selector*>(node);
    cerr << ind << "Simple_Selector " << selector << (selector->has_line_break() ? " [line-break]": " -") << (selector->has_line_feed() ? " [line-feed]": " -") << endl;

  } else if (dynamic_cast<Selector_Schema*>(node)) {
    Selector_Schema* selector = dynamic_cast<Selector_Schema*>(node);
    cerr << ind << "Selector_Schema " << selector
      << " [block:" << selector->last_block() << "]"
      << (selector->last_block() && selector->last_block()->is_root() ? " [root]" : "")
      << " [@media:" << selector->media_block() << "]"
      << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -")
    << endl;

    debug_ast(selector->contents(), ind + " ");
    // for(auto i : selector->elements()) { debug_ast(i, ind + " ", env); }

  } else if (dynamic_cast<Selector*>(node)) {
    Selector* selector = dynamic_cast<Selector*>(node);
    cerr << ind << "Selector " << selector
      << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -")
    << endl;

  } else if (dynamic_cast<Media_Query_Expression*>(node)) {
    Media_Query_Expression* block = dynamic_cast<Media_Query_Expression*>(node);
    cerr << ind << "Media_Query_Expression " << block
      << (block->is_interpolated() ? " [is_interpolated]": " -")
    << endl;
    debug_ast(block->feature(), ind + " f) ");
    debug_ast(block->value(), ind + " v) ");

  } else if (dynamic_cast<Media_Query*>(node)) {
    Media_Query* block = dynamic_cast<Media_Query*>(node);
    cerr << ind << "Media_Query " << block
      << (block->is_negated() ? " [is_negated]": " -")
      << (block->is_restricted() ? " [is_restricted]": " -")
    << endl;
    debug_ast(block->media_type(), ind + " ");
    for(auto i : block->elements()) { debug_ast(i, ind + " ", env); }

  } else if (dynamic_cast<Media_Block*>(node)) {
    Media_Block* block = dynamic_cast<Media_Block*>(node);
    cerr << ind << "Media_Block " << block << " " << block->tabs() << endl;
    debug_ast(block->media_queries(), ind + " =@ ");
    debug_ast(block->selector(), ind + " -@ ");
    if (block->block()) for(auto i : block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Feature_Block*>(node)) {
    Feature_Block* block = dynamic_cast<Feature_Block*>(node);
    cerr << ind << "Feature_Block " << block << " " << block->tabs() << endl;
    if (block->block()) for(auto i : block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Block*>(node)) {
    Block* root_block = dynamic_cast<Block*>(node);
    cerr << ind << "Block " << root_block << " " << root_block->tabs() << endl;
    if (root_block->block()) for(auto i : root_block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Warning*>(node)) {
    Warning* block = dynamic_cast<Warning*>(node);
    cerr << ind << "Warning " << block << " " << block->tabs() << endl;
  } else if (dynamic_cast<Error*>(node)) {
    Error* block = dynamic_cast<Error*>(node);
    cerr << ind << "Error " << block << " " << block->tabs() << endl;
  } else if (dynamic_cast<Debug*>(node)) {
    Debug* block = dynamic_cast<Debug*>(node);
    cerr << ind << "Debug " << block << " " << block->tabs() << endl;
  } else if (dynamic_cast<Comment*>(node)) {
    Comment* block = dynamic_cast<Comment*>(node);
    cerr << ind << "Comment " << block << " " << block->tabs() <<
      " <" << prettyprint(block->pstate().token.ws_before()) << ">" << endl;
    debug_ast(block->text(), ind + "// ", env);
  } else if (dynamic_cast<If*>(node)) {
    If* block = dynamic_cast<If*>(node);
    cerr << ind << "If " << block << " " << block->tabs() << endl;
  } else if (dynamic_cast<Return*>(node)) {
    Return* block = dynamic_cast<Return*>(node);
    cerr << ind << "Return " << block << " " << block->tabs() << endl;
  } else if (dynamic_cast<Extension*>(node)) {
    Extension* block = dynamic_cast<Extension*>(node);
    cerr << ind << "Extension " << block << " " << block->tabs() << endl;
    debug_ast(block->selector(), ind + "-> ", env);
  } else if (dynamic_cast<Content*>(node)) {
    Content* block = dynamic_cast<Content*>(node);
    cerr << ind << "Content " << block << " " << block->tabs() << endl;
  } else if (dynamic_cast<Import_Stub*>(node)) {
    Import_Stub* block = dynamic_cast<Import_Stub*>(node);
    cerr << ind << "Import_Stub " << block << " " << block->tabs() << endl;
  } else if (dynamic_cast<Import*>(node)) {
    Import* block = dynamic_cast<Import*>(node);
    cerr << ind << "Import " << block << " " << block->tabs() << endl;
    // vector<string>         files_;
    for (auto imp : block->urls()) debug_ast(imp, "@ ", env);
  } else if (dynamic_cast<Assignment*>(node)) {
    Assignment* block = dynamic_cast<Assignment*>(node);
    cerr << ind << "Assignment " << block << " <<" << block->variable() << ">> " << block->tabs() << endl;
    debug_ast(block->value(), ind + "=", env);
  } else if (dynamic_cast<Declaration*>(node)) {
    Declaration* block = dynamic_cast<Declaration*>(node);
    cerr << ind << "Declaration " << block << " " << block->tabs() << endl;
    debug_ast(block->property(), ind + " prop: ", env);
    debug_ast(block->value(), ind + " value: ", env);
  } else if (dynamic_cast<At_Rule*>(node)) {
    At_Rule* block = dynamic_cast<At_Rule*>(node);
    cerr << ind << "At_Rule " << block << " [" << block->keyword() << "] " << block->tabs() << endl;
    debug_ast(block->value(), ind + "+", env);
    debug_ast(block->selector(), ind + "~", env);
    if (block->block()) for(auto i : block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Each*>(node)) {
    Each* block = dynamic_cast<Each*>(node);
    cerr << ind << "Each " << block << " " << block->tabs() << endl;
    if (block->block()) for(auto i : block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<For*>(node)) {
    For* block = dynamic_cast<For*>(node);
    cerr << ind << "For " << block << " " << block->tabs() << endl;
    if (block->block()) for(auto i : block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<While*>(node)) {
    While* block = dynamic_cast<While*>(node);
    cerr << ind << "While " << block << " " << block->tabs() << endl;
    if (block->block()) for(auto i : block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Definition*>(node)) {
    Definition* block = dynamic_cast<Definition*>(node);
    cerr << ind << "Definition " << block << " " << block->tabs() << endl;
    if (block->block()) for(auto i : block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Mixin_Call*>(node)) {
    Mixin_Call* block = dynamic_cast<Mixin_Call*>(node);
    cerr << ind << "Mixin_Call " << block << " " << block->tabs() << endl;
    if (block->block()) for(auto i : block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Ruleset*>(node)) {
    Ruleset* ruleset = dynamic_cast<Ruleset*>(node);
    cerr << ind << "Ruleset " << ruleset << " " << ruleset->tabs() << endl;
    debug_ast(ruleset->selector(), ind + " ");
    if (ruleset->block()) for(auto i : ruleset->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Block*>(node)) {
    Block* block = dynamic_cast<Block*>(node);
    cerr << ind << "Block " << block << " " << block->tabs() << endl;
    for(auto i : block->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Textual*>(node)) {
    Textual* expression = dynamic_cast<Textual*>(node);
    cerr << ind << "Textual ";
    if (expression->type() == Textual::NUMBER) cerr << " [NUMBER]";
    else if (expression->type() == Textual::PERCENTAGE) cerr << " [PERCENTAGE]";
    else if (expression->type() == Textual::DIMENSION) cerr << " [DIMENSION]";
    else if (expression->type() == Textual::HEX) cerr << " [HEX]";
    cerr << expression << " [" << expression->value() << "]" << endl;
  } else if (dynamic_cast<Variable*>(node)) {
    Variable* expression = dynamic_cast<Variable*>(node);
    cerr << ind << "Variable " << expression << " [" << expression->name() << "]" << endl;
    string name(expression->name());
    if (env && env->has(name)) debug_ast(static_cast<Expression*>((*env)[name]), ind + " -> ", env);
  } else if (dynamic_cast<Function_Call_Schema*>(node)) {
    Function_Call_Schema* expression = dynamic_cast<Function_Call_Schema*>(node);
    cerr << ind << "Function_Call_Schema " << expression << "]" << endl;
    debug_ast(expression->name(), ind + "name: ", env);
    debug_ast(expression->arguments(), ind + " args: ", env);
  } else if (dynamic_cast<Function_Call*>(node)) {
    Function_Call* expression = dynamic_cast<Function_Call*>(node);
    cerr << ind << "Function_Call " << expression << " [" << expression->name() << "]" << endl;
    debug_ast(expression->arguments(), ind + " args: ", env);
  } else if (dynamic_cast<Arguments*>(node)) {
    Arguments* expression = dynamic_cast<Arguments*>(node);
    cerr << ind << "Arguments " << expression << "]" << endl;
    for(auto i : expression->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Argument*>(node)) {
    Argument* expression = dynamic_cast<Argument*>(node);
    cerr << ind << "Argument " << expression << " [" << expression->value() << "]" << endl;
    debug_ast(expression->value(), ind + " value: ", env);
  } else if (dynamic_cast<Unary_Expression*>(node)) {
    Unary_Expression* expression = dynamic_cast<Unary_Expression*>(node);
    cerr << ind << "Unary_Expression " << expression << " [" << expression->type() << "]" << endl;
    debug_ast(expression->operand(), ind + " operand: ", env);
  } else if (dynamic_cast<Binary_Expression*>(node)) {
    Binary_Expression* expression = dynamic_cast<Binary_Expression*>(node);
    cerr << ind << "Binary_Expression " << expression << " [" << expression->type() << "]" << endl;
    debug_ast(expression->left(), ind + " left:  ", env);
    debug_ast(expression->right(), ind + " right: ", env);
  } else if (dynamic_cast<Map*>(node)) {
    Map* expression = dynamic_cast<Map*>(node);
    cerr << ind << "Map " << expression << " [Hashed]" << endl;
  } else if (dynamic_cast<List*>(node)) {
    List* expression = dynamic_cast<List*>(node);
    cerr << ind << "List " << expression << " (" << expression->length() << ") " <<
      (expression->separator() == Sass::List::Separator::COMMA ? "Comma " : "Space ") <<
      " [delayed: " << expression->is_delayed() << "] " <<
      " [interpolant: " << expression->is_interpolant() << "] " <<
      endl;
    for(auto i : expression->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Content*>(node)) {
    Content* expression = dynamic_cast<Content*>(node);
    cerr << ind << "Content " << expression << " [Statement]" << endl;
  } else if (dynamic_cast<Boolean*>(node)) {
    Boolean* expression = dynamic_cast<Boolean*>(node);
    cerr << ind << "Boolean " << expression << " [" << expression->value() << "]" << endl;
  } else if (dynamic_cast<Color*>(node)) {
    Color* expression = dynamic_cast<Color*>(node);
    cerr << ind << "Color " << expression << " [" << expression->r() << ":"  << expression->g() << ":" << expression->b() << "@" << expression->a() << "]" << endl;
  } else if (dynamic_cast<Number*>(node)) {
    Number* expression = dynamic_cast<Number*>(node);
    cerr << ind << "Number " << expression << " [" << expression->value() << expression->unit() << "]" << endl;
  } else if (dynamic_cast<String_Quoted*>(node)) {
    String_Quoted* expression = dynamic_cast<String_Quoted*>(node);
    cerr << ind << "String_Quoted : " << expression << " [" << prettyprint(expression->value()) << "]" <<
      (expression->is_delayed() ? " {delayed}" : "") <<
      (expression->sass_fix_1291() ? " {sass_fix_1291}" : "") <<
      (expression->quote_mark() != 0 ? " {qm:" + string(1, expression->quote_mark()) + "}" : "") <<
      " <" << prettyprint(expression->pstate().token.ws_before()) << ">" << endl;
  } else if (dynamic_cast<String_Constant*>(node)) {
    String_Constant* expression = dynamic_cast<String_Constant*>(node);
    cerr << ind << "String_Constant : " << expression << " [" << prettyprint(expression->value()) << "]" <<
      (expression->is_delayed() ? " {delayed}" : "") <<
      (expression->sass_fix_1291() ? " {sass_fix_1291}" : "") <<
      " <" << prettyprint(expression->pstate().token.ws_before()) << ">" << endl;
  } else if (dynamic_cast<String_Schema*>(node)) {
    String_Schema* expression = dynamic_cast<String_Schema*>(node);
    cerr << ind << "String_Schema " << expression << " " << expression->concrete_type() <<
      (expression->has_interpolants() ? " {has_interpolants}" : "") <<
      endl;
    for(auto i : expression->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<String*>(node)) {
    String* expression = dynamic_cast<String*>(node);
    cerr << ind << "String " << expression << expression->concrete_type() <<
      " " << (expression->sass_fix_1291() ? "{sass_fix_1291}" : "") <<
      endl;
  } else if (dynamic_cast<Expression*>(node)) {
    Expression* expression = dynamic_cast<Expression*>(node);
    cerr << ind << "Expression " << expression << " " << expression->concrete_type() << endl;
  } else if (dynamic_cast<Has_Block*>(node)) {
    Has_Block* has_block = dynamic_cast<Has_Block*>(node);
    cerr << ind << "Has_Block " << has_block << " " << has_block->tabs() << endl;
    if (has_block->block()) for(auto i : has_block->block()->elements()) { debug_ast(i, ind + " ", env); }
  } else if (dynamic_cast<Statement*>(node)) {
    Statement* statement = dynamic_cast<Statement*>(node);
    cerr << ind << "Statement " << statement << " " << statement->tabs() << endl;
  }

  if (ind == "") cerr << "####################################################################\n";
}

#endif // SASS_DEBUGGER
