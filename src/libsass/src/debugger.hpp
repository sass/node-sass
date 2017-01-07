#ifndef SASS_DEBUGGER_H
#define SASS_DEBUGGER_H

#include <string>
#include <sstream>
#include "node.hpp"
#include "ast_fwd_decl.hpp"

using namespace Sass;

inline void debug_ast(AST_Node_Ptr node, std::string ind = "", Env* env = 0);

inline void debug_sources_set(SourcesSet& set, std::string ind = "")
{
  if (ind == "") std::cerr << "#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
  for(auto const &pair : set) {
    debug_ast(&pair, ind + "");
    // debug_ast(set[pair], ind + "first: ");
  }
  if (ind == "") std::cerr << "#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
}

inline std::string str_replace(std::string str, const std::string& oldStr, const std::string& newStr)
{
  size_t pos = 0;
  while((pos = str.find(oldStr, pos)) != std::string::npos)
  {
     str.replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
  return str;
}

inline std::string prettyprint(const std::string& str) {
  std::string clean = str_replace(str, "\n", "\\n");
  clean = str_replace(clean, "	", "\\t");
  clean = str_replace(clean, "\r", "\\r");
  return clean;
}

inline std::string longToHex(long long t) {
  std::stringstream is;
  is << std::hex << t;
  return is.str();
}

inline std::string pstate_source_position(AST_Node_Ptr node)
{
  std::stringstream str;
  Position start(node->pstate());
  Position end(start + node->pstate().offset);
  str << (start.file == std::string::npos ? -1 : start.file)
    << "@[" << start.line << ":" << start.column << "]"
    << "-[" << end.line << ":" << end.column << "]";
#ifdef DEBUG_SHARED_PTR
      str << "x" << node->getRefCount() << ""
      << " " << node->getDbgFile()
      << "@" << node->getDbgLine();
#endif
  return str.str();
}

inline void debug_ast(AST_Node_Ptr node, std::string ind, Env* env)
{
  if (node == 0) return;
  if (ind == "") std::cerr << "####################################################################\n";
  if (dynamic_cast<Bubble_Ptr>(node)) {
    Bubble_Ptr bubble = dynamic_cast<Bubble_Ptr>(node);
    std::cerr << ind << "Bubble " << bubble;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << bubble->tabs();
    std::cerr << std::endl;
    debug_ast(&bubble->node(), ind + " ", env);
  } else if (dynamic_cast<Trace_Ptr>(node)) {
    Trace_Ptr trace = dynamic_cast<Trace_Ptr>(node);
    std::cerr << ind << "Trace " << trace;
    std::cerr << " (" << pstate_source_position(node) << ")"
    << " [name:" << trace->name() << "]"
    << std::endl;
    debug_ast(&trace->block(), ind + " ", env);
  } else if (dynamic_cast<At_Root_Block_Ptr>(node)) {
    At_Root_Block_Ptr root_block = dynamic_cast<At_Root_Block_Ptr>(node);
    std::cerr << ind << "At_Root_Block " << root_block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << root_block->tabs();
    std::cerr << std::endl;
    debug_ast(&root_block->expression(), ind + ":", env);
    debug_ast(&root_block->block(), ind + " ", env);
  } else if (dynamic_cast<Selector_List_Ptr>(node)) {
    Selector_List_Ptr selector = dynamic_cast<Selector_List_Ptr>(node);
    std::cerr << ind << "Selector_List " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " [@media:" << selector->media_block() << "]";
    std::cerr << (selector->is_invisible() ? " [INVISIBLE]": " -");
    std::cerr << (selector->has_placeholder() ? " [PLACEHOLDER]": " -");
    std::cerr << (selector->is_optional() ? " [is_optional]": " -");
    std::cerr << (selector->has_parent_ref() ? " [has-parent]": " -");
    std::cerr << (selector->has_line_break() ? " [line-break]": " -");
    std::cerr << (selector->has_line_feed() ? " [line-feed]": " -");
    std::cerr << std::endl;

    for(const Complex_Selector_Obj& i : selector->elements()) { debug_ast(&i, ind + " ", env); }

//  } else if (dynamic_cast<Expression_Ptr>(node)) {
//    Expression_Ptr expression = dynamic_cast<Expression_Ptr>(node);
//    std::cerr << ind << "Expression " << expression << " " << expression->concrete_type() << std::endl;

  } else if (dynamic_cast<Parent_Selector_Ptr>(node)) {
    Parent_Selector_Ptr selector = dynamic_cast<Parent_Selector_Ptr>(node);
    std::cerr << ind << "Parent_Selector " << selector;
//    if (selector->not_selector()) cerr << " [in_declaration]";
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " [" << (selector->is_real_parent_ref() ? "REAL" : "FAKE") << "]";
    std::cerr << " <" << prettyprint(selector->pstate().token.ws_before()) << ">" << std::endl;
//    debug_ast(selector->selector(), ind + "->", env);

  } else if (dynamic_cast<Complex_Selector_Ptr>(node)) {
    Complex_Selector_Ptr selector = dynamic_cast<Complex_Selector_Ptr>(node);
    std::cerr << ind << "Complex_Selector " << selector
      << " (" << pstate_source_position(node) << ")"
      << " <" << selector->hash() << ">"
      << " [length:" << longToHex(selector->length()) << "]"
      << " [weight:" << longToHex(selector->specificity()) << "]"
      << " [@media:" << selector->media_block() << "]"
      << (selector->is_invisible() ? " [INVISIBLE]": " -")
      << (selector->has_placeholder() ? " [PLACEHOLDER]": " -")
      << (selector->is_optional() ? " [is_optional]": " -")
      << (selector->has_parent_ref() ? " [has parent]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -")
      << (selector->has_line_break() ? " [line-break]": " -")
      << " -- ";
      std::string del;
      switch (selector->combinator()) {
        case Complex_Selector::PARENT_OF:   del = ">"; break;
        case Complex_Selector::PRECEDES:    del = "~"; break;
        case Complex_Selector::ADJACENT_TO: del = "+"; break;
        case Complex_Selector::ANCESTOR_OF: del = " "; break;
        case Complex_Selector::REFERENCE:   del = "//"; break;
      }
      // if (del = "/") del += selector->reference()->perform(&to_string) + "/";
    std::cerr << " <" << prettyprint(selector->pstate().token.ws_before()) << ">" << std::endl;
    debug_ast(&selector->head(), ind + " " /* + "[" + del + "]" */, env);
    if (selector->tail()) {
      debug_ast(&selector->tail(), ind + "{" + del + "}", env);
    } else if(del != " ") {
      std::cerr << ind << " |" << del << "| {trailing op}" << std::endl;
    }
    SourcesSet set = selector->sources();
    // debug_sources_set(set, ind + "  @--> ");
  } else if (dynamic_cast<Compound_Selector_Ptr>(node)) {
    Compound_Selector_Ptr selector = dynamic_cast<Compound_Selector_Ptr>(node);
    std::cerr << ind << "Compound_Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " [weight:" << longToHex(selector->specificity()) << "]";
    std::cerr << " [@media:" << selector->media_block() << "]";
    std::cerr << (selector->extended() ? " [extended]": " -");
    std::cerr << (selector->is_optional() ? " [is_optional]": " -");
    std::cerr << (selector->has_parent_ref() ? " [has-parent]": " -");
    std::cerr << (selector->has_line_break() ? " [line-break]": " -");
    std::cerr << (selector->has_line_feed() ? " [line-feed]": " -");
    std::cerr << " <" << prettyprint(selector->pstate().token.ws_before()) << ">" << std::endl;
    for(const Simple_Selector_Obj& i : selector->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Wrapped_Selector_Ptr>(node)) {
    Wrapped_Selector_Ptr selector = dynamic_cast<Wrapped_Selector_Ptr>(node);
    std::cerr << ind << "Wrapped_Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " <<" << selector->ns_name() << ">>";
    std::cerr << (selector->is_optional() ? " [is_optional]": " -");
    std::cerr << (selector->has_parent_ref() ? " [has-parent]": " -");
    std::cerr << (selector->has_line_break() ? " [line-break]": " -");
    std::cerr << (selector->has_line_feed() ? " [line-feed]": " -");
    std::cerr << std::endl;
    debug_ast(&selector->selector(), ind + " () ", env);
  } else if (dynamic_cast<Pseudo_Selector_Ptr>(node)) {
    Pseudo_Selector_Ptr selector = dynamic_cast<Pseudo_Selector_Ptr>(node);
    std::cerr << ind << "Pseudo_Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " <<" << selector->ns_name() << ">>";
    std::cerr << (selector->is_optional() ? " [is_optional]": " -");
    std::cerr << (selector->has_parent_ref() ? " [has-parent]": " -");
    std::cerr << (selector->has_line_break() ? " [line-break]": " -");
    std::cerr << (selector->has_line_feed() ? " [line-feed]": " -");
    std::cerr << std::endl;
    debug_ast(&selector->expression(), ind + " <= ", env);
  } else if (dynamic_cast<Attribute_Selector_Ptr>(node)) {
    Attribute_Selector_Ptr selector = dynamic_cast<Attribute_Selector_Ptr>(node);
    std::cerr << ind << "Attribute_Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " <<" << selector->ns_name() << ">>";
    std::cerr << (selector->is_optional() ? " [is_optional]": " -");
    std::cerr << (selector->has_parent_ref() ? " [has-parent]": " -");
    std::cerr << (selector->has_line_break() ? " [line-break]": " -");
    std::cerr << (selector->has_line_feed() ? " [line-feed]": " -");
    std::cerr << std::endl;
    debug_ast(&selector->value(), ind + "[" + selector->matcher() + "] ", env);
  } else if (dynamic_cast<Class_Selector_Ptr>(node)) {
    Class_Selector_Ptr selector = dynamic_cast<Class_Selector_Ptr>(node);
    std::cerr << ind << "Class_Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " <<" << selector->ns_name() << ">>";
    std::cerr << (selector->is_optional() ? " [is_optional]": " -");
    std::cerr << (selector->has_parent_ref() ? " [has-parent]": " -");
    std::cerr << (selector->has_line_break() ? " [line-break]": " -");
    std::cerr << (selector->has_line_feed() ? " [line-feed]": " -");
    std::cerr << std::endl;
  } else if (dynamic_cast<Id_Selector_Ptr>(node)) {
    Id_Selector_Ptr selector = dynamic_cast<Id_Selector_Ptr>(node);
    std::cerr << ind << "Id_Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " <<" << selector->ns_name() << ">>";
    std::cerr << (selector->is_optional() ? " [is_optional]": " -");
    std::cerr << (selector->has_parent_ref() ? " [has-parent]": " -");
    std::cerr << (selector->has_line_break() ? " [line-break]": " -");
    std::cerr << (selector->has_line_feed() ? " [line-feed]": " -");
    std::cerr << std::endl;
  } else if (dynamic_cast<Element_Selector_Ptr>(node)) {
    Element_Selector_Ptr selector = dynamic_cast<Element_Selector_Ptr>(node);
    std::cerr << ind << "Element_Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <" << selector->hash() << ">";
    std::cerr << " <<" << selector->ns_name() << ">>";
    std::cerr << (selector->is_optional() ? " [is_optional]": " -");
    std::cerr << (selector->has_parent_ref() ? " [has-parent]": " -");
    std::cerr << (selector->has_line_break() ? " [line-break]": " -");
    std::cerr << (selector->has_line_feed() ? " [line-feed]": " -");
    std::cerr << " <" << prettyprint(selector->pstate().token.ws_before()) << ">";
    std::cerr << std::endl;
  } else if (dynamic_cast<Placeholder_Selector_Ptr>(node)) {

    Placeholder_Selector_Ptr selector = dynamic_cast<Placeholder_Selector_Ptr>(node);
    std::cerr << ind << "Placeholder_Selector [" << selector->ns_name() << "] " << selector;
    std::cerr << " (" << pstate_source_position(selector) << ")"
      << " <" << selector->hash() << ">"
      << " [@media:" << selector->media_block() << "]"
      << (selector->is_optional() ? " [is_optional]": " -")
      << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -")
    << std::endl;

  } else if (dynamic_cast<Simple_Selector*>(node)) {
    Simple_Selector* selector = dynamic_cast<Simple_Selector*>(node);
    std::cerr << ind << "Simple_Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << (selector->has_line_break() ? " [line-break]": " -") << (selector->has_line_feed() ? " [line-feed]": " -") << std::endl;

  } else if (dynamic_cast<Selector_Schema_Ptr>(node)) {
    Selector_Schema_Ptr selector = dynamic_cast<Selector_Schema_Ptr>(node);
    std::cerr << ind << "Selector_Schema " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")"
      << (selector->at_root() && selector->at_root() ? " [@ROOT]" : "")
      << " [@media:" << selector->media_block() << "]"
      << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -")
    << std::endl;

    debug_ast(&selector->contents(), ind + " ");
    // for(auto i : selector->elements()) { debug_ast(i, ind + " ", env); }

  } else if (dynamic_cast<Selector_Ptr>(node)) {
    Selector_Ptr selector = dynamic_cast<Selector_Ptr>(node);
    std::cerr << ind << "Selector " << selector;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << (selector->has_line_break() ? " [line-break]": " -")
      << (selector->has_line_feed() ? " [line-feed]": " -")
    << std::endl;

  } else if (dynamic_cast<Media_Query_Expression_Ptr>(node)) {
    Media_Query_Expression_Ptr block = dynamic_cast<Media_Query_Expression_Ptr>(node);
    std::cerr << ind << "Media_Query_Expression " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << (block->is_interpolated() ? " [is_interpolated]": " -")
    << std::endl;
    debug_ast(&block->feature(), ind + " feature) ");
    debug_ast(&block->value(), ind + " value) ");

  } else if (dynamic_cast<Media_Query_Ptr>(node)) {
    Media_Query_Ptr block = dynamic_cast<Media_Query_Ptr>(node);
    std::cerr << ind << "Media_Query " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << (block->is_negated() ? " [is_negated]": " -")
      << (block->is_restricted() ? " [is_restricted]": " -")
    << std::endl;
    debug_ast(&block->media_type(), ind + " ");
    for(const auto& i : block->elements()) { debug_ast(&i, ind + " ", env); }

  } else if (dynamic_cast<Media_Block_Ptr>(node)) {
    Media_Block_Ptr block = dynamic_cast<Media_Block_Ptr>(node);
    std::cerr << ind << "Media_Block " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    debug_ast(&block->media_queries(), ind + " =@ ");
    if (block->block()) for(const Statement_Obj& i : block->block()->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Supports_Block_Ptr>(node)) {
    Supports_Block_Ptr block = dynamic_cast<Supports_Block_Ptr>(node);
    std::cerr << ind << "Supports_Block " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    debug_ast(&block->condition(), ind + " =@ ");
    debug_ast(&block->block(), ind + " <>");
  } else if (dynamic_cast<Supports_Operator_Ptr>(node)) {
    Supports_Operator_Ptr block = dynamic_cast<Supports_Operator_Ptr>(node);
    std::cerr << ind << "Supports_Operator " << block;
    std::cerr << " (" << pstate_source_position(node) << ")"
    << std::endl;
    debug_ast(&block->left(), ind + " left) ");
    debug_ast(&block->right(), ind + " right) ");
  } else if (dynamic_cast<Supports_Negation_Ptr>(node)) {
    Supports_Negation_Ptr block = dynamic_cast<Supports_Negation_Ptr>(node);
    std::cerr << ind << "Supports_Negation " << block;
    std::cerr << " (" << pstate_source_position(node) << ")"
    << std::endl;
    debug_ast(&block->condition(), ind + " condition) ");
  } else if (dynamic_cast<At_Root_Query_Ptr>(node)) {
    At_Root_Query_Ptr block = dynamic_cast<At_Root_Query_Ptr>(node);
    std::cerr << ind << "At_Root_Query " << block;
    std::cerr << " (" << pstate_source_position(node) << ")"
    << std::endl;
    debug_ast(&block->feature(), ind + " feature) ");
    debug_ast(&block->value(), ind + " value) ");
  } else if (dynamic_cast<Supports_Declaration_Ptr>(node)) {
    Supports_Declaration_Ptr block = dynamic_cast<Supports_Declaration_Ptr>(node);
    std::cerr << ind << "Supports_Declaration " << block;
    std::cerr << " (" << pstate_source_position(node) << ")"
    << std::endl;
    debug_ast(&block->feature(), ind + " feature) ");
    debug_ast(&block->value(), ind + " value) ");
  } else if (dynamic_cast<Block_Ptr>(node)) {
    Block_Ptr root_block = dynamic_cast<Block_Ptr>(node);
    std::cerr << ind << "Block " << root_block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    if (root_block->is_root()) std::cerr << " [root]";
    std::cerr << " " << root_block->tabs() << std::endl;
    for(const Statement_Obj& i : root_block->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Warning_Ptr>(node)) {
    Warning_Ptr block = dynamic_cast<Warning_Ptr>(node);
    std::cerr << ind << "Warning " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    debug_ast(&block->message(), ind + " : ");
  } else if (dynamic_cast<Error_Ptr>(node)) {
    Error_Ptr block = dynamic_cast<Error_Ptr>(node);
    std::cerr << ind << "Error " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
  } else if (dynamic_cast<Debug_Ptr>(node)) {
    Debug_Ptr block = dynamic_cast<Debug_Ptr>(node);
    std::cerr << ind << "Debug " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    debug_ast(&block->value(), ind + " ");
  } else if (dynamic_cast<Comment_Ptr>(node)) {
    Comment_Ptr block = dynamic_cast<Comment_Ptr>(node);
    std::cerr << ind << "Comment " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() <<
      " <" << prettyprint(block->pstate().token.ws_before()) << ">" << std::endl;
    debug_ast(&block->text(), ind + "// ", env);
  } else if (dynamic_cast<If_Ptr>(node)) {
    If_Ptr block = dynamic_cast<If_Ptr>(node);
    std::cerr << ind << "If " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    debug_ast(&block->predicate(), ind + " = ");
    debug_ast(&block->block(), ind + " <>");
    debug_ast(&block->alternative(), ind + " ><");
  } else if (dynamic_cast<Return_Ptr>(node)) {
    Return_Ptr block = dynamic_cast<Return_Ptr>(node);
    std::cerr << ind << "Return " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
  } else if (dynamic_cast<Extension_Ptr>(node)) {
    Extension_Ptr block = dynamic_cast<Extension_Ptr>(node);
    std::cerr << ind << "Extension " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    debug_ast(&block->selector(), ind + "-> ", env);
  } else if (dynamic_cast<Content_Ptr>(node)) {
    Content_Ptr block = dynamic_cast<Content_Ptr>(node);
    std::cerr << ind << "Content " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [@media:" << block->media_block() << "]";
    std::cerr << " " << block->tabs() << std::endl;
  } else if (dynamic_cast<Import_Stub_Ptr>(node)) {
    Import_Stub_Ptr block = dynamic_cast<Import_Stub_Ptr>(node);
    std::cerr << ind << "Import_Stub " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << block->imp_path() << "] ";
    std::cerr << " " << block->tabs() << std::endl;
  } else if (dynamic_cast<Import_Ptr>(node)) {
    Import_Ptr block = dynamic_cast<Import_Ptr>(node);
    std::cerr << ind << "Import " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    // std::vector<std::string>         files_;
    for (auto imp : block->urls()) debug_ast(&imp, ind + "@: ", env);
    debug_ast(&block->import_queries(), ind + "@@ ");
  } else if (dynamic_cast<Assignment_Ptr>(node)) {
    Assignment_Ptr block = dynamic_cast<Assignment_Ptr>(node);
    std::cerr << ind << "Assignment " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " <<" << block->variable() << ">> " << block->tabs() << std::endl;
    debug_ast(&block->value(), ind + "=", env);
  } else if (dynamic_cast<Declaration_Ptr>(node)) {
    Declaration_Ptr block = dynamic_cast<Declaration_Ptr>(node);
    std::cerr << ind << "Declaration " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    debug_ast(&block->property(), ind + " prop: ", env);
    debug_ast(&block->value(), ind + " value: ", env);
    debug_ast(&block->block(), ind + " ", env);
  } else if (dynamic_cast<Keyframe_Rule_Ptr>(node)) {
    Keyframe_Rule_Ptr has_block = dynamic_cast<Keyframe_Rule_Ptr>(node);
    std::cerr << ind << "Keyframe_Rule " << has_block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << has_block->tabs() << std::endl;
    if (has_block->name()) debug_ast(&has_block->name(), ind + "@");
    if (has_block->block()) for(const Statement_Obj& i : has_block->block()->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Directive_Ptr>(node)) {
    Directive_Ptr block = dynamic_cast<Directive_Ptr>(node);
    std::cerr << ind << "Directive " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << block->keyword() << "] " << block->tabs() << std::endl;
    debug_ast(&block->selector(), ind + "~", env);
    debug_ast(&block->value(), ind + "+", env);
    if (block->block()) for(const Statement_Obj& i : block->block()->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Each_Ptr>(node)) {
    Each_Ptr block = dynamic_cast<Each_Ptr>(node);
    std::cerr << ind << "Each " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    if (block->block()) for(const Statement_Obj& i : block->block()->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<For_Ptr>(node)) {
    For_Ptr block = dynamic_cast<For_Ptr>(node);
    std::cerr << ind << "For " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    if (block->block()) for(const Statement_Obj& i : block->block()->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<While_Ptr>(node)) {
    While_Ptr block = dynamic_cast<While_Ptr>(node);
    std::cerr << ind << "While " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << block->tabs() << std::endl;
    if (block->block()) for(const Statement_Obj& i : block->block()->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Definition_Ptr>(node)) {
    Definition_Ptr block = dynamic_cast<Definition_Ptr>(node);
    std::cerr << ind << "Definition " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [name: " << block->name() << "] ";
    std::cerr << " [type: " << (block->type() == Sass::Definition::Type::MIXIN ? "Mixin " : "Function ") << "] ";
    // this seems to lead to segfaults some times?
    // std::cerr << " [signature: " << block->signature() << "] ";
    std::cerr << " [native: " << block->native_function() << "] ";
    std::cerr << " " << block->tabs() << std::endl;
    debug_ast(&block->parameters(), ind + " params: ", env);
    if (block->block()) debug_ast(&block->block(), ind + " ", env);
  } else if (dynamic_cast<Mixin_Call_Ptr>(node)) {
    Mixin_Call_Ptr block = dynamic_cast<Mixin_Call_Ptr>(node);
    std::cerr << ind << "Mixin_Call " << block << " " << block->tabs();
    std::cerr << " (" << pstate_source_position(block) << ")";
    std::cerr << " [" <<  block->name() << "]";
    std::cerr << " [has_content: " << block->has_content() << "] " << std::endl;
    debug_ast(&block->arguments(), ind + " args: ");
    if (block->block()) debug_ast(&block->block(), ind + " ", env);
  } else if (Ruleset_Ptr ruleset = dynamic_cast<Ruleset_Ptr>(node)) {
    std::cerr << ind << "Ruleset " << ruleset;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [indent: " << ruleset->tabs() << "]";
    std::cerr << (ruleset->is_invisible() ? " [INVISIBLE]" : "");
    std::cerr << (ruleset->at_root() ? " [@ROOT]" : "");
    std::cerr << (ruleset->is_root() ? " [root]" : "");
    std::cerr << std::endl;
    debug_ast(&ruleset->selector(), ind + ">");
    debug_ast(&ruleset->block(), ind + " ");
  } else if (dynamic_cast<Block_Ptr>(node)) {
    Block_Ptr block = dynamic_cast<Block_Ptr>(node);
    std::cerr << ind << "Block " << block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << (block->is_invisible() ? " [INVISIBLE]" : "");
    std::cerr << " [indent: " << block->tabs() << "]" << std::endl;
    for(const Statement_Obj& i : block->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Textual_Ptr>(node)) {
    Textual_Ptr expression = dynamic_cast<Textual_Ptr>(node);
    std::cerr << ind << "Textual " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    if (expression->type() == Textual::NUMBER) std::cerr << " [NUMBER]";
    else if (expression->type() == Textual::PERCENTAGE) std::cerr << " [PERCENTAGE]";
    else if (expression->type() == Textual::DIMENSION) std::cerr << " [DIMENSION]";
    else if (expression->type() == Textual::HEX) std::cerr << " [HEX]";
    std::cerr << " [" << expression->value() << "]";
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    if (expression->is_delayed()) std::cerr << " [delayed]";
    std::cerr << std::endl;
  } else if (dynamic_cast<Variable_Ptr>(node)) {
    Variable_Ptr expression = dynamic_cast<Variable_Ptr>(node);
    std::cerr << ind << "Variable " << expression;
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << expression->name() << "]" << std::endl;
    std::string name(expression->name());
    if (env && env->has(name)) debug_ast(SASS_MEMORY_CAST(Expression, (*env)[name]), ind + " -> ", env);
  } else if (dynamic_cast<Function_Call_Schema_Ptr>(node)) {
    Function_Call_Schema_Ptr expression = dynamic_cast<Function_Call_Schema_Ptr>(node);
    std::cerr << ind << "Function_Call_Schema " << expression;
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << "" << std::endl;
    debug_ast(&expression->name(), ind + "name: ", env);
    debug_ast(&expression->arguments(), ind + " args: ", env);
  } else if (dynamic_cast<Function_Call_Ptr>(node)) {
    Function_Call_Ptr expression = dynamic_cast<Function_Call_Ptr>(node);
    std::cerr << ind << "Function_Call " << expression;
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << expression->name() << "]";
    if (expression->is_delayed()) std::cerr << " [delayed]";
    if (expression->is_interpolant()) std::cerr << " [interpolant]";
    std::cerr << std::endl;
    debug_ast(&expression->arguments(), ind + " args: ", env);
  } else if (dynamic_cast<Arguments_Ptr>(node)) {
    Arguments_Ptr expression = dynamic_cast<Arguments_Ptr>(node);
    std::cerr << ind << "Arguments " << expression;
    if (expression->is_delayed()) std::cerr << " [delayed]";
    std::cerr << " (" << pstate_source_position(node) << ")";
    if (expression->has_named_arguments()) std::cerr << " [has_named_arguments]";
    if (expression->has_rest_argument()) std::cerr << " [has_rest_argument]";
    if (expression->has_keyword_argument()) std::cerr << " [has_keyword_argument]";
    std::cerr << std::endl;
    for(const Argument_Obj& i : expression->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Argument_Ptr>(node)) {
    Argument_Ptr expression = dynamic_cast<Argument_Ptr>(node);
    std::cerr << ind << "Argument " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << expression->value() << "]";
    std::cerr << " [name: " << expression->name() << "] ";
    std::cerr << " [rest: " << expression->is_rest_argument() << "] ";
    std::cerr << " [keyword: " << expression->is_keyword_argument() << "] " << std::endl;
    debug_ast(&expression->value(), ind + " value: ", env);
  } else if (dynamic_cast<Parameters_Ptr>(node)) {
    Parameters_Ptr expression = dynamic_cast<Parameters_Ptr>(node);
    std::cerr << ind << "Parameters " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [has_optional: " << expression->has_optional_parameters() << "] ";
    std::cerr << " [has_rest: " << expression->has_rest_parameter() << "] ";
    std::cerr << std::endl;
    for(const Parameter_Obj& i : expression->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Parameter_Ptr>(node)) {
    Parameter_Ptr expression = dynamic_cast<Parameter_Ptr>(node);
    std::cerr << ind << "Parameter " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [name: " << expression->name() << "] ";
    std::cerr << " [default: " << expression->default_value() << "] ";
    std::cerr << " [rest: " << expression->is_rest_parameter() << "] " << std::endl;
  } else if (dynamic_cast<Unary_Expression_Ptr>(node)) {
    Unary_Expression_Ptr expression = dynamic_cast<Unary_Expression_Ptr>(node);
    std::cerr << ind << "Unary_Expression " << expression;
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    std::cerr << " [delayed: " << expression->is_delayed() << "] ";
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << expression->type() << "]" << std::endl;
    debug_ast(&expression->operand(), ind + " operand: ", env);
  } else if (dynamic_cast<Binary_Expression_Ptr>(node)) {
    Binary_Expression_Ptr expression = dynamic_cast<Binary_Expression_Ptr>(node);
    std::cerr << ind << "Binary_Expression " << expression;
    if (expression->is_interpolant()) std::cerr << " [is interpolant] ";
    if (expression->is_left_interpolant()) std::cerr << " [left interpolant] ";
    if (expression->is_right_interpolant()) std::cerr << " [right interpolant] ";
    std::cerr << " [delayed: " << expression->is_delayed() << "] ";
    std::cerr << " [ws_before: " << expression->op().ws_before << "] ";
    std::cerr << " [ws_after: " << expression->op().ws_after << "] ";
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << expression->type_name() << "]" << std::endl;
    debug_ast(&expression->left(), ind + " left:  ", env);
    debug_ast(&expression->right(), ind + " right: ", env);
  } else if (dynamic_cast<Map_Ptr>(node)) {
    Map_Ptr expression = dynamic_cast<Map_Ptr>(node);
    std::cerr << ind << "Map " << expression;
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [Hashed]" << std::endl;
    for (const auto& i : expression->elements()) {
      debug_ast(&i.first, ind + " key: ");
      debug_ast(&i.second, ind + " val: ");
    }
  } else if (dynamic_cast<List_Ptr>(node)) {
    List_Ptr expression = dynamic_cast<List_Ptr>(node);
    std::cerr << ind << "List " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " (" << expression->length() << ") " <<
      (expression->separator() == SASS_COMMA ? "Comma " : expression->separator() == SASS_HASH ? "Map" : "Space ") <<
      " [delayed: " << expression->is_delayed() << "] " <<
      " [interpolant: " << expression->is_interpolant() << "] " <<
      " [listized: " << expression->from_selector() << "] " <<
      " [arglist: " << expression->is_arglist() << "] " <<
      " [hash: " << expression->hash() << "] " <<
      std::endl;
    for(const auto& i : expression->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Content_Ptr>(node)) {
    Content_Ptr expression = dynamic_cast<Content_Ptr>(node);
    std::cerr << ind << "Content " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [@media:" << expression->media_block() << "]";
    std::cerr << " [Statement]" << std::endl;
  } else if (dynamic_cast<Boolean_Ptr>(node)) {
    Boolean_Ptr expression = dynamic_cast<Boolean_Ptr>(node);
    std::cerr << ind << "Boolean " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    std::cerr << " [" << expression->value() << "]" << std::endl;
  } else if (dynamic_cast<Color_Ptr>(node)) {
    Color_Ptr expression = dynamic_cast<Color_Ptr>(node);
    std::cerr << ind << "Color " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [delayed: " << expression->is_delayed() << "] ";
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    std::cerr << " [" << expression->r() << ":"  << expression->g() << ":" << expression->b() << "@" << expression->a() << "]" << std::endl;
  } else if (dynamic_cast<Number_Ptr>(node)) {
    Number_Ptr expression = dynamic_cast<Number_Ptr>(node);
    std::cerr << ind << "Number " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] ";
    std::cerr << " [" << expression->value() << expression->unit() << "]" <<
      " [hash: " << expression->hash() << "] " <<
      std::endl;
  } else if (dynamic_cast<Null_Ptr>(node)) {
    Null_Ptr expression = dynamic_cast<Null_Ptr>(node);
    std::cerr << ind << "Null " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [interpolant: " << expression->is_interpolant() << "] "
      // " [hash: " << expression->hash() << "] "
      << std::endl;
  } else if (dynamic_cast<String_Quoted_Ptr>(node)) {
    String_Quoted_Ptr expression = dynamic_cast<String_Quoted_Ptr>(node);
    std::cerr << ind << "String_Quoted " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << prettyprint(expression->value()) << "]";
    if (expression->is_delayed()) std::cerr << " [delayed]";
    if (expression->is_interpolant()) std::cerr << " [interpolant]";
    if (expression->quote_mark()) std::cerr << " [quote_mark: " << expression->quote_mark() << "]";
    std::cerr << " <" << prettyprint(expression->pstate().token.ws_before()) << ">" << std::endl;
  } else if (dynamic_cast<String_Constant_Ptr>(node)) {
    String_Constant_Ptr expression = dynamic_cast<String_Constant_Ptr>(node);
    std::cerr << ind << "String_Constant " << expression;
    if (expression->concrete_type()) {
      std::cerr << " " << expression->concrete_type();
    }
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " [" << prettyprint(expression->value()) << "]";
    if (expression->is_delayed()) std::cerr << " [delayed]";
    if (expression->is_interpolant()) std::cerr << " [interpolant]";
    std::cerr << " <" << prettyprint(expression->pstate().token.ws_before()) << ">" << std::endl;
  } else if (dynamic_cast<String_Schema_Ptr>(node)) {
    String_Schema_Ptr expression = dynamic_cast<String_Schema_Ptr>(node);
    std::cerr << ind << "String_Schema " << expression;
    std::cerr << " (" << pstate_source_position(expression) << ")";
    std::cerr << " " << expression->concrete_type();
    std::cerr << " (" << pstate_source_position(node) << ")";
    if (expression->is_delayed()) std::cerr << " [delayed]";
    if (expression->is_interpolant()) std::cerr << " [is interpolant]";
    if (expression->has_interpolant()) std::cerr << " [has interpolant]";
    if (expression->is_left_interpolant()) std::cerr << " [left interpolant] ";
    if (expression->is_right_interpolant()) std::cerr << " [right interpolant] ";
    std::cerr << " <" << prettyprint(expression->pstate().token.ws_before()) << ">" << std::endl;
    for(const auto& i : expression->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<String_Ptr>(node)) {
    String_Ptr expression = dynamic_cast<String_Ptr>(node);
    std::cerr << ind << "String " << expression;
    std::cerr << " " << expression->concrete_type();
    std::cerr << " (" << pstate_source_position(node) << ")";
    if (expression->is_interpolant()) std::cerr << " [interpolant]";
    std::cerr << " <" << prettyprint(expression->pstate().token.ws_before()) << ">" << std::endl;
  } else if (dynamic_cast<Expression_Ptr>(node)) {
    Expression_Ptr expression = dynamic_cast<Expression_Ptr>(node);
    std::cerr << ind << "Expression " << expression;
    std::cerr << " (" << pstate_source_position(node) << ")";
    switch (expression->concrete_type()) {
      case Expression::Concrete_Type::NONE: std::cerr << " [NONE]"; break;
      case Expression::Concrete_Type::BOOLEAN: std::cerr << " [BOOLEAN]"; break;
      case Expression::Concrete_Type::NUMBER: std::cerr << " [NUMBER]"; break;
      case Expression::Concrete_Type::COLOR: std::cerr << " [COLOR]"; break;
      case Expression::Concrete_Type::STRING: std::cerr << " [STRING]"; break;
      case Expression::Concrete_Type::LIST: std::cerr << " [LIST]"; break;
      case Expression::Concrete_Type::MAP: std::cerr << " [MAP]"; break;
      case Expression::Concrete_Type::SELECTOR: std::cerr << " [SELECTOR]"; break;
      case Expression::Concrete_Type::NULL_VAL: std::cerr << " [NULL_VAL]"; break;
      case Expression::Concrete_Type::C_WARNING: std::cerr << " [C_WARNING]"; break;
      case Expression::Concrete_Type::C_ERROR: std::cerr << " [C_ERROR]"; break;
      case Expression::Concrete_Type::FUNCTION: std::cerr << " [FUNCTION]"; break;
      case Expression::Concrete_Type::NUM_TYPES: std::cerr << " [NUM_TYPES]"; break;
    }
    std::cerr << std::endl;
  } else if (dynamic_cast<Has_Block_Ptr>(node)) {
    Has_Block_Ptr has_block = dynamic_cast<Has_Block_Ptr>(node);
    std::cerr << ind << "Has_Block " << has_block;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << has_block->tabs() << std::endl;
    if (has_block->block()) for(const Statement_Obj& i : has_block->block()->elements()) { debug_ast(&i, ind + " ", env); }
  } else if (dynamic_cast<Statement_Ptr>(node)) {
    Statement_Ptr statement = dynamic_cast<Statement_Ptr>(node);
    std::cerr << ind << "Statement " << statement;
    std::cerr << " (" << pstate_source_position(node) << ")";
    std::cerr << " " << statement->tabs() << std::endl;
  }

  if (ind == "") std::cerr << "####################################################################\n";
}

inline void debug_node(Node* node, std::string ind = "")
{
  if (ind == "") std::cerr << "#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
  if (node->isCombinator()) {
    std::cerr << ind;
    std::cerr << "Combinator ";
    std::cerr << node << " ";
    if (node->got_line_feed) std::cerr << "[LF] ";
    switch (node->combinator()) {
      case Complex_Selector::ADJACENT_TO: std::cerr << "{+} "; break;
      case Complex_Selector::PARENT_OF:   std::cerr << "{>} "; break;
      case Complex_Selector::PRECEDES:    std::cerr << "{~} "; break;
      case Complex_Selector::REFERENCE:   std::cerr << "{@} "; break;
      case Complex_Selector::ANCESTOR_OF: std::cerr << "{ } "; break;
    }
    std::cerr << std::endl;
    // debug_ast(node->combinator(), ind + "  ");
  } else if (node->isSelector()) {
    std::cerr << ind;
    std::cerr << "Selector ";
    std::cerr << node << " ";
    if (node->got_line_feed) std::cerr << "[LF] ";
    std::cerr << std::endl;
    debug_ast(&node->selector(), ind + "  ");
  } else if (node->isCollection()) {
    std::cerr << ind;
    std::cerr << "Collection ";
    std::cerr << node << " ";
    if (node->got_line_feed) std::cerr << "[LF] ";
    std::cerr << std::endl;
    for(auto n : (*node->collection())) {
      debug_node(&n, ind + "  ");
    }
  } else if (node->isNil()) {
    std::cerr << ind;
    std::cerr << "Nil ";
    std::cerr << node << " ";
    if (node->got_line_feed) std::cerr << "[LF] ";
    std::cerr << std::endl;
  } else {
    std::cerr << ind;
    std::cerr << "OTHER ";
    std::cerr << node << " ";
    if (node->got_line_feed) std::cerr << "[LF] ";
    std::cerr << std::endl;
  }
  if (ind == "") std::cerr << "#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
}

/*
inline void debug_ast(const AST_Node_Ptr node, std::string ind = "", Env* env = 0)
{
  debug_ast(const_cast<AST_Node_Ptr>(node), ind, env);
}
*/
inline void debug_node(const Node* node, std::string ind = "")
{
  debug_node(const_cast<Node*>(node), ind);
}

inline void debug_subset_map(Sass::Subset_Map& map, std::string ind = "")
{
  if (ind == "") std::cerr << "#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
  for(auto const &it : map.values()) {
    debug_ast(&it.first, ind + "first: ");
    debug_ast(&it.second, ind + "second: ");
  }
  if (ind == "") std::cerr << "#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
}

typedef std::pair<Complex_Selector_Obj, Compound_Selector_Obj> ExtensionPair;
typedef std::vector<ExtensionPair> SubsetMapEntries;

inline void debug_subset_entries(SubsetMapEntries* entries, std::string ind = "")
{
  if (ind == "") std::cerr << "#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
  for(auto const &pair : *entries) {
    debug_ast(&pair.first, ind + "first: ");
    debug_ast(&pair.second, ind + "second: ");
  }
  if (ind == "") std::cerr << "#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
}

#endif // SASS_DEBUGGER
