#include <cstdlib>
#include <iostream>
#include "parser.hpp"
#include "constants.hpp"
#include "error.hpp"

#ifndef SASS_PRELEXER
#include "prelexer.hpp"
#endif

namespace Sass {
  using namespace std;
  using namespace Constants;

  void Parser::parse_scss()
  {
    read_bom();
    lex< optional_spaces >();
    Selector_Lookahead lookahead_result;
    while (position < end) {
      if (lex< block_comment >()) {
        String_Constant* contents = new (ctx.mem) String_Constant(path, line, lexed);
        Comment*     comment  = new (ctx.mem) Comment(path, line, contents);
        (*root) << comment;
      }
      else if (peek< import >()) {
        (*root) << parse_import();
      }
      else if (peek< mixin >() || peek< function >()) {
        (*root) << parse_definition();
      }
      else if (peek< variable >()) {
        (*root) << parse_assignment();
        if (!lex< exactly<';'> >()) throw_syntax_error("top-level variable binding must be terminated by ';'");
      }
      else if (peek< sequence< optional< exactly<'*'> >, alternatives< identifier_schema, identifier >, optional_spaces, exactly<':'>, optional_spaces, exactly<'{'> > >(position)) {
        (*root) << parse_propset();
      }
      else if ((lookahead_result = lookahead_for_selector(position)).found) {
        (*root) << parse_ruleset(lookahead_result);
      }
      else if (peek< include >() /* || peek< exactly<'+'> >() */) {
        Mixin_Call* mixin_call = parse_mixin_call();
        (*root) << mixin_call;
        if (!mixin_call->block() && !lex< exactly<';'> >()) throw_syntax_error("top-level @include directive must be terminated by ';'");
      }
      else if (peek< if_directive >()) {
        (*root) << parse_if_directive();
      }
      else if (peek< for_directive >()) {
        (*root) << parse_for_directive();
      }
      else if (peek< each_directive >()) {
        (*root) << parse_each_directive();
      }
      else if (peek< while_directive >()) {
        (*root) << parse_while_directive();
      }
      else if (peek< media >()) {
        (*root) << parse_media_block();
      }
      // else if (peek< keyframes >()) {
      //   (*root) << parse_keyframes();
      // }
      else if (peek< warn >()) {
        (*root) << parse_warning();
        if (!lex< exactly<';'> >()) throw_syntax_error("top-level @warn directive must be terminated by ';'");
      }
      // ignore the @charset directive for now
      else if (lex< exactly< charset_kwd > >()) {
        lex< string_constant >();
        lex< exactly<';'> >();
      }
      else if (peek< directive >()) {
        At_Rule* at_rule = parse_directive();
        if (!at_rule->block() && !lex< exactly<';'> >()) {
          throw_syntax_error("top-level blockless directive must be terminated by ';'");
        }
        (*root) << at_rule;
      }
      else {
        lex< spaces_and_comments >();
        if (position >= end) break;
        throw_syntax_error("invalid top-level expression");
      }
      lex< optional_spaces >();
    }
  }

  Import* Parser::parse_import()
  {
    lex< import >();
    Import* imp = new (ctx.mem) Import(path, line);
    bool first = true;
    do {
      if (lex< string_constant >()) {
        imp->strings().push_back(lexed);
      }
      else if (peek< uri_prefix >()) {
        imp->urls().push_back(parse_function_call());
      }
      else {
        if (first) throw_syntax_error("@import directive requires a url or quoted path");
        else throw_syntax_error("expecting another url or quoted path in @import list");
      }
      first = false;
    } while (lex< exactly<','> >());
    return imp;
    // if (lex< uri_prefix >())
    // {
    //   if (peek< string_constant >()) {
    //     String* schema = parse_string();
    //     Import<String*>* import = new (ctx.mem) Import<String*>(path, line, schema);
    //     if (!lex< exactly<')'> >()) throw_syntax_error("unterminated url in @import directive");
    //     return import;
    //   }
    //   else {
    //     const char* beg = position;
    //     const char* end = find_first< exactly<')'> >(position);
    //     if (!end) throw_syntax_error("unterminated url in @import directive");
    //     AST_Node* path_node(ctx.new_AST_Node*(AST_Node*::identifier, path, line, Token::make(beg, end)));
    //     AST_Node* importee(ctx.new_AST_Node*(AST_Node*::css_import, path, line, 1));
    //     importee << path_node;
    //     position = end;
    //     lex< exactly<')'> >();
    //     return importee;
    //   }
    // }
    // if (!lex< string_constant >()) throw_syntax_error("@import directive requires a url or quoted path");
    // string import_path(lexed.unquote());
    // // Try the folder containing the current file first. If that fails, loop
    // // through the include-paths.
    // try {
    //   const char* base_str = path.c_str();
    //   string base_path(Token::make(base_str, Prelexer::folders(base_str)));
    //   string resolved_path(base_path + import_path);
    //   Parser importee(Parser::make_from_file(ctx, resolved_path));
    //   importee.parse_scss();
    //   return importee.root;
    // }
    // catch (string& path) {
    //   // suppress the error and try the include paths
    // }
    // for (vector<string>::iterator path = ctx.include_paths.begin(); path < ctx.include_paths.end(); ++path) {
    //   try {
    //     Parser importee(Parser::make_from_file(ctx, *path + import_path));
    //     importee.parse_scss();
    //     return importee.root;
    //   }
    //   catch (string& path) {
    //     // continue looping
    //   }
    // }
    // // fail after we've tried all include-paths
    // throw_read_error("error reading file \"" + import_path + "\"");
    // // unreachable statement
    // return AST_Node*();
  }

  Definition* Parser::parse_definition()
  {
    Definition::Type which_type;
    if      (lex< mixin >())    which_type = Definition::MIXIN;
    else if (lex< function >()) which_type = Definition::FUNCTION;
    string which_str(lexed);
    if (!lex< identifier >()) throw_syntax_error("invalid name in " + which_str + " definition");
    string name(lexed);
    size_t line_of_def = line;
    Parameters* params = parse_parameters();
    if (!peek< exactly<'{'> >()) throw_syntax_error("body for " + which_str + " " + name + " must begin with a '{'");
    if (which_type == Definition::MIXIN) stack.push_back(mixin_def);
    else stack.push_back(function_def);
    Block* body = parse_block();
    stack.pop_back();
    Definition* def = new (ctx.mem) Definition(path, line_of_def, name, params, body, which_type);
    return def;
  }

