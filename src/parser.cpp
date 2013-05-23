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
        Flat_String* contents = new (ctx.mem) Flat_String(path, line, lexed);
        Comment*     comment  = new (ctx.mem) Comment(path, line, contents);
        (*root) << comment;
      }
      else if (peek< import >()) {
        Statement* import = parse_import();
        (*root) << import;
        // if (importee.type() == AST_Node*::css_import) (*root) << importee;
        // else                                     root += importee;
        // if (!lex< exactly<';'> >()) throw_syntax_error("top-level @import directive must be terminated by ';'");
      }
      else if (peek< mixin >() /* || peek< exactly<'='> >() */) {
        (*root) << parse_mixin_definition();
      }
      else if (peek< function >()) {
        (*root) << parse_function_definition();
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
        (*root) << parse_media_query();
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

  Statement* Parser::parse_import()
  {
    Statement* imp;
    lex< import >();
    if (peek< uri_prefix >()) {
      Function_Call* loc = parse_function_call();
      imp = new (ctx.mem) Import<Function_Call>(path, line, loc);
    }
    else if (peek< string_constant >()) {
      String* loc = parse_string();
      imp = new (ctx.mem) Import<String>(path, line, loc);
    }
    else {
      throw_syntax_error("@import directive requires a url or quoted path");
    }
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
    //   string base_path(Token::make(base_str, Prelexer::folders(base_str)).to_string());
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

  Definition<MIXIN>* Parser::parse_mixin_definition()
  {
    lex< mixin >() /* || lex< exactly<'='> >() */;
    if (!lex< identifier >()) throw_syntax_error("invalid name in @mixin directive");
    string name(lexed.begin, lexed.end);
    size_t mixin_line = line;
    Parameters* params = parse_parameters();
    if (!peek< exactly<'{'> >()) throw_syntax_error("body for mixin " + name + " must begin with a '{'");
    stack.push_back(mixin_def);
    Block* body = parse_block();
    stack.pop_back();
    Definition<MIXIN>* the_mixin = new (ctx.mem) Definition<MIXIN>(path, mixin_line, name, params, body);
    return the_mixin;
  }

  Definition<FUNCTION>* Parser::parse_function_definition()
  {
    lex< function >();
    size_t func_line = line;
    if (!lex< identifier >()) throw_syntax_error("name required for function definition");
    string name(lexed.begin, lexed.end);
    Parameters* params = parse_parameters();
    if (!peek< exactly<'{'> >()) throw_syntax_error("body for function " + name + " must begin with a '{'");
    stack.push_back(function_def);
    Block* body = parse_block();
    stack.pop_back();
    Definition<FUNCTION>* the_function = new (ctx.mem) Definition<FUNCTION>(path, func_line, name, params, body);
    return the_function;
  }

  Parameters* Parser::parse_parameters()
  {
    string name(lexed.begin, lexed.end); // for the error message
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
    string name(lexed.begin, lexed.end);
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
    string name(lexed.begin, lexed.end);
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
    string name(lexed.begin, lexed.end);
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
      string name(lexed.begin, lexed.end);
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
    string name(lexed.begin, lexed.end);
    size_t var_line = line;
    if (!lex< exactly<':'> >()) throw_syntax_error("expected ':' after " + lexed.to_string() + " in assignment statement");
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
      property_segment = new (ctx.mem) Flat_String(path, line, lexed);
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

  Interpolated_Selector* Parser::parse_selector_schema(const char* end_of_selector)
  {
    const char* i = position;
    const char* p;
    Interpolation* schema = new (ctx.mem) Interpolation(path, line);

    while (i < end_of_selector) {
      p = find_first_in_interval< exactly<hash_lbrace> >(i, end_of_selector);
      if (p) {
        // accumulate the preceding segment if there is one
        if (i < p) (*schema) << new (ctx.mem) Flat_String(path, line, Token(i, p));
        // find the end of the interpolant and parse it
        const char* j = find_first_in_interval< exactly<rbrace> >(p, end_of_selector);
        Expression* interp_node = Parser::make_from_token(ctx, Token(p+2, j), path, line).parse_list();
        (*schema) << interp_node;
        i = j + 1;
      }
      else { // no interpolants left; add the last segment if there is one
        if (i < end_of_selector) (*schema) << new (ctx.mem) Flat_String(path, line, Token(i, end_of_selector));
        break;
      }
    }
    position = end_of_selector;
    return new (ctx.mem) Interpolated_Selector(path, line, schema);
  }

  Selector_Group* Parser::parse_selector_group()
  {
    Selector_Group* group = new Selector_Group(path, line);
    do (*group) << parse_selector();
    while (lex< exactly<','> >());
    return group;
  }

  Selector_Combination* Parser::parse_selector()
  {
    AST_Node* seq1(parse_simple_selector_sequence());
    if (peek< exactly<','> >() ||
        peek< exactly<')'> >() ||
        peek< exactly<'{'> >() ||
        peek< exactly<';'> >()) return seq1;

    AST_Node* selector(ctx.new_AST_Node*(AST_Node*::selector, path, line, 2));
    selector << seq1;

    while (!peek< exactly<'{'> >() &&
           !peek< exactly<','> >() &&
           !peek< exactly<';'> >()) {
      selector << parse_simple_selector_sequence();
    }
    return selector;
  }

  Simple_Sequence* Parser::parse_simple_selector_sequence()
  {
    // check for initial and trailing combinators
    if (lex< exactly<'+'> >() ||
        lex< exactly<'~'> >() ||
        lex< exactly<'>'> >())
    { return ctx.new_AST_Node*(AST_Node*::selector_combinator, path, line, lexed); }

    // check for backref or type selector, which are only allowed at the front
    AST_Node* simp1;
    if (lex< exactly<'&'> >()) {
      simp1 = ctx.new_AST_Node*(AST_Node*::backref, path, line, lexed);
    }
    else if (lex< alternatives< type_selector, universal, string_constant, number > >()) {
      simp1 = ctx.new_AST_Node*(AST_Node*::simple_selector, path, line, lexed);
    }
    else {
      simp1 = parse_simple_selector();
    }

    // now we have one simple/atomic selector -- see if that's all
    if (peek< spaces >()       || peek< exactly<'>'> >() ||
        peek< exactly<'+'> >() || peek< exactly<'~'> >() ||
        peek< exactly<','> >() || peek< exactly<')'> >() ||
        peek< exactly<'{'> >() || peek< exactly<';'> >())
    { return simp1; }

    // otherwise, we have a sequence of simple selectors
    AST_Node* seq(ctx.new_AST_Node*(AST_Node*::simple_selector_sequence, path, line, 2));
    seq << simp1;

    while (!peek< spaces >(position) &&
           !(peek < exactly<'+'> >(position) ||
             peek < exactly<'~'> >(position) ||
             peek < exactly<'>'> >(position) ||
             peek < exactly<','> >(position) ||
             peek < exactly<')'> >(position) ||
             peek < exactly<'{'> >(position) ||
             peek < exactly<';'> >(position))) {
      seq << parse_simple_selector();
    }
    return seq;
  }

  AST_Node* Parser::parse_selector_combinator()
  {
    lex< exactly<'+'> >() || lex< exactly<'~'> >() ||
    lex< exactly<'>'> >() || lex< ancestor_of >();
    return ctx.new_AST_Node*(AST_Node*::selector_combinator, path, line, lexed);
  }

  AST_Node* Parser::parse_simple_selector()
  {
    if (lex< id_name >() || lex< class_name >() || lex< string_constant >() || lex< number >()) {
      return ctx.new_AST_Node*(AST_Node*::simple_selector, path, line, lexed);
    }
    else if (peek< exactly<':'> >(position)) {
      return parse_pseudo();
    }
    else if (peek< exactly<'['> >(position)) {
      return parse_attribute_selector();
    }
    else {
      throw_syntax_error("invalid selector after " + lexed.to_string());
    }
    // unreachable statement
    return AST_Node*();
  }

  AST_Node* Parser::parse_pseudo() {
    if (lex< pseudo_not >()) {
      AST_Node* ps_not(ctx.new_AST_Node*(AST_Node*::pseudo_negation, path, line, 2));
      ps_not << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
      ps_not << parse_selector_group();
      lex< exactly<')'> >();
      return ps_not;
    }
    else if (lex< sequence< pseudo_prefix, functional > >()) {
      AST_Node* pseudo(ctx.new_AST_Node*(AST_Node*::functional_pseudo, path, line, 2));
      Token name(lexed);
      pseudo << ctx.new_AST_Node*(AST_Node*::value, path, line, name);
      if (lex< alternatives< even, odd > >()) {
        pseudo << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
      }
      else if (peek< binomial >(position)) {
        if (peek< coefficient >()) {
          lex< coefficient >();
          pseudo << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
        }
        lex< exactly<'n'> >();
        pseudo << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
        lex< sign >();
        pseudo << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
        lex< digits >();
        pseudo << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
      }
      else if (lex< sequence< optional<sign>,
                              optional<digits>,
                              exactly<'n'> > >()) {
        pseudo << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
      }
      else if (lex< sequence< optional<sign>, digits > >()) {
        pseudo << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
      }
      else if (lex< identifier >()) {
        pseudo << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
      }
      else if (lex< string_constant >()) {
        pseudo << ctx.new_AST_Node*(AST_Node*::string_constant, path, line, lexed);
      }
      else {
        throw_syntax_error("invalid argument to " + name.to_string() + "...)");
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("unterminated argument to " + name.to_string() + "...)");
      return pseudo;
    }
    else if (lex < sequence< pseudo_prefix, identifier > >()) {
      return ctx.new_AST_Node*(AST_Node*::pseudo, path, line, lexed);
    }
    else {
      throw_syntax_error("unrecognized pseudo-class or pseudo-element");
    }
    // unreachable statement
    return AST_Node*();
  }

  AST_Node* Parser::parse_attribute_selector()
  {
    AST_Node* attr_sel(ctx.new_AST_Node*(AST_Node*::attribute_selector, path, line, 3));
    lex< exactly<'['> >();
    if (!lex< type_selector >()) throw_syntax_error("invalid attribute name in attribute selector");
    Token name(lexed);
    attr_sel << ctx.new_AST_Node*(AST_Node*::value, path, line, name);
    if (lex< exactly<']'> >()) return attr_sel;
    if (!lex< alternatives< exact_match, class_match, dash_match,
                            prefix_match, suffix_match, substring_match > >()) {
      throw_syntax_error("invalid operator in attribute selector for " + name.to_string());
    }
    attr_sel << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
    if (!lex< string_constant >() && !lex< identifier >()) throw_syntax_error("expected a string constant or identifier in attribute selector for " + name.to_string());
    attr_sel << ctx.new_AST_Node*(AST_Node*::value, path, line, lexed);
    if (!lex< exactly<']'> >()) throw_syntax_error("unterminated attribute selector for " + name.to_string());
    return attr_sel;
  }

  AST_Node* Parser::parse_block(AST_Node* surrounding_ruleset, AST_Node*::Type inside_of)
  {
    lex< exactly<'{'> >();
    bool semicolon = false;
    Selector_Lookahead lookahead_result;
    AST_Node* block(ctx.new_AST_Node*(AST_Node*::block, path, line, 0));
    while (!lex< exactly<'}'> >()) {
      if (semicolon) {
        if (!lex< exactly<';'> >()) throw_syntax_error("non-terminal statement or declaration must end with ';'");
        semicolon = false;
        while (lex< block_comment >()) {
          block << ctx.new_AST_Node*(AST_Node*::comment, path, line, lexed);
        }
        if (lex< exactly<'}'> >()) break;
      }
      if (lex< block_comment >()) {
        block << ctx.new_AST_Node*(AST_Node*::comment, path, line, lexed);
      }
      else if (peek< import >(position)) {
        if (inside_of == AST_Node*::mixin || inside_of == AST_Node*::function) {
          lex< import >(); // to adjust the line number
          throw_syntax_error("@import directive not allowed inside definition of mixin or function");
        }
        AST_Node* imported_tree(parse_import());
        if (imported_tree.type() == AST_Node*::css_import) {
          block << imported_tree;
        }
        else {
          for (size_t i = 0, S = imported_tree.size(); i < S; ++i) {
            block << imported_tree[i];
          }
          semicolon = true;
        }
      }
      else if (lex< variable >()) {
        block << parse_assignment();
        semicolon = true;
      }
      else if (peek< if_directive >()) {
        block << parse_if_directive(surrounding_ruleset, inside_of);
      }
      else if (peek< for_directive >()) {
        block << parse_for_directive(surrounding_ruleset, inside_of);
      }
      else if (peek< each_directive >()) {
        block << parse_each_directive(surrounding_ruleset, inside_of);
      }
      else if (peek < while_directive >()) {
        block << parse_while_directive(surrounding_ruleset, inside_of);
      }
      else if (lex < return_directive >()) {
        AST_Node* ret_expr(ctx.new_AST_Node*(AST_Node*::return_directive, path, line, 1));
        ret_expr << parse_list();
        ret_expr.should_eval() = true;
        block << ret_expr;
        semicolon = true;
      }
      else if (peek< warn >()) {
        block << parse_warning();
        semicolon = true;
      }
      else if (inside_of == AST_Node*::function) {
        throw_syntax_error("only variable declarations and control directives are allowed inside functions");
      }
      else if (peek< include >(position)) {
        AST_Node* the_call = parse_mixin_call(inside_of);
        block << the_call;
        // don't need a semicolon after a content block
        semicolon = (the_call.size() == 3) ? false : true;
      }
      else if (lex< content >()) {
        if (inside_of != AST_Node*::mixin) {
          throw_syntax_error("@content may only be used within a mixin");
        }
        block << ctx.new_AST_Node*(AST_Node*::mixin_content, path, line, 0); // just an expansion stub
        semicolon = true;
      }
      else if (peek< sequence< optional< exactly<'*'> >, alternatives< identifier_schema, identifier >, optional_spaces, exactly<':'>, optional_spaces, exactly<'{'> > >(position)) {
        block << parse_propset();
      }
      else if (peek < keyframes >()) {
        block << parse_keyframes(inside_of);
      }
      else if (peek< sequence< keyf, optional_spaces, exactly<'{'> > >()) {
        block << parse_keyframe(inside_of);
      }
      else if ((lookahead_result = lookahead_for_selector(position)).found) {
        block << parse_ruleset(lookahead_result, inside_of);
      }
      /*
      else if (peek< exactly<'+'> >()) {
        block << parse_mixin_call();
        semicolon = true;
      }
      */
      else if (lex< extend >()) {
        AST_Node* request(ctx.new_AST_Node*(AST_Node*::extend_directive, path, line, 1));
        Selector_Lookahead lookahead = lookahead_for_extension_target(position);

        if (!lookahead.found) throw_syntax_error("invalid selector for @extend");

        if (lookahead.has_interpolants) request << parse_selector_schema(lookahead.found);
        else                            request << parse_selector_group();

        semicolon = true;
        block << request;
      }
      else if (peek< media >()) {
        block << parse_media_query(inside_of);
      }
      // ignore the @charset directive for now
      else if (lex< exactly< charset_kwd > >()) {
        lex< string_constant >();
        lex< exactly<';'> >();
      }
      else if (peek< directive >()) {
        AST_Node* dir(parse_directive(surrounding_ruleset, inside_of));
        if (dir.type() == AST_Node*::blockless_directive) semicolon = true;
        block << dir;
      }
      else if (peek< percentage >() ){
        lex< percentage >();
        block << ctx.new_AST_Node*(path, line, atof(lexed.begin), AST_Node*::numeric_percentage);
        if (peek< exactly<'{'> >()) {
          AST_Node* inner(parse_block(AST_Node*()));
          block << inner;
        }
      }
      else if (!peek< exactly<';'> >()) {
        AST_Node* rule(parse_rule());
        // check for lbrace; if it's there, we have a namespace property with a value
        if (peek< exactly<'{'> >()) {
          AST_Node* inner(parse_block(AST_Node*()));
          AST_Node* propset(ctx.new_AST_Node*(AST_Node*::propset, path, line, 2));
          propset << rule[0];
          rule[0] = ctx.new_AST_Node*(AST_Node*::property, path, line, Token::make());
          inner.push_front(rule);
          propset << inner;
          block << propset;
        }
        else {
          block << rule;
          semicolon = true;
        }
      }
      else lex< exactly<';'> >();
      while (lex< block_comment >()) {
        block << ctx.new_AST_Node*(AST_Node*::comment, path, line, lexed);
      }
    }
    return block;
  }

  AST_Node* Parser::parse_rule() {
    AST_Node* rule(ctx.new_AST_Node*(AST_Node*::rule, path, line, 2));
    if (peek< sequence< optional< exactly<'*'> >, identifier_schema > >()) {
      rule << parse_identifier_schema();
    }
    else if (lex< sequence< optional< exactly<'*'> >, identifier > >()) {
      rule << ctx.new_AST_Node*(AST_Node*::property, path, line, lexed);
    }
    else {
      throw_syntax_error("invalid property name");
    }
    if (!lex< exactly<':'> >()) throw_syntax_error("property \"" + lexed.to_string() + "\" must be followed by a ':'");
    rule << parse_list();
    return rule;
  }

  AST_Node* Parser::parse_list()
  {
    return parse_comma_list();
  }

  AST_Node* Parser::parse_comma_list()
  {
    if (peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<'{'> >(position) ||
        peek< exactly<')'> >(position) ||
        peek< exactly<ellipsis> >(position))
    { return ctx.new_AST_Node*(AST_Node*::list, path, line, 0); }
    AST_Node* list1(parse_space_list());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< exactly<','> >(position)) return list1;

    AST_Node* comma_list(ctx.new_AST_Node*(AST_Node*::list, path, line, 2));
    comma_list.is_comma_separated() = true;
    comma_list << list1;
    comma_list.should_eval() |= list1.should_eval();

    while (lex< exactly<','> >())
    {
      AST_Node* list(parse_space_list());
      comma_list << list;
      comma_list.should_eval() |= list.should_eval();
    }

    return comma_list;
  }

  AST_Node* Parser::parse_space_list()
  {
    AST_Node* disj1(parse_disjunction());
    // if it's a singleton, return it directly; don't wrap it
    if (peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<'{'> >(position) ||
        peek< exactly<')'> >(position) ||
        peek< exactly<','> >(position) ||
        peek< exactly<ellipsis> >(position) ||
        peek< default_flag >(position))
    { return disj1; }

    AST_Node* space_list(ctx.new_AST_Node*(AST_Node*::list, path, line, 2));
    space_list << disj1;
    space_list.should_eval() |= disj1.should_eval();

    while (!(peek< exactly<';'> >(position) ||
             peek< exactly<'}'> >(position) ||
             peek< exactly<'{'> >(position) ||
             peek< exactly<')'> >(position) ||
             peek< exactly<','> >(position) ||
             peek< exactly<ellipsis> >(position) ||
             peek< default_flag >(position)))
    {
      AST_Node* disj(parse_disjunction());
      space_list << disj;
      space_list.should_eval() |= disj.should_eval();
    }

    return space_list;
  }

  AST_Node* Parser::parse_disjunction()
  {
    AST_Node* conj1(parse_conjunction());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< sequence< or_op, negate< identifier > > >()) return conj1;

    AST_Node* disjunction(ctx.new_AST_Node*(AST_Node*::disjunction, path, line, 2));
    disjunction << conj1;
    while (lex< sequence< or_op, negate< identifier > > >()) disjunction << parse_conjunction();
    disjunction.should_eval() = true;

    return disjunction;
  }

  AST_Node* Parser::parse_conjunction()
  {
    AST_Node* rel1(parse_relation());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< sequence< and_op, negate< identifier > > >()) return rel1;

    AST_Node* conjunction(ctx.new_AST_Node*(AST_Node*::conjunction, path, line, 2));
    conjunction << rel1;
    while (lex< sequence< and_op, negate< identifier > > >()) conjunction << parse_relation();
    conjunction.should_eval() = true;
    return conjunction;
  }

  AST_Node* Parser::parse_relation()
  {
    AST_Node* expr1(parse_expression());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< eq_op >(position)  ||
          peek< neq_op >(position) ||
          peek< gt_op >(position)  ||
          peek< gte_op >(position) ||
          peek< lt_op >(position)  ||
          peek< lte_op >(position)))
    { return expr1; }

    AST_Node* relation(ctx.new_AST_Node*(AST_Node*::relation, path, line, 3));
    expr1.should_eval() = true;
    relation << expr1;

    if (lex< eq_op >()) relation << ctx.new_AST_Node*(AST_Node*::eq, path, line, lexed);
    else if (lex< neq_op >()) relation << ctx.new_AST_Node*(AST_Node*::neq, path, line, lexed);
    else if (lex< gte_op >()) relation << ctx.new_AST_Node*(AST_Node*::gte, path, line, lexed);
    else if (lex< lte_op >()) relation << ctx.new_AST_Node*(AST_Node*::lte, path, line, lexed);
    else if (lex< gt_op >()) relation << ctx.new_AST_Node*(AST_Node*::gt, path, line, lexed);
    else if (lex< lt_op >()) relation << ctx.new_AST_Node*(AST_Node*::lt, path, line, lexed);

    AST_Node* expr2(parse_expression());
    expr2.should_eval() = true;
    relation << expr2;

    relation.should_eval() = true;
    return relation;
  }

  AST_Node* Parser::parse_expression()
  {
    AST_Node* term1(parse_term());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'+'> >(position) ||
          peek< sequence< negate< number >, exactly<'-'> > >(position)))
    { return term1; }

    AST_Node* expression(ctx.new_AST_Node*(AST_Node*::expression, path, line, 3));
    term1.should_eval() = true;
    expression << term1;

    while (lex< exactly<'+'> >() || lex< sequence< negate< number >, exactly<'-'> > >()) {
      if (lexed.begin[0] == '+') {
        expression << ctx.new_AST_Node*(AST_Node*::add, path, line, lexed);
      }
      else {
        expression << ctx.new_AST_Node*(AST_Node*::sub, path, line, lexed);
      }
      AST_Node* term(parse_term());
      term.should_eval() = true;
      expression << term;
    }
    expression.should_eval() = true;

    return expression;
  }

  AST_Node* Parser::parse_term()
  {
    AST_Node* fact1(parse_factor());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'*'> >(position) ||
          peek< exactly<'/'> >(position)))
    { return fact1; }

    AST_Node* term(ctx.new_AST_Node*(AST_Node*::term, path, line, 3));
    term << fact1;
    if (fact1.should_eval()) term.should_eval() = true;

    while (lex< exactly<'*'> >() || lex< exactly<'/'> >()) {
      if (lexed.begin[0] == '*') {
        term << ctx.new_AST_Node*(AST_Node*::mul, path, line, lexed);
        term.should_eval() = true;
      }
      else {
        term << ctx.new_AST_Node*(AST_Node*::div, path, line, lexed);
      }
      AST_Node* fact(parse_factor());
      term.should_eval() |= fact.should_eval();
      term << fact;
    }

    return term;
  }

  AST_Node* Parser::parse_factor()
  {
    if (lex< exactly<'('> >()) {
      AST_Node* value(parse_comma_list());
      value.should_eval() = true;
      if (value.type() == AST_Node*::list && value.size() > 0) {
        value[0].should_eval() = true;
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("unclosed parenthesis");
      return value;
    }
    else if (lex< sequence< exactly<'+'>, negate< number > > >()) {
      AST_Node* plus(ctx.new_AST_Node*(AST_Node*::unary_plus, path, line, 1));
      plus << parse_factor();
      plus.should_eval() = true;
      return plus;
    }
    else if (lex< sequence< exactly<'-'>, negate< number> > >()) {
      AST_Node* minus(ctx.new_AST_Node*(AST_Node*::unary_minus, path, line, 1));
      minus << parse_factor();
      minus.should_eval() = true;
      return minus;
    }
    else {
      return parse_value();
    }
  }

  AST_Node* Parser::parse_value()
  {
    if (lex< uri_prefix >())
    {
      AST_Node* result(ctx.new_AST_Node*(AST_Node*::uri, path, line, 1));
      if (lex< variable >()) {
        result << ctx.new_AST_Node*(AST_Node*::variable, path, line, lexed);
        result.should_eval() = true;
      }
      else if (lex< string_constant >()) {
        result << parse_string();
        result.should_eval() = true;
      }
      else if (peek< sequence< url_schema, spaces_and_comments, exactly<')'> > >()) {
        lex< url_schema >();
        result << Parser::make_from_token(ctx, lexed, path, line).parse_url_schema();
        result.should_eval() = true;
      }
      else if (peek< sequence< url_value, spaces_and_comments, exactly<')'> > >()) {
        lex< url_value >();
        result << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
      }
      else {
        const char* value = position;
        const char* rparen = find_first< exactly<')'> >(position);
        if (!rparen) throw_syntax_error("URI is missing ')'");
        Token content_tok(Token::make(value, rparen));
        AST_Node* content_node(ctx.new_AST_Node*(AST_Node*::identifier, path, line, content_tok));
        // lex< string_constant >();
        result << content_node;
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
    { return ctx.new_AST_Node*(AST_Node*::boolean, path, line, true); }

    if (lex< sequence< false_val, negate< identifier > > >())
    { return ctx.new_AST_Node*(AST_Node*::boolean, path, line, false); }

    if (lex< important >())
    { return ctx.new_AST_Node*(AST_Node*::important, path, line, lexed); }

    if (lex< identifier >())
    { return ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed); }

    if (lex< percentage >())
    { return ctx.new_AST_Node*(AST_Node*::textual_percentage, path, line, lexed); }

    if (lex< dimension >())
    { return ctx.new_AST_Node*(AST_Node*::textual_dimension, path, line, lexed); }

    if (lex< number >())
    { return ctx.new_AST_Node*(AST_Node*::textual_number, path, line, lexed); }

    if (lex< hex >())
    { return ctx.new_AST_Node*(AST_Node*::textual_hex, path, line, lexed); }

    // if (lex< percentage >())
    // { return ctx.new_AST_Node*(path, line, atof(lexed.begin), AST_Node*::numeric_percentage); }

    // if (lex< dimension >()) {
    //   return ctx.new_AST_Node*(path, line, atof(lexed.begin),
    //                           Token::make(Prelexer::number(lexed.begin), lexed.end));
    // }

    // if (lex< number >())
    // { return ctx.new_AST_Node*(path, line, atof(lexed.begin)); }

    // if (lex< hex >()) {
    //   AST_Node* triple(ctx.new_AST_Node*(AST_Node*::numeric_color, path, line, 4));
    //   Token hext(Token::make(lexed.begin+1, lexed.end));
    //   if (hext.length() == 6) {
    //     for (int i = 0; i < 6; i += 2) {
    //       triple << ctx.new_AST_Node*(path, line, static_cast<double>(strtol(string(hext.begin+i, 2).c_str(), NULL, 16)));
    //     }
    //   }
    //   else {
    //     for (int i = 0; i < 3; ++i) {
    //       triple << ctx.new_AST_Node*(path, line, static_cast<double>(strtol(string(2, hext.begin[i]).c_str(), NULL, 16)));
    //     }
    //   }
    //   triple << ctx.new_AST_Node*(path, line, 1.0);
    //   return triple;
    // }

    if (peek< string_constant >())
    { return parse_string(); }

    if (lex< variable >())
    {
      AST_Node* var(ctx.new_AST_Node*(AST_Node*::variable, path, line, lexed));
      var.should_eval() = true;
      return var;
    }

    throw_syntax_error("error reading values after " + lexed.to_string());

    // unreachable statement
    return AST_Node*();
  }

  String* Parser::parse_string()
  {
    lex< string_constant >();
    Token str(lexed);
    const char* i = str.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(str.begin, str.end);
    if (!p) {
      AST_Node* result(ctx.new_AST_Node*(AST_Node*::string_constant, path, line, str));
      result.is_quoted() = true;
      return result;
    }

    AST_Node* schema(ctx.new_AST_Node*(AST_Node*::string_schema, path, line, 1));
    while (i < str.end) {
      p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, str.end);
      if (p) {
        if (i < p) {
          schema << ctx.new_AST_Node*(AST_Node*::identifier, path, line, Token::make(i, p)); // accumulate the preceding segment if it's nonempty
        }
        const char* j = find_first_in_interval< exactly<rbrace> >(p, str.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          AST_Node* interp_node(Parser::make_from_token(ctx, Token::make(p+2, j), path, line).parse_list());
          interp_node.should_eval() = true;
          schema << interp_node;
          i = j+1;
        }
        else {
          // throw an error if the interpolant is unterminated
          throw_syntax_error("unterminated interpolant inside string constant " + str.to_string());
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < str.end) schema << ctx.new_AST_Node*(AST_Node*::identifier, path, line, Token::make(i, str.end));
        break;
      }
    }
    schema.is_quoted() = true;
    schema.should_eval() = true;
    return schema;
  }

  AST_Node* Parser::parse_value_schema()
  {
    AST_Node* schema(ctx.new_AST_Node*(AST_Node*::value_schema, path, line, 1));

    while (position < end) {
      if (lex< interpolant >()) {
        Token insides(Token::make(lexed.begin + 2, lexed.end - 1));
        AST_Node* interp_node(Parser::make_from_token(ctx, insides, path, line).parse_list());
        interp_node.should_eval() = true;
        schema << interp_node;
      }
      else if (lex< identifier >()) {
        schema << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
      }
      else if (lex< percentage >()) {
        schema << ctx.new_AST_Node*(AST_Node*::textual_percentage, path, line, lexed);
        // schema << ctx.new_AST_Node*(path, line, atof(lexed.begin), AST_Node*::numeric_percentage);
      }
      else if (lex< dimension >()) {
        schema << ctx.new_AST_Node*(AST_Node*::textual_dimension, path, line, lexed);
        // schema << ctx.new_AST_Node*(path, line, atof(lexed.begin),
        //                            Token::make(Prelexer::number(lexed.begin), lexed.end));
      }
      else if (lex< number >()) {
        schema << ctx.new_AST_Node*(AST_Node*::textual_number, path, line, lexed);
        // schema << ctx.new_AST_Node*(path, line, atof(lexed.begin));
      }
      else if (lex< hex >()) {
        schema << ctx.new_AST_Node*(AST_Node*::textual_hex, path, line, lexed);
        // AST_Node* triple(ctx.new_AST_Node*(AST_Node*::numeric_color, path, line, 4));
        // Token hext(Token::make(lexed.begin+1, lexed.end));
        // if (hext.length() == 6) {
        //   for (int i = 0; i < 6; i += 2) {
        //     triple << ctx.new_AST_Node*(path, line, static_cast<double>(strtol(string(hext.begin+i, 2).c_str(), NULL, 16)));
        //   }
        // }
        // else {
        //   for (int i = 0; i < 3; ++i) {
        //     triple << ctx.new_AST_Node*(path, line, static_cast<double>(strtol(string(2, hext.begin[i]).c_str(), NULL, 16)));
        //   }
        // }
        // triple << ctx.new_AST_Node*(path, line, 1.0);
        // schema << triple;
      }
      else if (lex< string_constant >()) {
        AST_Node* str(ctx.new_AST_Node*(AST_Node*::string_constant, path, line, lexed));
        str.is_quoted() = true;
        schema << str;
      }
      else if (lex< variable >()) {
        schema << ctx.new_AST_Node*(AST_Node*::variable, path, line, lexed);
      }
      else {
        throw_syntax_error("error parsing interpolated value");
      }
    }
    schema.should_eval() = true;
    return schema;
  }

  AST_Node* Parser::parse_url_schema()
  {
    AST_Node* schema(ctx.new_AST_Node*(AST_Node*::value_schema, path, line, 1));

    while (position < end) {
      if (position[0] == '/') {
        lexed = Token::make(position, position+1);
        schema << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
        ++position;
      }
      else if (lex< interpolant >()) {
        Token insides(Token::make(lexed.begin + 2, lexed.end - 1));
        AST_Node* interp_node(Parser::make_from_token(ctx, insides, path, line).parse_list());
        interp_node.should_eval() = true;
        schema << interp_node;
      }
      else if (lex< sequence< identifier, exactly<':'> > >()) {
        schema << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
      }
      else if (lex< filename >()) {
        schema << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
      }
      else {
        throw_syntax_error("error parsing interpolated url");
      }
    }
    schema.should_eval() = true;
    return schema;
  }

  AST_Node* Parser::parse_identifier_schema()
  {
    lex< sequence< optional< exactly<'*'> >, identifier_schema > >();
    Token id(lexed);
    const char* i = id.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(id.begin, id.end);
    if (!p) {
      return ctx.new_AST_Node*(AST_Node*::string_constant, path, line, id);
    }

    AST_Node* schema(ctx.new_AST_Node*(AST_Node*::identifier_schema, path, line, 1));
    while (i < id.end) {
      p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, id.end);
      if (p) {
        if (i < p) {
          schema << ctx.new_AST_Node*(AST_Node*::identifier, path, line, Token::make(i, p)); // accumulate the preceding segment if it's nonempty
        }
        const char* j = find_first_in_interval< exactly<rbrace> >(p, id.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          AST_Node* interp_node(Parser::make_from_token(ctx, Token::make(p+2, j), path, line).parse_list());
          interp_node.should_eval() = true;
          schema << interp_node;
          i = j+1;
        }
        else {
          // throw an error if the interpolant is unterminated
          throw_syntax_error("unterminated interpolant inside interpolated identifier " + id.to_string());
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < id.end) schema << ctx.new_AST_Node*(AST_Node*::identifier, path, line, Token::make(i, id.end));
        break;
      }
    }
    schema.should_eval() = true;
    return schema;
  }

  Function_Call* Parser::parse_function_call()
  {
    AST_Node* name;
    if (lex< identifier_schema >()) {
      name = parse_identifier_schema();
    }
    else {
      lex< identifier >();
      name = ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
    }

    AST_Node* args(parse_arguments());
    AST_Node* call(ctx.new_AST_Node*(AST_Node*::function_call, name.path(), name.line(), 2));
    call << name << args;
    call.should_eval() = true;
    return call;
  }

  If* Parser::parse_if_directive(AST_Node* surrounding_ruleset, AST_Node*::Type inside_of)
  {
    lex< if_directive >();
    AST_Node* conditional(ctx.new_AST_Node*(AST_Node*::if_directive, path, line, 2));
    conditional << parse_list(); // the predicate
    if (!lex< exactly<'{'> >()) throw_syntax_error("expected '{' after the predicate for @if");
    conditional << parse_block(surrounding_ruleset, inside_of); // the consequent
    // collect all "@else if"s
    while (lex< elseif_directive >()) {
      conditional << parse_list(); // the next predicate
      if (!lex< exactly<'{'> >()) throw_syntax_error("expected '{' after the predicate for @else if");
      conditional << parse_block(surrounding_ruleset, inside_of); // the next consequent
    }
    // parse the "@else" if present
    if (lex< else_directive >()) {
      if (!lex< exactly<'{'> >()) throw_syntax_error("expected '{' after @else");
      conditional << parse_block(surrounding_ruleset, inside_of); // the alternative
    }
    return conditional;
  }

  For* Parser::parse_for_directive(AST_Node* surrounding_ruleset, AST_Node*::Type inside_of)
  {
    lex< for_directive >();
    size_t for_line = line;
    if (!lex< variable >()) throw_syntax_error("@for directive requires an iteration variable");
    AST_Node* var(ctx.new_AST_Node*(AST_Node*::variable, path, line, lexed));
    if (!lex< from >()) throw_syntax_error("expected 'from' keyword in @for directive");
    AST_Node* lower_bound(parse_expression());
    AST_Node*::Type for_type = AST_Node*::for_through_directive;
    if (lex< through >()) for_type = AST_Node*::for_through_directive;
    else if (lex< to >()) for_type = AST_Node*::for_to_directive;
    else                  throw_syntax_error("expected 'through' or 'to' keywod in @for directive");
    AST_Node* upper_bound(parse_expression());
    if (!peek< exactly<'{'> >()) throw_syntax_error("expected '{' after the upper bound in @for directive");
    AST_Node* body(parse_block(surrounding_ruleset, inside_of));
    AST_Node* loop(ctx.new_AST_Node*(for_type, path, for_line, 4));
    loop << var << lower_bound << upper_bound << body;
    return loop;
  }

  Each* Parser::parse_each_directive(AST_Node* surrounding_ruleset, AST_Node*::Type inside_of)
  {
    lex < each_directive >();
    size_t each_line = line;
    if (!lex< variable >()) throw_syntax_error("@each directive requires an iteration variable");
    AST_Node* var(ctx.new_AST_Node*(AST_Node*::variable, path, line, lexed));
    if (!lex< in >()) throw_syntax_error("expected 'in' keyword in @each directive");
    AST_Node* list(parse_list());
    if (!peek< exactly<'{'> >()) throw_syntax_error("expected '{' after the upper bound in @each directive");
    AST_Node* body(parse_block(surrounding_ruleset, inside_of));
    AST_Node* each(ctx.new_AST_Node*(AST_Node*::each_directive, path, each_line, 3));
    each << var << list << body;
    return each;
  }

  While* Parser::parse_while_directive(AST_Node* surrounding_ruleset, AST_Node*::Type inside_of)
  {
    lex< while_directive >();
    size_t while_line = line;
    AST_Node* predicate(parse_list());
    AST_Node* body(parse_block(surrounding_ruleset, inside_of));
    AST_Node* loop(ctx.new_AST_Node*(AST_Node*::while_directive, path, while_line, 2));
    loop << predicate << body;
    return loop;
  }

  At_Rule* Parser::parse_directive(AST_Node* surrounding_ruleset, AST_Node*::Type inside_of)
  {
    lex< directive >();
    AST_Node* dir_name(ctx.new_AST_Node*(AST_Node*::blockless_directive, path, line, lexed));
    if (!peek< exactly<'{'> >()) return dir_name;
    AST_Node* block(parse_block(surrounding_ruleset, inside_of));
    AST_Node* dir(ctx.new_AST_Node*(AST_Node*::block_directive, path, line, 2));
    dir << dir_name << block;
    return dir;
  }

  Media_Query* Parser::parse_media_query(AST_Node*::Type inside_of)
  {
    lex< media >();
    AST_Node* media_query(ctx.new_AST_Node*(AST_Node*::media_query, path, line, 2));
    AST_Node* media_expr(parse_media_expression());
    if (peek< exactly<'{'> >()) {
      media_query << media_expr;
    }
    else if (peek< exactly<','> >()) {
      AST_Node* media_expr_group(ctx.new_AST_Node*(AST_Node*::media_expression_group, path, line, 2));
      media_expr_group << media_expr;
      while (lex< exactly<','> >()) {
        media_expr_group << parse_media_expression();
      }
      media_query << media_expr_group;
    }
    else {
      throw_syntax_error("expected '{' in media query");
    }
    media_query << parse_block(AST_Node*(), inside_of);
    return media_query;
  }

  Media_Expression* Parser::parse_media_expression()
  {
    AST_Node* media_expr(ctx.new_AST_Node*(AST_Node*::media_expression, path, line, 1));
    // if the query begins with 'not' or 'only', then a media type is required
    if (lex< not_op >() || lex< exactly<only_kwd> >()) {
      media_expr << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
      if (!lex< identifier >()) throw_syntax_error("media type expected in media query");
      media_expr << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
    }
    // otherwise, the media type is optional
    else if (lex< identifier >()) {
      media_expr << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
    }
    // if no media type was present, then require a parenthesized property
    if (media_expr.empty()) {
      if (!lex< exactly<'('> >()) throw_syntax_error("invalid media query");
      media_expr << parse_rule();
      if (!lex< exactly<')'> >()) throw_syntax_error("unclosed parenthesis");
    }
    // parse the rest of the properties for this disjunct
    while (!peek< exactly<','> >() && !peek< exactly<'{'> >()) {
      if (!lex< and_op >()) throw_syntax_error("invalid media query");
      media_expr << ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed);
      if (!lex< exactly<'('> >()) throw_syntax_error("invalid media query");
      media_expr << parse_rule();
      if (!lex< exactly<')'> >()) throw_syntax_error("unclosed parenthesis");
    }
    return media_expr;
  }

  AST_Node* Parser::parse_keyframes(AST_Node*::Type inside_of)
  {
    lex< keyframes >();
    AST_Node* keyframes(ctx.new_AST_Node*(AST_Node*::keyframes, path, line, 2));
    AST_Node* keyword(ctx.new_AST_Node*(AST_Node*::identifier, path, line, lexed));
    AST_Node* n(parse_expression());
    keyframes << keyword;
    keyframes << n;
    keyframes << parse_block(AST_Node*(), inside_of);
    return keyframes;
  }

  AST_Node* Parser::parse_keyframe(AST_Node*::Type inside_of) {
    AST_Node* keyframe(ctx.new_AST_Node*(AST_Node*::keyframe, path, line, 2));
    lex< keyf >();
    AST_Node* n = ctx.new_AST_Node*(AST_Node*::string_t, path, line, lexed);
    keyframe << n;
    if (peek< exactly<'{'> >()) {
      AST_Node* inner(parse_block(AST_Node*(), inside_of));
      keyframe << inner;
    }
    return keyframe;
  }

  Warning* Parser::parse_warning()
  {
    lex< warn >();
    AST_Node* warning(ctx.new_AST_Node*(AST_Node*::warning, path, line, 1));
    warning << parse_list();
    warning[0].should_eval() = true;
    return warning;
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