  Parameters* Parser::parse_parameters()
  {
    string name(lexed); // for the error message
    Parameters* params = new (ctx.mem) Parameters(path, line);
    if (lex< exactly<'('> >()) {
      // if there's anything there at all
      if (!peek< exactly<')'> >()) {
        do (*params) << parse_parameter();
        while (lex< exactly<','> >());
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("expected a variable name (e.g. $x) or ')' for the parameter list for " + name);
    }
    return params;
  }

  Parameter* Parser::parse_parameter()
  {
    lex< variable >();
    string name(lexed);
    Expression* val = 0;
    bool is_rest = false;
    if (lex< exactly<':'> >()) { // there's a default value
      val = parse_space_list();
    }
    else if (lex< exactly< ellipsis > >()) {
      is_rest = true;
    }
    Parameter* p = new (ctx.mem) Parameter(path, line, name, val, is_rest);
    return p;
  }

  Mixin_Call* Parser::parse_mixin_call()
  {
    lex< include >() /* || lex< exactly<'+'> >() */;
    if (!lex< identifier >()) throw_syntax_error("invalid name in @include directive");
    size_t line_of_call = line;
    string name(lexed);
    Arguments* args = parse_arguments();
    Block* content = 0;
    if (peek< exactly<'{'> >()) {
      content = parse_block();
    }
    Mixin_Call* the_call = new (ctx.mem) Mixin_Call(path, line_of_call, name, args, content);
    return the_call;
  }

  Arguments* Parser::parse_arguments()
  {
    string name(lexed);
    Arguments* args = new (ctx.mem) Arguments(path, line);

    if (lex< exactly<'('> >()) {
      // if there's anything there at all
      if (!peek< exactly<')'> >()) {
        do (*args) << parse_argument();
        while (lex< exactly<','> >());
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("expected a variable name (e.g. $x) or ')' for the parameter list for " + name);
    }

    return args;
  }

  Argument* Parser::parse_argument()
  {
    if (peek< sequence < variable, spaces_and_comments, exactly<':'> > >()) {
      lex< variable >();
      string name(lexed);
      lex< exactly<':'> >();
      Expression* val = parse_space_list();
      Argument* arg = new (ctx.mem) Argument(path, line, val, name);
      return arg;
    }
    else {
      bool is_arglist = false;
      Expression* val = parse_space_list();
      if (lex< exactly< ellipsis > >()) {
        is_arglist = true;
      }
      Argument* arg = new (ctx.mem) Argument(path, line, val, "", is_arglist);
    }
    return 0; // unreachable
  }

  Assignment* Parser::parse_assignment()
  {
    lex< variable >();
    string name(lexed);
    size_t var_line = line;
    if (!lex< exactly<':'> >()) throw_syntax_error("expected ':' after " + name + " in assignment statement");
    Expression* val = parse_list();
    bool is_guarded = lex< default_flag >();
    Assignment* var = new (ctx.mem) Assignment(path, var_line, name, val, is_guarded);
    return var;
  }

  Propset* Parser::parse_propset()
  {
    String* property_segment;
    if (peek< sequence< optional< exactly<'*'> >, identifier_schema > >()) {
      property_segment = parse_identifier_schema();
    }
    else {
      lex< sequence< optional< exactly<'*'> >, identifier > >();
      property_segment = new (ctx.mem) String_Constant(path, line, lexed);
    }
    lex< exactly<':'> >();
    Block* block = parse_block();
    if (block->empty()) throw_syntax_error("namespaced property cannot be empty");
    Propset* propset = new (ctx.mem) Propset(path, line, property_segment, block);
    return propset;
  }

  Ruleset* Parser::parse_ruleset(Selector_Lookahead lookahead)
  {
    Selector* sel;
    if (lookahead.has_interpolants) {
      sel = parse_selector_schema(lookahead.found);
    }
    else {
      sel = parse_selector_group();
    }
    if (!peek< exactly<'{'> >()) throw_syntax_error("expected a '{' after the selector");
    Block* block = parse_block();
    Ruleset* ruleset = new (ctx.mem) Ruleset(path, line, sel, block);
    return ruleset;
  }

  Selector_Schema* Parser::parse_selector_schema(const char* end_of_selector)
  {
    const char* i = position;
    const char* p;
    String_Schema* schema = new (ctx.mem) String_Schema(path, line);

    while (i < end_of_selector) {
      p = find_first_in_interval< exactly<hash_lbrace> >(i, end_of_selector);
      if (p) {
        // accumulate the preceding segment if there is one
        if (i < p) (*schema) << new (ctx.mem) String_Constant(path, line, Token(i, p));
        // find the end of the interpolant and parse it
        const char* j = find_first_in_interval< exactly<rbrace> >(p, end_of_selector);
        Expression* interp_node = Parser::make_from_token(ctx, Token(p+2, j), path, line).parse_list();
        (*schema) << interp_node;
        i = j + 1;
      }
      else { // no interpolants left; add the last segment if there is one
        if (i < end_of_selector) (*schema) << new (ctx.mem) String_Constant(path, line, Token(i, end_of_selector));
        break;
      }
    }
    position = end_of_selector;
    return new (ctx.mem) Selector_Schema(path, line, schema);
  }

  Selector_Group* Parser::parse_selector_group()
  {
    Selector_Group* group = new Selector_Group(path, line);
    do (*group) << parse_selector_combination();
    while (lex< exactly<','> >());
    return group;
  }

  Selector_Combination* Parser::parse_selector_combination()
  {
    Simple_Selector_Sequence* lhs;
    if (peek< exactly<'+'> >() ||
        peek< exactly<'~'> >() ||
        peek< exactly<'>'> >()) {
      // no selector before the combinator
      lhs = 0;
    }
    else {
      lhs = parse_simple_selector_sequence();
    }
    size_t sel_line = line;

    Selector_Combination::Combinator cmb;
    if      (lex< exactly<'+'> >()) cmb = Selector_Combination::ADJACENT_TO;
    else if (lex< exactly<'~'> >()) cmb = Selector_Combination::PRECEDES;
    else if (lex< exactly<'>'> >()) cmb = Selector_Combination::PARENT_OF;
    else                            cmb = Selector_Combination::ANCESTOR_OF;

    Selector_Combination* rhs;
    if (peek< exactly<','> >() ||
        peek< exactly<')'> >() ||
        peek< exactly<'{'> >() ||
        peek< exactly<';'> >()) {
      // no selector after the combinator
      rhs = 0;
    }
    else {
      rhs = parse_selector_combination();
    }

    return new (ctx.mem) Selector_Combination(path, sel_line, cmb, lhs, rhs);
  }

  Simple_Selector_Sequence* Parser::parse_simple_selector_sequence()
  {
    Simple_Selector_Sequence* seq = new (ctx.mem) Simple_Selector_Sequence(path, line);
    // check for backref or type selector, which are only allowed at the front
    if (lex< exactly<'&'> >()) {
      (*seq) << new (ctx.mem) Selector_Reference(path, line);
    }
    else if (lex< alternatives< type_selector, universal, string_constant, dimension, percentage, number > >()) {
      (*seq) << new (ctx.mem) Type_Selector(path, line, lexed);
    }
    else {
      (*seq) << parse_simple_selector();
    }

    while (!peek< spaces >(position) &&
           !(peek < exactly<'+'> >(position) ||
             peek < exactly<'~'> >(position) ||
             peek < exactly<'>'> >(position) ||
             peek < exactly<','> >(position) ||
             peek < exactly<')'> >(position) ||
             peek < exactly<'{'> >(position) ||
             peek < exactly<';'> >(position))) {
      (*seq) << parse_simple_selector();
    }
    return seq;
  }

  Simple_Selector* Parser::parse_simple_selector()
  {
    if (lex< id_name >() || lex< class_name >()) {
      return new (ctx.mem) Selector_Qualifier(path, line, lexed);
    }

    if (lex< string_constant >() || lex< number >()) {
      return new (ctx.mem) Type_Selector(path, line, lexed);
    }
    else if (peek< pseudo_not >()) {
      return parse_negated_selector();
    }
    else if (peek< exactly<':'> >(position)) {
      return parse_pseudo_selector();
    }
    else if (peek< exactly<'['> >(position)) {
      return parse_attribute_selector();
    }
    else {
      throw_syntax_error("invalid selector after " + lexed);
    }
    // unreachable statement
    return 0;
  }

  Negated_Selector* Parser::parse_negated_selector()
  {
    return 0;
  }

  Pseudo_Selector* Parser::parse_pseudo_selector() {
    if (lex< sequence< pseudo_prefix, functional > >()) {
      string name(lexed);
      Expression* expr;
      if (lex< alternatives< even, odd > >()) {
        expr = new (ctx.mem) String_Constant(path, line, lexed);
      }
      else if (peek< binomial >(position)) {
        lex< sequence< optional< coefficient >, exactly<'n'> > >();
        String_Constant* var_coef = new (ctx.mem) String_Constant(path, line, lexed);
        lex< sign >();
        Binary_Expression::Type op = (lexed == "+" ? Binary_Expression::ADD : Binary_Expression::SUB);
        lex< digits >();
        String_Constant* constant = new (ctx.mem) String_Constant(path, line, lexed);
        expr = new (ctx.mem) Binary_Expression(path, line, op, var_coef, constant);
      }
      else if (lex< sequence< optional<sign>,
                              optional<digits>,
                              exactly<'n'> > >()) {
        expr = new (ctx.mem) String_Constant(path, line, lexed);
      }
      else if (lex< sequence< optional<sign>, digits > >()) {
        expr = new (ctx.mem) String_Constant(path, line, lexed);
      }
      else if (lex< identifier >()) {
        expr = new (ctx.mem) String_Constant(path, line, lexed);
      }
      else if (lex< string_constant >()) {
        expr = new (ctx.mem) String_Constant(path, line, lexed);
      }
      else {
        throw_syntax_error("invalid argument to " + name + "...)");
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("unterminated argument to " + name + "...)");
      return new (ctx.mem) Pseudo_Selector(path, line, name, expr);
    }
    else if (lex < sequence< pseudo_prefix, identifier > >()) {
      return new (ctx.mem) Pseudo_Selector(path, line, lexed);
    }
    else {
      throw_syntax_error("unrecognized pseudo-class or pseudo-element");
    }
    // unreachable statement
    return 0;
  }

  Attribute_Selector* Parser::parse_attribute_selector()
  {
    lex< exactly<'['> >();
    if (!lex< type_selector >()) throw_syntax_error("invalid attribute name in attribute selector");
    string name(lexed);
    if (lex< exactly<']'> >()) return new (ctx.mem) Attribute_Selector(path, line, name, "", "");
    if (!lex< alternatives< exact_match, class_match, dash_match,
                            prefix_match, suffix_match, substring_match > >()) {
      throw_syntax_error("invalid operator in attribute selector for " + name);
    }
    string matcher(lexed);
    if (!lex< string_constant >() && !lex< identifier >()) throw_syntax_error("expected a string constant or identifier in attribute selector for " + name);
    string value(lexed);
    if (!lex< exactly<']'> >()) throw_syntax_error("unterminated attribute selector for " + name);
    return new (ctx.mem) Attribute_Selector(path, line, name, matcher, value);
  }

  Block* Parser::parse_block()
  {
    lex< exactly<'{'> >();
    bool semicolon = false;
    Selector_Lookahead lookahead_result;
    Block* block = new (ctx.mem) Block(path, line);
    while (!lex< exactly<'}'> >()) {
      if (semicolon) {
        if (!lex< exactly<';'> >()) throw_syntax_error("non-terminal statement or declaration must end with ';'");
        semicolon = false;
        while (lex< block_comment >()) {
          String_Constant* contents = new (ctx.mem) String_Constant(path, line, lexed);
          Comment*         comment  = new (ctx.mem) Comment(path, line, contents);
          (*block) << comment;
        }
        if (lex< exactly<'}'> >()) break;
      }
      if (lex< block_comment >()) {
        String_Constant* contents = new (ctx.mem) String_Constant(path, line, lexed);
        Comment*         comment  = new (ctx.mem) Comment(path, line, contents);
        (*block) << comment;
      }
      else if (peek< import >(position)) {
        if (stack.back() == mixin_def || stack.back() == function_def) {
          lex< import >(); // to adjust the line number
          throw_syntax_error("@import directive not allowed inside definition of mixin or function");
        }
        (*block) << parse_import();
        // semicolon = true?
      }
      else if (lex< variable >()) {
        (*block) << parse_assignment();
        semicolon = true;
      }
      else if (peek< if_directive >()) {
        (*block) << parse_if_directive();
      }
      else if (peek< for_directive >()) {
        (*block) << parse_for_directive();
      }
      else if (peek< each_directive >()) {
        (*block) << parse_each_directive();
      }
      else if (peek < while_directive >()) {
        (*block) << parse_while_directive();
      }
      else if (lex < return_directive >()) {
        (*block) << new (ctx.mem) Return(path, line, parse_list());
        semicolon = true;
      }
      else if (peek< warn >()) {
        (*block) << parse_warning();
        semicolon = true;
      }
      else if (stack.back() == function_def) {
        throw_syntax_error("only variable declarations and control directives are allowed inside functions");
      }
      else if (peek< include >(position)) {
        Mixin_Call* the_call = parse_mixin_call();
        (*block) << the_call;
        // don't need a semicolon after a content block
        semicolon = (the_call->block()) ? false : true;
      }
      else if (lex< content >()) {
        if (stack.back() != mixin_def) {
          throw_syntax_error("@content may only be used within a mixin");
        }
        (*block) << new (ctx.mem) Content(path, line);
        semicolon = true;
      }
      else if (peek< sequence< optional< exactly<'*'> >, alternatives< identifier_schema, identifier >, optional_spaces, exactly<':'>, optional_spaces, exactly<'{'> > >(position)) {
        (*block) << parse_propset();
      }
      // else if (peek < keyframes >()) {
      //   (*block) << parse_keyframes(inside_of);
      // }
      // else if (peek< sequence< keyf, optional_spaces, exactly<'{'> > >()) {
      //   (*block) << parse_keyframe(inside_of);
      // }
      else if ((lookahead_result = lookahead_for_selector(position)).found) {
        (*block) << parse_ruleset(lookahead_result);
      }
      /*
      else if (peek< exactly<'+'> >()) {
        (*block) << parse_mixin_call();
        semicolon = true;
      }
      */
      else if (lex< extend >()) {
        Selector_Lookahead lookahead = lookahead_for_extension_target(position);
        if (!lookahead.found) throw_syntax_error("invalid selector for @extend");
        Selector* target;
        if (lookahead.has_interpolants) target = parse_selector_schema(lookahead.found);
        else                            target = parse_selector_group();
        (*block) << new (ctx.mem) Extend(path, line, target);
        semicolon = true;
      }
      else if (peek< media >()) {
        (*block) << parse_media_block();
      }
      // ignore the @charset directive for now
      else if (lex< exactly< charset_kwd > >()) {
        lex< string_constant >();
        lex< exactly<';'> >();
      }
      else if (peek< directive >()) {
        At_Rule* at_rule = parse_directive();
        if (!at_rule->block()) {
          semicolon = true;
        }
        (*block) << at_rule;
      }
      // // related to keyframe stuff
      // else if (peek< percentage >() ){
      //   lex< percentage >();
      //   (*block) << ctx.new_AST_Node*(path, line, atof(lexed.begin), AST_Node*::numeric_percentage);
      //   if (peek< exactly<'{'> >()) {
      //     AST_Node* inner(parse_block(AST_Node*()));
      //     (*block) << inner;
      //   }
      // }
      else if (!peek< exactly<';'> >()) {
        if (peek< sequence< optional< exactly<'*'> >, identifier_schema, exactly<':'>, exactly<'{'> > >()) {
          String* prop = parse_identifier_schema();
          Block* inner = parse_block();
          (*block) << new (ctx.mem) Propset(path, line, prop, block);
        }
        else if (peek< sequence< optional< exactly<'*'> >, identifier, exactly<':'>, exactly<'{'> > >()) {
          lex< sequence< optional< exactly<'*'> >, identifier > >();
          String* prop = new (ctx.mem) String_Constant(path, line, lexed);
          Block* inner = parse_block();
          (*block) << new (ctx.mem) Propset(path, line, prop, block);
        }
        else {
          (*block) << parse_declaration();
          semicolon = true;
        }
      }
      else lex< exactly<';'> >();
      while (lex< block_comment >()) {
        String_Constant* contents = new (ctx.mem) String_Constant(path, line, lexed);
        Comment*         comment  = new (ctx.mem) Comment(path, line, contents);
        (*block) << comment;
      }
    }
    return block;
  }

  Declaration* Parser::parse_declaration() {
    String* prop;
    if (peek< sequence< optional< exactly<'*'> >, identifier_schema > >()) {
      prop = parse_identifier_schema();
    }
    else if (lex< sequence< optional< exactly<'*'> >, identifier > >()) {
      prop = new (ctx.mem) String_Constant(path, line, lexed);
    }
    else {
      throw_syntax_error("invalid property name");
    }
    if (!lex< exactly<':'> >()) throw_syntax_error("property \"" + string(lexed) + "\" must be followed by a ':'");
    Expression* list = parse_list();
    return new (ctx.mem) Declaration(path, prop->line(), prop, list, lex<important>());
  }

  Expression* Parser::parse_list()
  {
    return parse_comma_list();
  }

  Expression* Parser::parse_comma_list()
  {
    if (peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<'{'> >(position) ||
        peek< exactly<')'> >(position) ||
        peek< exactly<ellipsis> >(position))
    { return new (ctx.mem) List(path, line, 0); }
    Expression* list1 = parse_space_list();
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< exactly<','> >(position)) return list1;

    List* comma_list = new (ctx.mem) List(path, line, 2, List::comma);
    (*comma_list) << list1;

    while (lex< exactly<','> >())
    {
      Expression* list = parse_space_list();
      (*comma_list) << list;
    }

    return comma_list;
  }

  Expression* Parser::parse_space_list()
  {
    Expression* disj1 = parse_disjunction();
    // if it's a singleton, return it directly; don't wrap it
    if (peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<'{'> >(position) ||
        peek< exactly<')'> >(position) ||
        peek< exactly<','> >(position) ||
        peek< exactly<ellipsis> >(position) ||
        peek< default_flag >(position))
    { return disj1; }

    List* space_list = new (ctx.mem) List(path, line, 2, List::space);
    (*space_list) << disj1;

    while (!(peek< exactly<';'> >(position) ||
             peek< exactly<'}'> >(position) ||
             peek< exactly<'{'> >(position) ||
             peek< exactly<')'> >(position) ||
             peek< exactly<','> >(position) ||
             peek< exactly<ellipsis> >(position) ||
             peek< default_flag >(position)))
    {
      (*space_list) << parse_disjunction();
    }

    return space_list;
  }

  Expression* Parser::parse_disjunction()
  {
    Expression* conj1 = parse_conjunction();
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< sequence< or_op, negate< identifier > > >()) return conj1;

    vector<Expression*> operands;
    while (lex< sequence< or_op, negate< identifier > > >())
      operands.push_back(parse_conjunction());

    return fold_operands(conj1, operands, Binary_Expression::OR);
  }

  Expression* Parser::parse_conjunction()
  {
    Expression* rel1 = parse_relation();
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< sequence< and_op, negate< identifier > > >()) return rel1;

    vector<Expression*> operands;
    while (lex< sequence< and_op, negate< identifier > > >())
      operands.push_back(parse_relation());

    return fold_operands(rel1, operands, Binary_Expression::AND);
  }

  Expression* Parser::parse_relation()
  {
    Expression* expr1 = parse_expression();
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< eq_op >(position)  ||
          peek< neq_op >(position) ||
          peek< gte_op >(position) ||
          peek< gt_op >(position)  ||
          peek< lte_op >(position) ||
          peek< lt_op >(position)))
    { return expr1; }

    Binary_Expression::Type op
    = lex<eq_op>()  ? Binary_Expression::EQ
    : lex<neq_op>() ? Binary_Expression::NEQ
    : lex<gte_op>() ? Binary_Expression::GTE
    : lex<lte_op>() ? Binary_Expression::LTE
    : lex<gt_op>()  ? Binary_Expression::GT
    :                 Binary_Expression::LT;

    Expression* expr2 = parse_expression();

    return new (ctx.mem) Binary_Expression(path, expr1->line(), op, expr1, expr2);
  }

  Expression* Parser::parse_expression()
  {
    Expression* term1 = parse_term();
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'+'> >(position) ||
          peek< sequence< negate< number >, exactly<'-'> > >(position)))
    { return term1; }

    vector<Expression*> operands;
    vector<Binary_Expression::Type> operators;
    while (lex< exactly<'+'> >() || lex< sequence< negate< number >, exactly<'-'> > >()) {
      operators.push_back(lexed == "+" ? Binary_Expression::ADD : Binary_Expression::SUB);
      operands.push_back(parse_term());
    }

    return fold_operands(term1, operands, operators);
  }

  Expression* Parser::parse_term()
  {
    Expression* fact1 = parse_factor();
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'*'> >(position) ||
          peek< exactly<'/'> >(position) ||
          peek< exactly<'%'> >(position)))
    { return fact1; }

    vector<Expression*> operands;
    vector<Binary_Expression::Type> operators;
    while (lex< exactly<'*'> >() || lex< exactly<'/'> >() || lex< exactly<'%'> >()) {
      if      (lexed == "*") operators.push_back(Binary_Expression::MUL);
      else if (lexed == "/") operators.push_back(Binary_Expression::DIV);
      else                   operators.push_back(Binary_Expression::MOD);
      operands.push_back(parse_factor());
    }

    return fold_operands(fact1, operands, operators);
  }

  Expression* Parser::parse_factor()
  {
    if (lex< exactly<'('> >()) {
      Expression* value = parse_comma_list();
      if (!lex< exactly<')'> >()) throw_syntax_error("unclosed parenthesis");
      return value;
    }
    else if (lex< sequence< exactly<'+'>, negate< number > > >()) {
      return new (ctx.mem) Unary_Expression(path, line, Unary_Expression::PLUS, parse_factor());
    }
    else if (lex< sequence< exactly<'-'>, negate< number> > >()) {
      return new (ctx.mem) Unary_Expression(path, line, Unary_Expression::MINUS, parse_factor());
    }
    else {
      return parse_value();
    }
  }

  Expression* Parser::parse_value()
  {
    if (lex< uri_prefix >()) {
      Arguments* args = new (ctx.mem) Arguments(path, line);
      Function_Call* result = new (ctx.mem) Function_Call(path, line, "url", args);
      if (lex< variable >()) {
        Variable* var = new (ctx.mem) Variable(path, line, lexed);
        (*args) << new (ctx.mem) Argument(path, line, var);
      }
      else if (peek< string_constant >()) {
        String* s = parse_string();
        (*args) << new (ctx.mem) Argument(path, line, s);
      }
      else if (peek< sequence< url_schema, spaces_and_comments, exactly<')'> > >()) {
        lex< url_schema >();
        String_Schema* the_url = Parser::make_from_token(ctx, lexed, path, line).parse_url_schema();
        (*args) << new (ctx.mem) Argument(path, line, the_url);
      }
      else if (peek< sequence< url_value, spaces_and_comments, exactly<')'> > >()) {
        lex< url_value >();
        String_Constant* the_url = new (ctx.mem) String_Constant(path, line, lexed);
        (*args) << new (ctx.mem) Argument(path, line, the_url);
      }
      else {
        const char* value = position;
        const char* rparen = find_first< exactly<')'> >(position);
        if (!rparen) throw_syntax_error("URI is missing ')'");
        Token content_tok(Token(value, rparen));
        String_Constant* content_node = new (ctx.mem) String_Constant(path, line, content_tok);
        (*args) << new (ctx.mem) Argument(path, line, content_node);
        position = rparen;
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("URI is missing ')'");
      return result;
    }

    if (peek< functional >())
    { return parse_function_call(); }

    if (lex< value_schema >())
    { return Parser::make_from_token(ctx, lexed, path, line).parse_value_schema(); }

    if (lex< sequence< true_val, negate< identifier > > >())
    { return new (ctx.mem) Boolean(path, line, true); }

    if (lex< sequence< false_val, negate< identifier > > >())
    { return new (ctx.mem) Boolean(path, line, false); }

    if (lex< identifier >())
    { return new (ctx.mem) String_Constant(path, line, lexed); }

    if (lex< percentage >())
    { return new (ctx.mem) Textual(path, line, Textual::PERCENTAGE, lexed); }

    if (lex< dimension >())
    { return new (ctx.mem) Textual(path, line, Textual::DIMENSION, lexed); }

    if (lex< number >())
    { return new (ctx.mem) Textual(path, line, Textual::NUMBER, lexed); }

    if (lex< hex >())
    { return new (ctx.mem) Textual(path, line, Textual::HEX, lexed); }

    if (peek< string_constant >())
    { return parse_string(); }

    if (lex< variable >())
    { return new (ctx.mem) Variable(path, line, lexed); }

    throw_syntax_error("error reading values after " + lexed);

    // unreachable statement
    return 0;
  }

  String* Parser::parse_string()
  {
    lex< string_constant >();
    Token str(lexed);
    const char* i = str.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(str.begin, str.end);
    if (!p) {
      return new (ctx.mem) String_Constant(path, line, str);
    }

    String_Schema* schema = new (ctx.mem) String_Schema(path, line);
    while (i < str.end) {
      p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, str.end);
      if (p) {
        if (i < p) {
          (*schema) << new (ctx.mem) String_Constant(path, line, Token(i, p)); // accumulate the preceding segment if it's nonempty
        }
        const char* j = find_first_in_interval< exactly<rbrace> >(p, str.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          (*schema) << Parser::make_from_token(ctx, Token(p+2, j), path, line).parse_list();
          i = j+1;
        }
        else {
          // throw an error if the interpolant is unterminated
          throw_syntax_error("unterminated interpolant inside string constant " + str);
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < str.end) (*schema) << new (ctx.mem) String_Constant(path, line, Token(i, str.end));
        break;
      }
    }
    return schema;
  }

  String_Schema* Parser::parse_value_schema()
  {
    String_Schema* schema = new (ctx.mem) String_Schema(path, line);

    while (position < end) {
      if (lex< interpolant >()) {
        Token insides(Token(lexed.begin + 2, lexed.end - 1));
        Expression* interp_node = Parser::make_from_token(ctx, insides, path, line).parse_list();
        (*schema) << interp_node;
      }
      else if (lex< identifier >()) {
        (*schema) << new (ctx.mem) String_Constant(path, line, lexed);
      }
      else if (lex< percentage >()) {
        (*schema) << new (ctx.mem) Textual(path, line, Textual::PERCENTAGE, lexed);
      }
      else if (lex< dimension >()) {
        (*schema) << new (ctx.mem) Textual(path, line, Textual::DIMENSION, lexed);
      }
      else if (lex< number >()) {
        (*schema) << new (ctx.mem) Textual(path, line, Textual::NUMBER, lexed);
      }
      else if (lex< hex >()) {
        (*schema) << new (ctx.mem) Textual(path, line, Textual::HEX, lexed);
      }
      else if (lex< string_constant >()) {
        (*schema) << new (ctx.mem) String_Constant(path, line, lexed);
      }
      else if (lex< variable >()) {
        (*schema) << new (ctx.mem) Variable(path, line, lexed);
      }
      else {
        throw_syntax_error("error parsing interpolated value");
      }
    }
    return schema;
  }

  String_Schema* Parser::parse_url_schema()
  {
    String_Schema* schema = new (ctx.mem) String_Schema(path, line);

    while (position < end) {
      if (position[0] == '/') {
        lexed = Token(position, position+1);
        (*schema) << new (ctx.mem) String_Constant(path, line, lexed);
        ++position;
      }
      else if (lex< interpolant >()) {
        Token insides(Token(lexed.begin + 2, lexed.end - 1));
        (*schema) << Parser::make_from_token(ctx, insides, path, line).parse_list();
      }
      else if (lex< sequence< identifier, exactly<':'> > >()) {
        (*schema) << new (ctx.mem) String_Constant(path, line, lexed);
      }
      else if (lex< filename >()) {
        (*schema) << new (ctx.mem) String_Constant(path, line, lexed);
      }
      else {
        throw_syntax_error("error parsing interpolated url");
      }
    }
    return schema;
  }

  String* Parser::parse_identifier_schema()
  {
    lex< sequence< optional< exactly<'*'> >, identifier_schema > >();
    Token id(lexed);
    const char* i = id.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(id.begin, id.end);
    if (!p) {
      return new (ctx.mem) String_Constant(path, line, id);
    }

    String_Schema* schema = new (ctx.mem) String_Schema(path, line);
    while (i < id.end) {
      p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, id.end);
      if (p) {
        if (i < p) {
          (*schema) << new (ctx.mem) String_Constant(path, line, Token(i, p)); // accumulate the preceding segment if it's nonempty
        }
        const char* j = find_first_in_interval< exactly<rbrace> >(p, id.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          (*schema) << Parser::make_from_token(ctx, Token(p+2, j), path, line).parse_list();
          i = j+1;
        }
        else {
          // throw an error if the interpolant is unterminated
          throw_syntax_error("unterminated interpolant inside interpolated identifier " + id);
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < id.end) (*schema) << new (ctx.mem) String_Constant(path, line, Token(i, id.end));
        break;
      }
    }
    return schema;
  }

  Function_Call* Parser::parse_function_call()
  {
    lex< identifier >();
    string name(lexed);
    size_t line_of_call;

    Function_Call* the_call = new (ctx.mem) Function_Call(path, line_of_call, name, parse_arguments());
    return the_call;
  }

  If* Parser::parse_if_directive(bool else_if)
  {
    lex< if_directive >() || (else_if && lex< exactly<if_after_else_kwd> >());
    size_t if_line = line;
    Expression* predicate = parse_list();
    if (!peek< exactly<'{'> >()) throw_syntax_error("expected '{' after the predicate for @if");
    Block* consequent = parse_block();
    Block* alternative = 0;
    if (lex< else_directive >()) {
      if (peek< exactly<if_after_else_kwd> >()) {
        alternative = new (ctx.mem) Block(path, line);
        (*alternative) << parse_if_directive(true);
      }
      else if (!peek< exactly<'{'> >()) {
        throw_syntax_error("expected '{' after @else");
      }
      else {
        alternative = parse_block();
      }
    }
    return new (ctx.mem) If(path, if_line, predicate, consequent, alternative);
  }

  For* Parser::parse_for_directive()
  {
    lex< for_directive >();
    size_t for_line = line;
    if (!lex< variable >()) throw_syntax_error("@for directive requires an iteration variable");
    string var(lexed);
    if (!lex< from >()) throw_syntax_error("expected 'from' keyword in @for directive");
    Expression* lower_bound = parse_expression();
    bool inclusive;
    if (lex< through >()) inclusive = true;
    else if (lex< to >()) inclusive = false;
    else                  throw_syntax_error("expected 'through' or 'to' keywod in @for directive");
    Expression* upper_bound = parse_expression();
    if (!peek< exactly<'{'> >()) throw_syntax_error("expected '{' after the upper bound in @for directive");
    Block* body = parse_block();
    return new (ctx.mem) For(path, for_line, var, lower_bound, upper_bound, body, inclusive);
  }

  Each* Parser::parse_each_directive()
  {
    lex < each_directive >();
    size_t each_line = line;
    if (!lex< variable >()) throw_syntax_error("@each directive requires an iteration variable");
    string var(lexed);
    if (!lex< in >()) throw_syntax_error("expected 'in' keyword in @each directive");
    Expression* list = parse_list();
    if (!peek< exactly<'{'> >()) throw_syntax_error("expected '{' after the upper bound in @each directive");
    Block* body = parse_block();
    return new (ctx.mem) Each(path, each_line, var, list, body);
  }

  While* Parser::parse_while_directive()
  {
    lex< while_directive >();
    size_t while_line = line;
    Expression* predicate = parse_list();
    Block* body = parse_block();
    return new (ctx.mem) While(path, while_line, predicate, body);
  }

  At_Rule* Parser::parse_directive()
  {
    lex< directive >();
    string kwd(lexed);
    size_t dir_line = line;
    Block* block = (peek< exactly<'{'> >() ? parse_block() : 0);
    return new (ctx.mem) At_Rule(path, dir_line, kwd, 0, block);
  }

  Media_Block* Parser::parse_media_block()
  {
    lex< media >();
    size_t media_line = line;

    List* media_queries = parse_media_queries();

    if (!peek< exactly<'{'> >()) throw_syntax_error("expected '{' in media query");
    Block* block = parse_block();

    return new (ctx.mem) Media_Block(path, media_line, media_queries, block);
  }

  List* Parser::parse_media_queries()
  {
    List* media_queries = new (ctx.mem) List(path, line);
    if (!peek< exactly<'{'> >()) (*media_queries) << parse_media_query();
    while (lex< exactly<','> >()) (*media_queries) << parse_media_query();
    return media_queries;
  }

  List* Parser::parse_media_query()
  {
    List* media_query = new (ctx.mem) List(path, line);
    if (peek< identifier_schema >()) {
      (*media_query) << parse_identifier_schema();
      if (peek< identifier_schema >()) {
        (*media_query) << parse_identifier_schema();
      }
    }
    else {
      (*media_query) << parse_media_expression();
    }
    while (lex< exactly< and_kwd > >()) {
      (*media_query) << parse_media_expression();
    }
    return media_query;
  }

  Media_Query_Expression* Parser::parse_media_expression()
  {
    if (!lex< exactly<'('> >()) {
      throw_syntax_error("media query expression must begin with '('");
    }
    if (!peek< identifier_schema >()) {
      throw_syntax_error("media feature required in media query expression");
    }
    String* first = parse_identifier_schema();
    Expression* second = 0;
    if (lex< exactly<':'> >()) {
      second = parse_list();
    }
    if (!lex< exactly<')'> >()) {
      throw_syntax_error("unclosed parenthesis in media query expression");
    }
    return new (ctx.mem) Media_Query_Expression(path, first->line(), first, second);
  }

  // AST_Node* Parser::parse_keyframes(AST_Node*::Type inside_of)
  // {
  //   lex< keyframes >();
  //   AST_Node* keyframes(ctx.new_AST_Node*(AST_Node*::keyframes, path, line, 2));
  //   AST_Node* keyword(ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed));
  //   AST_Node* n(parse_expression());
  //   keyframes << keyword;
  //   keyframes << n;
  //   keyframes << parse_block(AST_Node*(), inside_of);
  //   return keyframes;
  // }

  // AST_Node* Parser::parse_keyframe(AST_Node*::Type inside_of) {
  //   AST_Node* keyframe(ctx.new_AST_Node*(AST_Node*::keyframe, path, line, 2));
  //   lex< keyf >();
  //   AST_Node* n = ctx.new_AST_Node*(AST_Node*::string_t, path, line, lexed);
  //   keyframe << n;
  //   if (peek< exactly<'{'> >()) {
  //     AST_Node* inner(parse_block(AST_Node*(), inside_of));
  //     keyframe << inner;
  //   }
  //   return keyframe;
  // }

  Warning* Parser::parse_warning()
  {
    lex< warn >();
    return new (ctx.mem) Warning(path, line, parse_list());
  }

  Selector_Lookahead Parser::lookahead_for_selector(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;
    bool saw_interpolant = false;

    while ((q = peek< identifier >(p))                             ||
           (q = peek< id_name >(p))                                ||
           (q = peek< class_name >(p))                             ||
           (q = peek< sequence< pseudo_prefix, identifier > >(p))  ||
           (q = peek< string_constant >(p))                        ||
           (q = peek< exactly<'*'> >(p))                           ||
           (q = peek< exactly<'('> >(p))                           ||
           (q = peek< exactly<')'> >(p))                           ||
           (q = peek< exactly<'['> >(p))                           ||
           (q = peek< exactly<']'> >(p))                           ||
           (q = peek< exactly<'+'> >(p))                           ||
           (q = peek< exactly<'~'> >(p))                           ||
           (q = peek< exactly<'>'> >(p))                           ||
           (q = peek< exactly<','> >(p))                           ||
           (q = peek< binomial >(p))                               ||
           (q = peek< sequence< optional<sign>,
                                optional<digits>,
                                exactly<'n'> > >(p))               ||
           (q = peek< sequence< optional<sign>,
                                digits > >(p))                     ||
           (q = peek< number >(p))                                 ||
           (q = peek< exactly<'&'> >(p))                           ||
           (q = peek< alternatives<exact_match,
                                   class_match,
                                   dash_match,
                                   prefix_match,
                                   suffix_match,
                                   substring_match> >(p))          ||
           (q = peek< sequence< exactly<'.'>, interpolant > >(p))  ||
           (q = peek< sequence< exactly<'#'>, interpolant > >(p))  ||
           (q = peek< sequence< exactly<'-'>, interpolant > >(p))  ||
           (q = peek< sequence< pseudo_prefix, interpolant > >(p)) ||
           (q = peek< interpolant >(p))) {
      p = q;
      if (*(p - 1) == '}') saw_interpolant = true;
    }

    Selector_Lookahead result;
    result.found            = peek< exactly<'{'> >(p) ? p : 0;
    result.has_interpolants = saw_interpolant;

    return result;
  }

  Selector_Lookahead Parser::lookahead_for_extension_target(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;
    bool saw_interpolant = false;

    while ((q = peek< identifier >(p))                             ||
           (q = peek< id_name >(p))                                ||
           (q = peek< class_name >(p))                             ||
           (q = peek< sequence< pseudo_prefix, identifier > >(p))  ||
           (q = peek< string_constant >(p))                        ||
           (q = peek< exactly<'*'> >(p))                           ||
           (q = peek< exactly<'('> >(p))                           ||
           (q = peek< exactly<')'> >(p))                           ||
           (q = peek< exactly<'['> >(p))                           ||
           (q = peek< exactly<']'> >(p))                           ||
           (q = peek< exactly<'+'> >(p))                           ||
           (q = peek< exactly<'~'> >(p))                           ||
           (q = peek< exactly<'>'> >(p))                           ||
           (q = peek< exactly<','> >(p))                           ||
           (q = peek< binomial >(p))                               ||
           (q = peek< sequence< optional<sign>,
                                optional<digits>,
                                exactly<'n'> > >(p))               ||
           (q = peek< sequence< optional<sign>,
                                digits > >(p))                     ||
           (q = peek< number >(p))                                 ||
           (q = peek< exactly<'&'> >(p))                           ||
           (q = peek< alternatives<exact_match,
                                   class_match,
                                   dash_match,
                                   prefix_match,
                                   suffix_match,
                                   substring_match> >(p))          ||
           (q = peek< sequence< exactly<'.'>, interpolant > >(p))  ||
           (q = peek< sequence< exactly<'#'>, interpolant > >(p))  ||
           (q = peek< sequence< exactly<'-'>, interpolant > >(p))  ||
           (q = peek< sequence< pseudo_prefix, interpolant > >(p)) ||
           (q = peek< interpolant >(p))) {
      p = q;
      if (*(p - 1) == '}') saw_interpolant = true;
    }

    Selector_Lookahead result;
    result.found            = peek< alternatives< exactly<';'>, exactly<'}'> > >(p) ? p : 0;
    result.has_interpolants = saw_interpolant;

    return result;
  }

  void Parser::read_bom()
  {
    size_t skip = 0;
    string encoding;
    bool utf_8 = false;
    switch ((unsigned char) source[0]) {
    case 0xEF:
      skip = check_bom_chars(source, utf_8_bom, 3);
      encoding = "UTF-8";
      utf_8 = true;
      break;
    case 0xFE:
      skip = check_bom_chars(source, utf_16_bom_be, 2);
      encoding = "UTF-16 (big endian)";
      break;
    case 0xFF:
      skip = check_bom_chars(source, utf_16_bom_le, 2);
      skip += (skip ? check_bom_chars(source, utf_32_bom_le, 4) : 0);
      encoding = (skip == 2 ? "UTF-16 (little endian)" : "UTF-32 (little endian)");
      break;
    case 0x00:
      skip = check_bom_chars(source, utf_32_bom_be, 4);
      encoding = "UTF-32 (big endian)";
      break;
    case 0x2B:
      skip = check_bom_chars(source, utf_7_bom_1, 4)
           | check_bom_chars(source, utf_7_bom_2, 4)
           | check_bom_chars(source, utf_7_bom_3, 4)
           | check_bom_chars(source, utf_7_bom_4, 4)
           | check_bom_chars(source, utf_7_bom_5, 5);
      encoding = "UTF-7";
      break;
    case 0xF7:
      skip = check_bom_chars(source, utf_1_bom, 3);
      encoding = "UTF-1";
      break;
    case 0xDD:
      skip = check_bom_chars(source, utf_ebcdic_bom, 4);
      encoding = "UTF-EBCDIC";
      break;
    case 0x0E:
      skip = check_bom_chars(source, scsu_bom, 3);
      encoding = "SCSU";
      break;
    case 0xFB:
      skip = check_bom_chars(source, bocu_1_bom, 3);
      encoding = "BOCU-1";
      break;
    case 0x84:
      skip = check_bom_chars(source, gb_18030_bom, 4);
      encoding = "GB-18030";
      break;
    }
    if (skip > 0 && !utf_8) throw_syntax_error("only UTF-8 documents are currently supported; your document appears to be " + encoding);
    position += skip;
  }

  size_t check_bom_chars(const char* src, const unsigned char* bom, size_t len)
  {
    size_t skip = 0;
    for (size_t i = 0; i < len; ++i, ++skip) {
      if ((unsigned char) src[i] != bom[i]) return 0;
    }
    return skip;
  }

}
