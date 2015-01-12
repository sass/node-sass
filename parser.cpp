#include <cstdlib>
#include <iostream>
#include <vector>
#include "parser.hpp"
#include "file.hpp"
#include "inspect.hpp"
#include "to_string.hpp"
#include "constants.hpp"
#include "util.hpp"
#include "prelexer.hpp"
#include "sass_functions.h"

#include <typeinfo>

namespace Sass {
  using namespace std;
  using namespace Constants;

  Parser Parser::from_c_str(const char* str, Context& ctx, ParserState pstate)
  {
    Parser p(ctx, pstate);
    p.source   = str;
    p.position = p.source;
    p.end      = str + strlen(str);
    return p;
  }

  Parser Parser::from_token(Token t, Context& ctx, ParserState pstate)
  {
    Parser p(ctx, pstate);
    p.source   = t.begin;
    p.position = p.source;
    p.end      = t.end;
    p.dequote  = true;
    return p;
  }

  Block* Parser::parse()
  {
    Block* root = new (ctx.mem) Block(pstate);
    root->is_root(true);
    read_bom();
    lex< optional_spaces >();
    Selector_Lookahead lookahead_result;
    while (position < end) {
      if (lex< block_comment >()) {
        String*  contents = parse_interpolated_chunk(lexed);
        Comment* comment  = new (ctx.mem) Comment(pstate, contents);
        (*root) << comment;
      }
      else if (peek< import >()) {
        Import* imp = parse_import();
        if (!imp->urls().empty()) (*root) << imp;
        if (!imp->files().empty()) {
          for (size_t i = 0, S = imp->files().size(); i < S; ++i) {
            (*root) << new (ctx.mem) Import_Stub(pstate, imp->files()[i]);
          }
        }
        if (!lex< one_plus< exactly<';'> > >()) error("top-level @import directive must be terminated by ';'", pstate);
      }
      else if (peek< mixin >() || peek< function >()) {
        (*root) << parse_definition();
      }
      else if (peek< variable >()) {
        (*root) << parse_assignment();
        if (!lex< one_plus< exactly<';'> > >()) error("top-level variable binding must be terminated by ';'", pstate);
      }
      else if (peek< sequence< optional< exactly<'*'> >, alternatives< identifier_schema, identifier >, optional_spaces, exactly<':'>, optional_spaces, exactly<'{'> > >(position)) {
        (*root) << parse_propset();
      }
      else if (peek< include >() /* || peek< exactly<'+'> >() */) {
        Mixin_Call* mixin_call = parse_mixin_call();
        (*root) << mixin_call;
        if (!mixin_call->block() && !lex< one_plus< exactly<';'> > >()) error("top-level @include directive must be terminated by ';'", pstate);
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
      else if (peek< at_root >()) {
        (*root) << parse_at_root_block();
      }
      else if (peek< supports >()) {
        (*root) << parse_feature_block();
      }
      else if (peek< warn >()) {
        (*root) << parse_warning();
        if (!lex< one_plus< exactly<';'> > >()) error("top-level @warn directive must be terminated by ';'", pstate);
      }
      else if (peek< err >()) {
        (*root) << parse_error();
        if (!lex< one_plus< exactly<';'> > >()) error("top-level @error directive must be terminated by ';'", pstate);
      }
      else if (peek< dbg >()) {
        (*root) << parse_debug();
        if (!lex< one_plus< exactly<';'> > >()) error("top-level @debug directive must be terminated by ';'", pstate);
      }
      // ignore the @charset directive for now
      else if (lex< exactly< charset_kwd > >()) {
        lex< string_constant >();
        lex< one_plus< exactly<';'> > >();
      }
      else if (peek< at_keyword >()) {
        At_Rule* at_rule = parse_at_rule();
        (*root) << at_rule;
        if (!at_rule->block() && !lex< one_plus< exactly<';'> > >()) error("top-level directive must be terminated by ';'", pstate);
      }
      else if ((lookahead_result = lookahead_for_selector(position)).found) {
        (*root) << parse_ruleset(lookahead_result);
      }
      else if (peek< exactly<';'> >()) {
        lex< one_plus< exactly<';'> > >();
      }
      else {
        lex< spaces_and_comments >();
        if (position >= end) break;
        error("invalid top-level expression", pstate);
      }
      lex< optional_spaces >();
    }
    return root;
  }

  void Parser::add_single_file (Import* imp, string import_path) {

    string extension;
    string unquoted(unquote(import_path));
    if (unquoted.length() > 4) { // 2 quote marks + the 4 chars in .css
      // a string constant is guaranteed to end with a quote mark, so make sure to skip it when indexing from the end
      extension = unquoted.substr(unquoted.length() - 4, 4);
    }

    if (extension == ".css") {
      String_Constant* loc = new (ctx.mem) String_Constant(pstate, import_path, true);
      Argument* loc_arg = new (ctx.mem) Argument(pstate, loc);
      Arguments* loc_args = new (ctx.mem) Arguments(pstate);
      (*loc_args) << loc_arg;
      Function_Call* new_url = new (ctx.mem) Function_Call(pstate, "url", loc_args);
      imp->urls().push_back(new_url);
    }
    else {
      string current_dir = File::dir_name(path);
      string resolved(ctx.add_file(current_dir, unquoted));
      if (resolved.empty()) error("file to import not found or unreadable: " + unquoted + "\nCurrent dir: " + current_dir, pstate);
      imp->files().push_back(resolved);
    }

  }

  Import* Parser::parse_import()
  {
    lex< import >();
    Import* imp = new (ctx.mem) Import(pstate);
    bool first = true;
    do {
      if (lex< string_constant >()) {
        string import_path(lexed);

        // struct Sass_Options opt = sass_context_get_options(ctx)
        Sass_C_Import_Callback importer = ctx.importer;
        // custom importer
        if (importer) {
          Sass_Import* current = ctx.import_stack.back();
          Sass_C_Import_Fn fn = sass_import_get_function(importer);
          void* cookie = sass_import_get_cookie(importer);
          // create a new import entry
          string inc_path = unquote(import_path);
          struct Sass_Import** includes = fn(
            inc_path.c_str(),
            sass_import_get_path(current),
            cookie);
          if (includes) {
            struct Sass_Import** list = includes;
            while (*includes) {
              struct Sass_Import* include = *includes;
              const char *file = sass_import_get_path(include);
              char *source = sass_import_take_source(include);
              // char *srcmap = sass_import_take_srcmap(include);
              if (source) {
                if (file) {
                  ctx.add_source(file, inc_path, source);
                  imp->files().push_back(file);
                } else {
                  ctx.add_source(inc_path, inc_path, source);
                  imp->files().push_back(inc_path);
                }
              } else if(file) {
                add_single_file(imp, file);
              }
              ++includes;
            }
            // deallocate returned memory
            sass_delete_import_list(list);
            // parse next import
            continue;
          }
        }

        if (!unquote(import_path).substr(0, 7).compare("http://") ||
            !unquote(import_path).substr(0, 8).compare("https://") ||
            !unquote(import_path).substr(0, 2).compare("//"))
        {
          imp->urls().push_back(new (ctx.mem) String_Constant(pstate, import_path));
        }
        else {
          add_single_file(imp, import_path);
        }

      }
      else if (peek< uri_prefix >()) {
        imp->urls().push_back(parse_value());
      }
      else {
        if (first) error("@import directive requires a url or quoted path", pstate);
        else error("expecting another url or quoted path in @import list", pstate);
      }
      first = false;
    } while (lex< exactly<','> >());
    return imp;
  }

  Definition* Parser::parse_definition()
  {
    Definition::Type which_type = Definition::MIXIN;
    if      (lex< mixin >())    which_type = Definition::MIXIN;
    else if (lex< function >()) which_type = Definition::FUNCTION;
    string which_str(lexed);
    if (!lex< identifier >()) error("invalid name in " + which_str + " definition", pstate);
    string name(Util::normalize_underscores(lexed));
    if (which_type == Definition::FUNCTION && (name == "and" || name == "or" || name == "not"))
    { error("Invalid function name \"" + name + "\".", pstate); }
    ParserState source_position_of_def = pstate;
    Parameters* params = parse_parameters();
    if (!peek< exactly<'{'> >()) error("body for " + which_str + " " + name + " must begin with a '{'", pstate);
    if (which_type == Definition::MIXIN) stack.push_back(mixin_def);
    else stack.push_back(function_def);
    Block* body = parse_block();
    stack.pop_back();
    Definition* def = new (ctx.mem) Definition(source_position_of_def, name, params, body, which_type);
    return def;
  }

  Parameters* Parser::parse_parameters()
  {
    string name(lexed); // for the error message
    Parameters* params = new (ctx.mem) Parameters(pstate);
    if (lex< exactly<'('> >()) {
      // if there's anything there at all
      if (!peek< exactly<')'> >()) {
        do (*params) << parse_parameter();
        while (lex< exactly<','> >());
      }
      if (!lex< exactly<')'> >()) error("expected a variable name (e.g. $x) or ')' for the parameter list for " + name, pstate);
    }
    return params;
  }

  Parameter* Parser::parse_parameter()
  {
    lex< variable >();
    string name(Util::normalize_underscores(lexed));
    ParserState pos = pstate;
    Expression* val = 0;
    bool is_rest = false;
    if (lex< exactly<':'> >()) { // there's a default value
      val = parse_space_list();
      val->is_delayed(false);
    }
    else if (lex< exactly< ellipsis > >()) {
      is_rest = true;
    }
    Parameter* p = new (ctx.mem) Parameter(pos, name, val, is_rest);
    return p;
  }

  Mixin_Call* Parser::parse_mixin_call()
  {
    lex< include >() /* || lex< exactly<'+'> >() */;
    if (!lex< identifier >()) error("invalid name in @include directive", pstate);
    ParserState source_position_of_call = pstate;
    string name(Util::normalize_underscores(lexed));
    Arguments* args = parse_arguments();
    Block* content = 0;
    if (peek< exactly<'{'> >()) {
      content = parse_block();
    }
    Mixin_Call* the_call = new (ctx.mem) Mixin_Call(source_position_of_call, name, args, content);
    return the_call;
  }

  Arguments* Parser::parse_arguments()
  {
    string name(lexed);
    Arguments* args = new (ctx.mem) Arguments(pstate);

    if (lex< exactly<'('> >()) {
      // if there's anything there at all
      if (!peek< exactly<')'> >()) {
        do (*args) << parse_argument();
        while (lex< exactly<','> >());
      }
      if (!lex< exactly<')'> >()) error("expected a variable name (e.g. $x) or ')' for the parameter list for " + name, pstate);
    }

    return args;
  }

  Argument* Parser::parse_argument()
  {
    Argument* arg;
    if (peek< sequence < variable, spaces_and_comments, exactly<':'> > >()) {
      lex< variable >();
      string name(Util::normalize_underscores(lexed));
      ParserState p = pstate;
      lex< exactly<':'> >();
      Expression* val = parse_space_list();
      val->is_delayed(false);
      arg = new (ctx.mem) Argument(p, val, name);
    }
    else {
      bool is_arglist = false;
      bool is_keyword = false;
      Expression* val = parse_space_list();
      val->is_delayed(false);
      if (lex< exactly< ellipsis > >()) {
        if (val->concrete_type() == Expression::MAP) is_keyword = true;
        else is_arglist = true;
      }
      arg = new (ctx.mem) Argument(pstate, val, "", is_arglist, is_keyword);
    }
    return arg;
  }

  Assignment* Parser::parse_assignment()
  {
    lex< variable >();
    string name(Util::normalize_underscores(lexed));
    ParserState var_source_position = pstate;
    if (!lex< exactly<':'> >()) error("expected ':' after " + name + " in assignment statement", pstate);
    Expression* val = parse_list();
    val->is_delayed(false);
    bool is_guarded = false;
    bool is_global = false;
    while (peek< default_flag >() || peek< global_flag >()) {
      is_guarded = lex< default_flag >() || is_guarded;
      is_global = lex< global_flag >() || is_global;
    }
    Assignment* var = new (ctx.mem) Assignment(var_source_position, name, val, is_guarded, is_global);
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
      property_segment = new (ctx.mem) String_Constant(pstate, lexed);
    }
    Propset* propset = new (ctx.mem) Propset(pstate, property_segment);
    lex< exactly<':'> >();

    if (!peek< exactly<'{'> >()) error("expected a '{' after namespaced property", pstate);

    propset->block(parse_block());

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
    bool old_in_at_root = in_at_root;
    in_at_root = false;
    ParserState r_source_position = pstate;
    if (!peek< exactly<'{'> >()) error("expected a '{' after the selector", pstate);
    Block* block = parse_block();
    in_at_root = old_in_at_root;
    old_in_at_root = false;
    Ruleset* ruleset = new (ctx.mem) Ruleset(r_source_position, sel, block);
    return ruleset;
  }

  Selector_Schema* Parser::parse_selector_schema(const char* end_of_selector)
  {
    lex< optional_spaces >();
    const char* i = position;
    const char* p;
    String_Schema* schema = new (ctx.mem) String_Schema(pstate);

    while (i < end_of_selector) {
      p = find_first_in_interval< exactly<hash_lbrace> >(i, end_of_selector);
      if (p) {
        // accumulate the preceding segment if there is one
        if (i < p) (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, p, Position(0, 0)));
        // find the end of the interpolant and parse it
        const char* j = find_first_in_interval< exactly<rbrace> >(p, end_of_selector);
        Expression* interp_node = Parser::from_token(Token(p+2, j, Position(0, 0)), ctx, pstate).parse_list();
        interp_node->is_interpolant(true);
        (*schema) << interp_node;
        i = j + 1;
      }
      else { // no interpolants left; add the last segment if there is one
        if (i < end_of_selector) (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, end_of_selector, Position(0, 0)));
        break;
      }
    }
    position = end_of_selector;
    return new (ctx.mem) Selector_Schema(pstate, schema);
  }

  Selector_List* Parser::parse_selector_group()
  {
    To_String to_string;
    lex< spaces_and_comments >();
    Selector_List* group = new (ctx.mem) Selector_List(pstate);
    do {
      if (peek< exactly<'{'> >() ||
          peek< exactly<'}'> >() ||
          peek< exactly<')'> >() ||
          peek< exactly<';'> >())
        break; // in case there are superfluous commas at the end
      Complex_Selector* comb = parse_selector_combination();
      if (!comb->has_reference() && !in_at_root) {
        ParserState sel_source_position = pstate;
        Selector_Reference* ref = new (ctx.mem) Selector_Reference(sel_source_position);
        Compound_Selector* ref_wrap = new (ctx.mem) Compound_Selector(sel_source_position);
        (*ref_wrap) << ref;
        if (!comb->head()) {
          comb->head(ref_wrap);
          comb->has_reference(true);
        }
        else {
          comb = new (ctx.mem) Complex_Selector(sel_source_position, Complex_Selector::ANCESTOR_OF, ref_wrap, comb);
          comb->has_reference(true);
        }
      }
      (*group) << comb;
    }
    while (lex< one_plus< sequence< spaces_and_comments, exactly<','> > > >());
    while (lex< optional >());    // JMA - ignore optional flag if it follows the selector group
    return group;
  }

  Complex_Selector* Parser::parse_selector_combination()
  {
    lex< spaces_and_comments >();
    Position sel_source_position(-1);
    Compound_Selector* lhs;
    if (peek< exactly<'+'> >() ||
        peek< exactly<'~'> >() ||
        peek< exactly<'>'> >()) {
      // no selector before the combinator
      lhs = 0;
    }
    else {
      lhs = parse_simple_selector_sequence();
      sel_source_position = before_token;
    }

    Complex_Selector::Combinator cmb;
    if      (lex< exactly<'+'> >()) cmb = Complex_Selector::ADJACENT_TO;
    else if (lex< exactly<'~'> >()) cmb = Complex_Selector::PRECEDES;
    else if (lex< exactly<'>'> >()) cmb = Complex_Selector::PARENT_OF;
    else                            cmb = Complex_Selector::ANCESTOR_OF;

    Complex_Selector* rhs;
    if (peek< exactly<','> >() ||
        peek< exactly<')'> >() ||
        peek< exactly<'{'> >() ||
        peek< exactly<'}'> >() ||
        peek< exactly<';'> >() ||
        peek< optional >()) {
      // no selector after the combinator
      rhs = 0;
    }
    else {
      rhs = parse_selector_combination();
      sel_source_position = before_token;
    }
    if (!sel_source_position.line) sel_source_position = before_token;
    return new (ctx.mem) Complex_Selector(ParserState(path, sel_source_position, Offset(0, 0)), cmb, lhs, rhs);
  }

  Compound_Selector* Parser::parse_simple_selector_sequence()
  {
    Compound_Selector* seq = new (ctx.mem) Compound_Selector(pstate);
    bool sawsomething = false;
    if (lex< exactly<'&'> >()) {
      // if you see a &
      (*seq) << new (ctx.mem) Selector_Reference(pstate);
      sawsomething = true;
      // if you see a space after a &, then you're done
      if(lex< spaces >()) {
        return seq;
      }
    }
    if (sawsomething && lex< sequence< negate< functional >, alternatives< identifier_fragment, universal, string_constant, dimension, percentage, number > > >()) {
      // saw an ampersand, then allow type selectors with arbitrary number of hyphens at the beginning
      (*seq) << new (ctx.mem) Type_Selector(pstate, lexed);
    } else if (lex< sequence< negate< functional >, alternatives< type_selector, universal, string_constant, dimension, percentage, number > > >()) {
      // if you see a type selector
      (*seq) << new (ctx.mem) Type_Selector(pstate, lexed);
      sawsomething = true;
    }
    if (!sawsomething) {
      // don't blindly do this if you saw a & or selector
      (*seq) << parse_simple_selector();
    }

    while (!peek< spaces >(position) &&
           !(peek < exactly<'+'> >(position) ||
             peek < exactly<'~'> >(position) ||
             peek < exactly<'>'> >(position) ||
             peek < exactly<','> >(position) ||
             peek < exactly<')'> >(position) ||
             peek < exactly<'{'> >(position) ||
             peek < exactly<'}'> >(position) ||
             peek < exactly<';'> >(position))) {
      (*seq) << parse_simple_selector();
    }
    return seq;
  }

  Simple_Selector* Parser::parse_simple_selector()
  {
    if (lex< id_name >() || lex< class_name >()) {
      return new (ctx.mem) Selector_Qualifier(pstate, lexed);
    }
    else if (lex< string_constant >() || lex< number >()) {
      return new (ctx.mem) Type_Selector(pstate, lexed);
    }
    else if (peek< pseudo_not >()) {
      return parse_negated_selector();
    }
    else if (peek< exactly<':'> >(position) || peek< functional >()) {
      return parse_pseudo_selector();
    }
    else if (peek< exactly<'['> >(position)) {
      return parse_attribute_selector();
    }
    else if (lex< placeholder >()) {
      return new (ctx.mem) Selector_Placeholder(pstate, lexed);
    }
    else {
      error("invalid selector after " + lexed.to_string(), pstate);
    }
    // unreachable statement
    return 0;
  }

  Wrapped_Selector* Parser::parse_negated_selector()
  {
    lex< pseudo_not >();
    string name(lexed);
    ParserState nsource_position = pstate;
    Selector* negated = parse_selector_group();
    if (!lex< exactly<')'> >()) {
      error("negated selector is missing ')'", pstate);
    }
    return new (ctx.mem) Wrapped_Selector(nsource_position, name, negated);
  }

  Simple_Selector* Parser::parse_pseudo_selector() {
    if (lex< sequence< pseudo_prefix, functional > >() || lex< functional >()) {
      string name(lexed);
      String* expr = 0;
      ParserState p = pstate;
      Selector* wrapped = 0;
      if (lex< alternatives< even, odd > >()) {
        expr = new (ctx.mem) String_Constant(p, lexed);
      }
      else if (peek< binomial >(position)) {
        lex< sequence< optional< coefficient >, exactly<'n'> > >();
        String_Constant* var_coef = new (ctx.mem) String_Constant(p, lexed);
        lex< sign >();
        String_Constant* op = new (ctx.mem) String_Constant(p, lexed);
        // Binary_Expression::Type op = (lexed == "+" ? Binary_Expression::ADD : Binary_Expression::SUB);
        lex< digits >();
        String_Constant* constant = new (ctx.mem) String_Constant(p, lexed);
        // expr = new (ctx.mem) Binary_Expression(p, op, var_coef, constant);
        String_Schema* schema = new (ctx.mem) String_Schema(p, 3);
        *schema << var_coef << op << constant;
        expr = schema;
      }
      else if (peek< sequence< optional<sign>,
                               optional<digits>,
                               exactly<'n'>,
                               spaces_and_comments,
                               exactly<')'> > >()) {
        lex< sequence< optional<sign>,
                       optional<digits>,
                       exactly<'n'> > >();
        expr = new (ctx.mem) String_Constant(p, lexed);
      }
      else if (lex< sequence< optional<sign>, digits > >()) {
        expr = new (ctx.mem) String_Constant(p, lexed);
      }
      else if (peek< sequence< identifier, spaces_and_comments, exactly<')'> > >()) {
        lex< identifier >();
        expr = new (ctx.mem) String_Constant(p, lexed);
      }
      else if (lex< string_constant >()) {
        expr = new (ctx.mem) String_Constant(p, lexed);
      }
      else if (peek< exactly<')'> >()) {
        expr = new (ctx.mem) String_Constant(p, "");
      }
      else {
        wrapped = parse_selector_group();
      }
      if (!lex< exactly<')'> >()) error("unterminated argument to " + name + "...)", pstate);
      if (wrapped) {
        return new (ctx.mem) Wrapped_Selector(p, name, wrapped);
      }
      return new (ctx.mem) Pseudo_Selector(p, name, expr);
    }
    else if (lex < sequence< pseudo_prefix, identifier > >()) {
      return new (ctx.mem) Pseudo_Selector(pstate, lexed);
    }
    else {
      error("unrecognized pseudo-class or pseudo-element", pstate);
    }
    // unreachable statement
    return 0;
  }

  Attribute_Selector* Parser::parse_attribute_selector()
  {
    lex< exactly<'['> >();
    ParserState p = pstate;
    if (!lex< attribute_name >()) error("invalid attribute name in attribute selector", pstate);
    string name(lexed);
    if (lex< exactly<']'> >()) return new (ctx.mem) Attribute_Selector(p, name, "", 0);
    if (!lex< alternatives< exact_match, class_match, dash_match,
                            prefix_match, suffix_match, substring_match > >()) {
      error("invalid operator in attribute selector for " + name, pstate);
    }
    string matcher(lexed);

    String* value = 0;
    if (lex< identifier >()) {
      value = new (ctx.mem) String_Constant(p, lexed, true);
    }
    else if (lex< string_constant >()) {
      value = parse_interpolated_chunk(lexed);
    }
    else {
      error("expected a string constant or identifier in attribute selector for " + name, pstate);
    }

    if (!lex< exactly<']'> >()) error("unterminated attribute selector for " + name, pstate);
    return new (ctx.mem) Attribute_Selector(p, name, matcher, value);
  }

  Block* Parser::parse_block()
  {
    lex< exactly<'{'> >();
    bool semicolon = false;
    Selector_Lookahead lookahead_result;
    Block* block = new (ctx.mem) Block(pstate);

    // JMA - ensure that a block containing only block_comments is parsed
    while (lex< block_comment >()) {
      String*  contents = parse_interpolated_chunk(lexed);
      Comment* comment  = new (ctx.mem) Comment(pstate, contents);
      (*block) << comment;
    }

    while (!lex< exactly<'}'> >()) {
      if (semicolon) {
        if (!lex< one_plus< exactly<';'> > >()) {
          error("non-terminal statement or declaration must end with ';'", pstate);
        }
        semicolon = false;
        while (lex< block_comment >()) {
          String*  contents = parse_interpolated_chunk(lexed);
          Comment* comment  = new (ctx.mem) Comment(pstate, contents);
          (*block) << comment;
        }
        if (lex< sequence< exactly<'}'>, zero_plus< exactly<';'> > > >()) break;
      }
      if (lex< block_comment >()) {
        String*  contents = parse_interpolated_chunk(lexed);
        Comment* comment  = new (ctx.mem) Comment(pstate, contents);
        (*block) << comment;
      }
      else if (peek< import >(position)) {
        if (stack.back() == mixin_def || stack.back() == function_def) {
          lex< import >(); // to adjust the before_token number
          error("@import directives are not allowed inside mixins and functions", pstate);
        }
        Import* imp = parse_import();
        if (!imp->urls().empty()) (*block) << imp;
        if (!imp->files().empty()) {
          for (size_t i = 0, S = imp->files().size(); i < S; ++i) {
            (*block) << new (ctx.mem) Import_Stub(pstate, imp->files()[i]);
          }
        }
        semicolon = true;
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
        (*block) << new (ctx.mem) Return(pstate, parse_list());
        semicolon = true;
      }
      else if (peek< warn >()) {
        (*block) << parse_warning();
        semicolon = true;
      }
      else if (peek< err >()) {
        (*block) << parse_error();
        semicolon = true;
      }
      else if (peek< dbg >()) {
        (*block) << parse_debug();
        semicolon = true;
      }
      else if (stack.back() == function_def) {
        error("only variable declarations and control directives are allowed inside functions", pstate);
      }
      else if (peek< mixin >() || peek< function >()) {
        (*block) << parse_definition();
      }
      else if (peek< include >(position)) {
        Mixin_Call* the_call = parse_mixin_call();
        (*block) << the_call;
        // don't need a semicolon after a content block
        semicolon = (the_call->block()) ? false : true;
      }
      else if (lex< content >()) {
        if (stack.back() != mixin_def) {
          error("@content may only be used within a mixin", pstate);
        }
        (*block) << new (ctx.mem) Content(pstate);
        semicolon = true;
      }
      /*
      else if (peek< exactly<'+'> >()) {
        (*block) << parse_mixin_call();
        semicolon = true;
      }
      */
      else if (lex< extend >()) {
        Selector_Lookahead lookahead = lookahead_for_extension_target(position);
        if (!lookahead.found) error("invalid selector for @extend", pstate);
        Selector* target;
        if (lookahead.has_interpolants) target = parse_selector_schema(lookahead.found);
        else                            target = parse_selector_group();
        (*block) << new (ctx.mem) Extension(pstate, target);
        semicolon = true;
      }
      else if (peek< media >()) {
        (*block) << parse_media_block();
      }
      else if (peek< supports >()) {
        (*block) << parse_feature_block();
      }
      else if (peek< at_root >()) {
        (*block) << parse_at_root_block();
      }
      // ignore the @charset directive for now
      else if (lex< exactly< charset_kwd > >()) {
        lex< string_constant >();
        lex< one_plus< exactly<';'> > >();
      }
      else if (peek< at_keyword >()) {
        At_Rule* at_rule = parse_at_rule();
        (*block) << at_rule;
        if (!at_rule->block()) semicolon = true;
      }
      else if ((lookahead_result = lookahead_for_selector(position)).found) {
        (*block) << parse_ruleset(lookahead_result);
      }
      else if (peek< sequence< optional< exactly<'*'> >, alternatives< identifier_schema, identifier >, optional_spaces, exactly<':'>, optional_spaces, exactly<'{'> > >(position)) {
        (*block) << parse_propset();
      }
      else if (!peek< exactly<';'> >()) {
        if (peek< sequence< optional< exactly<'*'> >, identifier_schema, exactly<':'>, exactly<'{'> > >()) {
          (*block) << parse_propset();
        }
        else if (peek< sequence< optional< exactly<'*'> >, identifier, exactly<':'>, exactly<'{'> > >()) {
          (*block) << parse_propset();
        }
        else {
          Declaration* decl = parse_declaration();
          (*block) << decl;
          if (peek< exactly<'{'> >()) {
            // parse a propset that rides on the declaration's property
            Propset* ps = new (ctx.mem) Propset(pstate, decl->property(), parse_block());
            (*block) << ps;
          }
          else {
            // finish and let the semicolon get munched
            semicolon = true;
          }
        }
      }
      else lex< one_plus< exactly<';'> > >();
      while (lex< block_comment >()) {
        String*  contents = parse_interpolated_chunk(lexed);
        Comment* comment  = new (ctx.mem) Comment(pstate, contents);
        (*block) << comment;
      }
    }
    return block;
  }

  Declaration* Parser::parse_declaration() {
    String* prop = 0;
    if (peek< sequence< optional< exactly<'*'> >, identifier_schema > >()) {
      prop = parse_identifier_schema();
    }
    else if (lex< sequence< optional< exactly<'*'> >, identifier > >()) {
      prop = new (ctx.mem) String_Constant(pstate, lexed);
    }
    else if (lex< custom_property_name >()) {
      prop = new (ctx.mem) String_Constant(pstate, lexed);
    }
    else {
      error("invalid property name", pstate);
    }
    if (!lex< one_plus< exactly<':'> > >()) error("property \"" + string(lexed) + "\" must be followed by a ':'", pstate);
    if (peek< exactly<';'> >()) error("style declaration must contain a value", pstate);
    if (peek< static_value >()) {
      return new (ctx.mem) Declaration(prop->pstate(), prop, parse_static_value()/*, lex<important>()*/);
    }
    else {
      return new (ctx.mem) Declaration(prop->pstate(), prop, parse_list()/*, lex<important>()*/);
    }
  }

  Expression* Parser::parse_map()
  {
    To_String to_string;
    Expression* key = parse_list();

    // it's not a map so return the lexed value as a list value
    if (!peek< exactly<':'> >())
    { return key; }

    lex< exactly<':'> >();

    Expression* value = parse_space_list();

    Map* map = new (ctx.mem) Map(pstate, 1);
    (*map) << make_pair(key, value);

    while (lex< exactly<','> >())
    {
      // allow trailing commas - #495
      if (peek< exactly<')'> >(position))
      { break; }

      Expression* key = parse_list();

      if (!(lex< exactly<':'> >()))
      { error("invalid syntax", pstate); }

      Expression* value = parse_space_list();

      (*map) << make_pair(key, value);
    }

    if (map->has_duplicate_key())
    { error("Duplicate key \"" + map->get_duplicate_key()->perform(&to_string) + "\" in map " + map->perform(&to_string) + ".", pstate); }

    return map;
  }

  Expression* Parser::parse_list()
  {
    return parse_comma_list();
  }

  Expression* Parser::parse_comma_list()
  {
    if (//peek< exactly<'!'> >(position) ||
        peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<'{'> >(position) ||
        peek< exactly<')'> >(position) ||
        //peek< exactly<':'> >(position) ||
        peek< exactly<ellipsis> >(position))
    { return new (ctx.mem) List(pstate, 0); }
    Expression* list1 = parse_space_list();
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< exactly<','> >(position)) return list1;

    List* comma_list = new (ctx.mem) List(pstate, 2, List::COMMA);
    (*comma_list) << list1;

    while (lex< exactly<','> >())
    {
      if (//peek< exactly<'!'> >(position) ||
          peek< exactly<';'> >(position) ||
          peek< exactly<'}'> >(position) ||
          peek< exactly<'{'> >(position) ||
          peek< exactly<')'> >(position) ||
          peek< exactly<':'> >(position) ||
          peek< exactly<ellipsis> >(position)) {
        break;
      }
      Expression* list = parse_space_list();
      (*comma_list) << list;
    }

    return comma_list;
  }

  Expression* Parser::parse_space_list()
  {
    Expression* disj1 = parse_disjunction();
    // if it's a singleton, return it directly; don't wrap it
    if (//peek< exactly<'!'> >(position) ||
        peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<'{'> >(position) ||
        peek< exactly<')'> >(position) ||
        peek< exactly<','> >(position) ||
        peek< exactly<':'> >(position) ||
        peek< exactly<ellipsis> >(position) ||
        peek< default_flag >(position) ||
        peek< global_flag >(position))
    { return disj1; }

    List* space_list = new (ctx.mem) List(pstate, 2, List::SPACE);
    (*space_list) << disj1;

    while (!(//peek< exactly<'!'> >(position) ||
             peek< exactly<';'> >(position) ||
             peek< exactly<'}'> >(position) ||
             peek< exactly<'{'> >(position) ||
             peek< exactly<')'> >(position) ||
             peek< exactly<','> >(position) ||
             peek< exactly<':'> >(position) ||
             peek< exactly<ellipsis> >(position) ||
             peek< default_flag >(position) ||
             peek< global_flag >(position)))
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
    : lex<lt_op>()  ? Binary_Expression::LT
    :                 Binary_Expression::LT; // whatever

    Expression* expr2 = parse_expression();

    return new (ctx.mem) Binary_Expression(expr1->pstate(), op, expr1, expr2);
  }

  Expression* Parser::parse_expression()
  {
    Expression* term1 = parse_term();
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'+'> >(position) ||
          (peek< no_spaces >(position) && peek< sequence< negate< unsigned_number >, exactly<'-'>, negate< space > > >(position)) ||
          (peek< sequence< negate< unsigned_number >, exactly<'-'>, negate< unsigned_number > > >(position))) ||
          peek< identifier >(position))
    { return term1; }

    vector<Expression*> operands;
    vector<Binary_Expression::Type> operators;
    while (lex< exactly<'+'> >() || lex< sequence< negate< digits >, exactly<'-'> > >()) {
      operators.push_back(lexed.to_string() == "+" ? Binary_Expression::ADD : Binary_Expression::SUB);
      operands.push_back(parse_term());
    }

    return fold_operands(term1, operands, operators);
  }

  Expression* Parser::parse_term()
  {
    Expression* fact1 = parse_factor();

    // Special case: Ruby sass never tries to modulo if the lhs contains an interpolant
    if (peek< exactly<'%'> >(position) && fact1->concrete_type() == Expression::STRING) {
      try {
        String_Schema* ss = dynamic_cast<String_Schema*>(fact1);
        if (ss->has_interpolants()) return fact1;
      }
      catch (bad_cast&) {}
      catch (...) { throw; }
    }

    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'*'> >(position) ||
          peek< exactly<'/'> >(position) ||
          peek< exactly<'%'> >(position)))
    { return fact1; }

    vector<Expression*> operands;
    vector<Binary_Expression::Type> operators;
    while (lex< exactly<'*'> >() || lex< exactly<'/'> >() || lex< exactly<'%'> >()) {
      if      (lexed.to_string() == "*") operators.push_back(Binary_Expression::MUL);
      else if (lexed.to_string() == "/") operators.push_back(Binary_Expression::DIV);
      else                   operators.push_back(Binary_Expression::MOD);
      operands.push_back(parse_factor());
    }

    return fold_operands(fact1, operands, operators);
  }

  Expression* Parser::parse_factor()
  {
    if (lex< exactly<'('> >()) {
      Expression* value = parse_map();
      if (!lex< exactly<')'> >()) error("unclosed parenthesis", pstate);
      value->is_delayed(false);
      // make sure wrapped lists and division expressions are non-delayed within parentheses
      if (value->concrete_type() == Expression::LIST) {
        List* l = static_cast<List*>(value);
        if (!l->empty()) (*l)[0]->is_delayed(false);
      } else if (typeid(*value) == typeid(Binary_Expression)) {
        Binary_Expression* b = static_cast<Binary_Expression*>(value);
        Binary_Expression* lhs = static_cast<Binary_Expression*>(b->left());
        if (lhs && lhs->type() == Binary_Expression::DIV) lhs->is_delayed(false);
      }
      return value;
    }
    else if (peek< ie_property >()) {
      return parse_ie_property();
    }
    else if (peek< ie_keyword_arg >()) {
      return parse_ie_keyword_arg();
    }
    else if (peek< exactly< calc_kwd > >() ||
             peek< exactly< moz_calc_kwd > >() ||
             peek< exactly< webkit_calc_kwd > >()) {
      return parse_calc_function();
    }
    else if (peek< functional_schema >()) {
      return parse_function_call_schema();
    }
    else if (peek< sequence< identifier_schema, negate< exactly<'%'> > > >()) {
      return parse_identifier_schema();
    }
    else if (peek< functional >() && !peek< uri_prefix >()) {
      return parse_function_call();
    }
    else if (lex< sequence< exactly<'+'>, spaces_and_comments, negate< number > > >()) {
      return new (ctx.mem) Unary_Expression(pstate, Unary_Expression::PLUS, parse_factor());
    }
    else if (lex< sequence< exactly<'-'>, spaces_and_comments, negate< number> > >()) {
      return new (ctx.mem) Unary_Expression(pstate, Unary_Expression::MINUS, parse_factor());
    }
    else if (lex< sequence< not_op, spaces_and_comments > >()) {
      return new (ctx.mem) Unary_Expression(pstate, Unary_Expression::NOT, parse_factor());
    }
    else {
      return parse_value();
    }
  }

  Expression* Parser::parse_value()
  {
    if (lex< uri_prefix >()) {
      Arguments* args = new (ctx.mem) Arguments(pstate);
      Function_Call* result = new (ctx.mem) Function_Call(pstate, "url", args);
      const char* here = position;
      Position here_p = before_token;
      // Try to parse a SassScript expression. If it succeeds and we can munch
      // a matching rparen, then that's our url. If we can't munch a matching
      // rparen, or if the attempt to parse an expression fails, then try to
      // munch a regular CSS url.
      try {
        // special case -- if there's a comment, treat it as part of a URL
        lex<spaces>();
        if (peek<line_comment_prefix>() || peek<block_comment_prefix>()) error("comment in URL", pstate); // doesn't really matter what we throw
        Expression* expr = parse_list();
        if (!lex< exactly<')'> >()) error("dangling expression in URL", pstate); // doesn't really matter what we throw
        Argument* arg = new (ctx.mem) Argument(expr->pstate(), expr);
        *args << arg;
        return result;
      }
      catch (Sass_Error&) {
        // back up so we can try again
        position = here;
        before_token = here_p;
      }
      catch (...) { throw; }
      lex< spaces >();
      if (lex< url >()) {
        String* the_url = parse_interpolated_chunk(lexed);
        Argument* arg = new (ctx.mem) Argument(the_url->pstate(), the_url);
        *args << arg;
      }
      else {
        error("malformed URL", pstate);
      }
      if (!lex< exactly<')'> >()) error("URI is missing ')'", pstate);
      return result;
    }

    if (lex< important >())
    { return new (ctx.mem) String_Constant(pstate, "!important"); }

    if (lex< value_schema >())
    { return Parser::from_token(lexed, ctx, pstate).parse_value_schema(); }

    if (lex< sequence< true_val, negate< identifier > > >())
    { return new (ctx.mem) Boolean(pstate, true); }

    if (lex< sequence< false_val, negate< identifier > > >())
    { return new (ctx.mem) Boolean(pstate, false); }

    if (lex< sequence< null, negate< identifier > > >())
    { return new (ctx.mem) Null(pstate); }

    if (lex< identifier >()) {
      String_Constant* str = new (ctx.mem) String_Constant(pstate, lexed);
      // Dont' delay this string if it is a name color. Fixes #652.
      str->is_delayed(ctx.names_to_colors.count(lexed) == 0);
      return str;
    }

    if (lex< percentage >())
    { return new (ctx.mem) Textual(pstate, Textual::PERCENTAGE, lexed); }

    // match hex number first because 0x000 looks like a number followed by an indentifier
    if (lex< alternatives< hex, hex0 > >())
    { return new (ctx.mem) Textual(pstate, Textual::HEX, lexed); }

    // also handle the 10em- foo special case
    if (lex< sequence< dimension, optional< sequence< exactly<'-'>, negate< digit > > > > >())
    { return new (ctx.mem) Textual(pstate, Textual::DIMENSION, lexed); }

    if (lex< number >())
    { return new (ctx.mem) Textual(pstate, Textual::NUMBER, lexed); }

    if (peek< string_constant >())
    { return parse_string(); }

    if (lex< variable >())
    { return new (ctx.mem) Variable(pstate, Util::normalize_underscores(lexed)); }

    // Special case handling for `%` proceeding an interpolant.
    if (lex< sequence< exactly<'%'>, optional< percentage > > >())
    { return new (ctx.mem) String_Constant(pstate, lexed); }

    error("error reading values after " + lexed.to_string(), pstate);

    // unreachable statement
    return 0;
  }

  String* Parser::parse_interpolated_chunk(Token chunk)
  {
    const char* i = chunk.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(chunk.begin, chunk.end);
    if (!p) {
      String_Constant* str_node = new (ctx.mem) String_Constant(pstate, chunk, dequote);
      str_node->is_delayed(true);
      return str_node;
    }

    String_Schema* schema = new (ctx.mem) String_Schema(pstate);
    schema->quote_mark(*chunk.begin);
    while (i < chunk.end) {
      p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, chunk.end);
      if (p) {
        if (i < p) {
          (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, p, before_token)); // accumulate the preceding segment if it's nonempty
        }
        const char* j = find_first_in_interval< exactly<rbrace> >(p, chunk.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          Expression* interp_node = Parser::from_token(Token(p+2, j, before_token), ctx, pstate).parse_list();
          interp_node->is_interpolant(true);
          (*schema) << interp_node;
          i = j+1;
        }
        else {
          // throw an error if the interpolant is unterminated
          error("unterminated interpolant inside string constant " + chunk.to_string(), pstate);
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < chunk.end) (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, chunk.end, before_token));
        break;
      }
    }
    return schema;
  }

  String_Constant* Parser::parse_static_value()
  {
    lex< static_value >();
    Token str(lexed);
    --str.end;
    --position;
    String_Constant* str_node = new (ctx.mem) String_Constant(pstate, str);
    str_node->is_delayed(true);
    return str_node;
  }

  String* Parser::parse_string()
  {
    lex< string_constant >();
    Token str(lexed);
    return parse_interpolated_chunk(str);
    // const char* i = str.begin;
    // // see if there any interpolants
    // const char* p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(str.begin, str.end);
    // if (!p) {
    //   String_Constant* str_node = new (ctx.mem) String_Constant(pstate, str);
    //   str_node->is_delayed(true);
    //   return str_node;
    // }

    // String_Schema* schema = new (ctx.mem) String_Schema(pstate);
    // schema->quote_mark(*str.begin);
    // while (i < str.end) {
    //   p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, str.end);
    //   if (p) {
    //     if (i < p) {
    //       (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, p)); // accumulate the preceding segment if it's nonempty
    //     }
    //     const char* j = find_first_in_interval< exactly<rbrace> >(p, str.end); // find the closing brace
    //     if (j) {
    //       // parse the interpolant and accumulate it
    //       Expression* interp_node = Parser::from_token(Token(p+2, j), ctx, pstate).parse_list();
    //       interp_node->is_interpolant(true);
    //       (*schema) << interp_node;
    //       i = j+1;
    //     }
    //     else {
    //       // throw an error if the interpolant is unterminated
    //       error("unterminated interpolant inside string constant " + str.to_string(), pstate);
    //     }
    //   }
    //   else { // no interpolants left; add the last segment if nonempty
    //     if (i < str.end) (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, str.end));
    //     break;
    //   }
    // }
    // return schema;
  }

  String* Parser::parse_ie_property()
  {
    lex< ie_property >();
    Token str(lexed);
    const char* i = str.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(str.begin, str.end);
    if (!p) {
      String_Constant* str_node = new (ctx.mem) String_Constant(pstate, str);
      str_node->is_delayed(true);
      return str_node;
    }

    String_Schema* schema = new (ctx.mem) String_Schema(pstate);
    while (i < str.end) {
      p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, str.end);
      if (p) {
        if (i < p) {
          (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, p, before_token)); // accumulate the preceding segment if it's nonempty
        }
        const char* j = find_first_in_interval< exactly<rbrace> >(p, str.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          Expression* interp_node = Parser::from_token(Token(p+2, j, before_token), ctx, pstate).parse_list();
          interp_node->is_interpolant(true);
          (*schema) << interp_node;
          i = j+1;
        }
        else {
          // throw an error if the interpolant is unterminated
          error("unterminated interpolant inside IE function " + str.to_string(), pstate);
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < str.end) (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, str.end, before_token));
        break;
      }
    }
    return schema;
  }

  String* Parser::parse_ie_keyword_arg()
  {
    String_Schema* kwd_arg = new (ctx.mem) String_Schema(pstate, 3);
    if (lex< variable >()) *kwd_arg << new (ctx.mem) Variable(pstate, Util::normalize_underscores(lexed));
    else {
      lex< alternatives< identifier_schema, identifier > >();
      *kwd_arg << new (ctx.mem) String_Constant(pstate, lexed);
    }
    lex< exactly<'='> >();
    *kwd_arg << new (ctx.mem) String_Constant(pstate, lexed);
    if (peek< variable >()) *kwd_arg << parse_list();
    else if (lex< number >()) *kwd_arg << new (ctx.mem) Textual(pstate, Textual::NUMBER, Util::normalize_decimals(lexed));
    else {
      lex< alternatives< identifier_schema, identifier, number, hex > >();
      *kwd_arg << new (ctx.mem) String_Constant(pstate, lexed);
    }
    return kwd_arg;
  }

  String_Schema* Parser::parse_value_schema()
  {
    String_Schema* schema = new (ctx.mem) String_Schema(pstate);
    size_t num_items = 0;
    while (position < end) {
      if (lex< interpolant >()) {
        Token insides(Token(lexed.begin + 2, lexed.end - 1, before_token));
        Expression* interp_node = Parser::from_token(insides, ctx, pstate).parse_list();
        interp_node->is_interpolant(true);
        (*schema) << interp_node;
      }
      else if (lex< exactly<'%'> >()) {
        (*schema) << new (ctx.mem) String_Constant(pstate, lexed);
      }
      else if (lex< identifier >()) {
        (*schema) << new (ctx.mem) String_Constant(pstate, lexed);
      }
      else if (lex< percentage >()) {
        (*schema) << new (ctx.mem) Textual(pstate, Textual::PERCENTAGE, lexed);
      }
      else if (lex< dimension >()) {
        (*schema) << new (ctx.mem) Textual(pstate, Textual::DIMENSION, lexed);
      }
      else if (lex< number >()) {
        (*schema) << new (ctx.mem) Textual(pstate, Textual::NUMBER, lexed);
      }
      else if (lex< hex >()) {
        (*schema) << new (ctx.mem) Textual(pstate, Textual::HEX, lexed);
      }
      else if (lex< string_constant >()) {
        (*schema) << new (ctx.mem) String_Constant(pstate, lexed);
        if (!num_items) schema->quote_mark(*lexed.begin);
      }
      else if (lex< variable >()) {
        (*schema) << new (ctx.mem) Variable(pstate, Util::normalize_underscores(lexed));
      }
      else {
        error("error parsing interpolated value", pstate);
      }
      ++num_items;
    }
    return schema;
  }

  String_Schema* Parser::parse_url_schema()
  {
    String_Schema* schema = new (ctx.mem) String_Schema(pstate);

    while (position < end) {
      if (position[0] == '/') {
        lexed = Token(position, position+1, before_token);
        (*schema) << new (ctx.mem) String_Constant(pstate, lexed);
        ++position;
      }
      else if (lex< interpolant >()) {
        Token insides(Token(lexed.begin + 2, lexed.end - 1, before_token));
        Expression* interp_node = Parser::from_token(insides, ctx, pstate).parse_list();
        interp_node->is_interpolant(true);
        (*schema) << interp_node;
      }
      else if (lex< sequence< identifier, exactly<':'> > >()) {
        (*schema) << new (ctx.mem) String_Constant(pstate, lexed);
      }
      else if (lex< filename >()) {
        (*schema) << new (ctx.mem) String_Constant(pstate, lexed);
      }
      else {
        error("error parsing interpolated url", pstate);
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
      return new (ctx.mem) String_Constant(pstate, id);
    }

    String_Schema* schema = new (ctx.mem) String_Schema(pstate);
    while (i < id.end) {
      p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, id.end);
      if (p) {
        if (i < p) {
          (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, p, before_token)); // accumulate the preceding segment if it's nonempty
        }
        const char* j = find_first_in_interval< exactly<rbrace> >(p, id.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          Expression* interp_node = Parser::from_token(Token(p+2, j, before_token), ctx, pstate).parse_list();
          interp_node->is_interpolant(true);
          (*schema) << interp_node;
          schema->has_interpolants(true);
          i = j+1;
        }
        else {
          // throw an error if the interpolant is unterminated
          error("unterminated interpolant inside interpolated identifier " + id.to_string(), pstate);
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < id.end) (*schema) << new (ctx.mem) String_Constant(pstate, Token(i, id.end, before_token));
        break;
      }
    }
    return schema;
  }

  Function_Call* Parser::parse_calc_function()
  {
    lex< identifier >();
    string name(lexed);
    ParserState call_pos = pstate;
    lex< exactly<'('> >();
    ParserState arg_pos = pstate;
    const char* arg_beg = position;
    parse_list();
    const char* arg_end = position;
    lex< exactly<')'> >();

    Argument* arg = new (ctx.mem) Argument(arg_pos, parse_interpolated_chunk(Token(arg_beg, arg_end, before_token)));
    Arguments* args = new (ctx.mem) Arguments(arg_pos);
    *args << arg;
    return new (ctx.mem) Function_Call(call_pos, name, args);
  }

  Function_Call* Parser::parse_function_call()
  {
    lex< identifier >();
    string name(Util::normalize_underscores(lexed));
    ParserState source_position_of_call = pstate;

    Function_Call* the_call = new (ctx.mem) Function_Call(source_position_of_call, name, parse_arguments());
    return the_call;
  }

  Function_Call_Schema* Parser::parse_function_call_schema()
  {
    String* name = parse_identifier_schema();
    ParserState source_position_of_call = pstate;

    Function_Call_Schema* the_call = new (ctx.mem) Function_Call_Schema(source_position_of_call, name, parse_arguments());
    return the_call;
  }

  If* Parser::parse_if_directive(bool else_if)
  {
    lex< if_directive >() || (else_if && lex< exactly<if_after_else_kwd> >());
    ParserState if_source_position = pstate;
    Expression* predicate = parse_list();
    predicate->is_delayed(false);
    if (!peek< exactly<'{'> >()) error("expected '{' after the predicate for @if", pstate);
    Block* consequent = parse_block();
    Block* alternative = 0;
    if (lex< else_directive >()) {
      if (peek< exactly<if_after_else_kwd> >()) {
        alternative = new (ctx.mem) Block(pstate);
        (*alternative) << parse_if_directive(true);
      }
      else if (!peek< exactly<'{'> >()) {
        error("expected '{' after @else", pstate);
      }
      else {
        alternative = parse_block();
      }
    }
    return new (ctx.mem) If(if_source_position, predicate, consequent, alternative);
  }

  For* Parser::parse_for_directive()
  {
    lex< for_directive >();
    ParserState for_source_position = pstate;
    if (!lex< variable >()) error("@for directive requires an iteration variable", pstate);
    string var(Util::normalize_underscores(lexed));
    if (!lex< from >()) error("expected 'from' keyword in @for directive", pstate);
    Expression* lower_bound = parse_expression();
    lower_bound->is_delayed(false);
    bool inclusive = false;
    if (lex< through >()) inclusive = true;
    else if (lex< to >()) inclusive = false;
    else                  error("expected 'through' or 'to' keyword in @for directive", pstate);
    Expression* upper_bound = parse_expression();
    upper_bound->is_delayed(false);
    if (!peek< exactly<'{'> >()) error("expected '{' after the upper bound in @for directive", pstate);
    Block* body = parse_block();
    return new (ctx.mem) For(for_source_position, var, lower_bound, upper_bound, body, inclusive);
  }

  Each* Parser::parse_each_directive()
  {
    lex < each_directive >();
    ParserState each_source_position = pstate;
    if (!lex< variable >()) error("@each directive requires an iteration variable", pstate);
    vector<string> vars;
    vars.push_back(Util::normalize_underscores(lexed));
    while (peek< exactly<','> >() && lex< exactly<','> >()) {
      if (!lex< variable >()) error("@each directive requires an iteration variable", pstate);
      vars.push_back(Util::normalize_underscores(lexed));
    }
    if (!lex< in >()) error("expected 'in' keyword in @each directive", pstate);
    Expression* list = parse_list();
    list->is_delayed(false);
    if (list->concrete_type() == Expression::LIST) {
      List* l = static_cast<List*>(list);
      for (size_t i = 0, L = l->length(); i < L; ++i) {
        (*l)[i]->is_delayed(false);
      }
    }
    if (!peek< exactly<'{'> >()) error("expected '{' after the upper bound in @each directive", pstate);
    Block* body = parse_block();
    return new (ctx.mem) Each(each_source_position, vars, list, body);
  }

  While* Parser::parse_while_directive()
  {
    lex< while_directive >();
    ParserState while_source_position = pstate;
    Expression* predicate = parse_list();
    predicate->is_delayed(false);
    Block* body = parse_block();
    return new (ctx.mem) While(while_source_position, predicate, body);
  }

  Media_Block* Parser::parse_media_block()
  {
    lex< media >();
    ParserState media_source_position = pstate;

    List* media_queries = parse_media_queries();

    if (!peek< exactly<'{'> >()) {
      error("expected '{' in media query", pstate);
    }
    Block* block = parse_block();

    return new (ctx.mem) Media_Block(media_source_position, media_queries, block);
  }

  List* Parser::parse_media_queries()
  {
    List* media_queries = new (ctx.mem) List(pstate, 0, List::COMMA);
    if (!peek< exactly<'{'> >()) (*media_queries) << parse_media_query();
    while (lex< exactly<','> >()) (*media_queries) << parse_media_query();
    return media_queries;
  }

  // Expression* Parser::parse_media_query()
  Media_Query* Parser::parse_media_query()
  {
    Media_Query* media_query = new (ctx.mem) Media_Query(pstate);

    if (lex< exactly< not_kwd > >()) media_query->is_negated(true);
    else if (lex< exactly< only_kwd > >()) media_query->is_restricted(true);

    if (peek< identifier_schema >()) media_query->media_type(parse_identifier_schema());
    else if (lex< identifier >())    media_query->media_type(new (ctx.mem) String_Constant(pstate, lexed));
    else                             (*media_query) << parse_media_expression();

    while (lex< exactly< and_kwd > >()) (*media_query) << parse_media_expression();

    return media_query;
  }

  Media_Query_Expression* Parser::parse_media_expression()
  {
    if (peek< identifier_schema >()) {
      String* ss = parse_identifier_schema();
      return new (ctx.mem) Media_Query_Expression(pstate, ss, 0, true);
    }
    if (!lex< exactly<'('> >()) {
      error("media query expression must begin with '('", pstate);
    }
    Expression* feature = 0;
    if (peek< exactly<')'> >()) {
      error("media feature required in media query expression", pstate);
    }
    feature = parse_expression();
    Expression* expression = 0;
    if (lex< exactly<':'> >()) {
      expression = parse_list();
    }
    if (!lex< exactly<')'> >()) {
      error("unclosed parenthesis in media query expression", pstate);
    }
    return new (ctx.mem) Media_Query_Expression(feature->pstate(), feature, expression);
  }

  Feature_Block* Parser::parse_feature_block()
  {
    lex< supports >();
    ParserState supports_source_position = pstate;

    Feature_Query* feature_queries = parse_feature_queries();

    if (!peek< exactly<'{'> >()) {
      error("expected '{' in feature query", pstate);
    }
    Block* block = parse_block();

    return new (ctx.mem) Feature_Block(supports_source_position, feature_queries, block);
  }

  Feature_Query* Parser::parse_feature_queries()
  {
    Feature_Query* fq = new (ctx.mem) Feature_Query(pstate);
    Feature_Query_Condition* cond = new (ctx.mem) Feature_Query_Condition(pstate);
    cond->is_root(true);
    while (!peek< exactly<')'> >(position) && !peek< exactly<'{'> >(position))
      (*cond) << parse_feature_query();
    (*fq) << cond;

    if (fq->empty()) error("expected @supports condition (e.g. (display: flexbox))", pstate);

    return fq;
  }

  Feature_Query_Condition* Parser::parse_feature_query()
  {
    if (peek< not_op >(position)) return parse_supports_negation();
    else if (peek< and_op >(position)) return parse_supports_conjunction();
    else if (peek< or_op >(position)) return parse_supports_disjunction();
    else if (peek< exactly<'('> >(position)) return parse_feature_query_in_parens();
    else return parse_supports_declaration();
  }

  Feature_Query_Condition* Parser::parse_feature_query_in_parens()
  {
    Feature_Query_Condition* cond = new (ctx.mem) Feature_Query_Condition(pstate);

    if (!lex< exactly<'('> >()) error("@supports declaration expected '('", pstate);
    while (!peek< exactly<')'> >(position) && !peek< exactly<'{'> >(position))
      (*cond) << parse_feature_query();
    if (!lex< exactly<')'> >()) error("unclosed parenthesis in @supports declaration", pstate);

    return (cond->length() == 1) ? (*cond)[0] : cond;
  }

  Feature_Query_Condition* Parser::parse_supports_negation()
  {
    lex< not_op >();

    Feature_Query_Condition* cond = parse_feature_query();
    cond->operand(Feature_Query_Condition::NOT);

    return cond;
  }

  Feature_Query_Condition* Parser::parse_supports_conjunction()
  {
    lex< and_op >();

    Feature_Query_Condition* cond = parse_feature_query();
    cond->operand(Feature_Query_Condition::AND);

    return cond;
  }

  Feature_Query_Condition* Parser::parse_supports_disjunction()
  {
    lex< or_op >();

    Feature_Query_Condition* cond = parse_feature_query();
    cond->operand(Feature_Query_Condition::OR);

    return cond;
  }

  Feature_Query_Condition* Parser::parse_supports_declaration()
  {
    Declaration* declaration = parse_declaration();
    Feature_Query_Condition* cond = new (ctx.mem) Feature_Query_Condition(declaration->pstate(),
                                                                          1,
                                                                          declaration->property(),
                                                                          declaration->value());
    return cond;
  }

  At_Root_Block* Parser::parse_at_root_block()
  {
    lex<at_root>();
    ParserState at_source_position = pstate;
    Block* body = 0;
    At_Root_Expression* expr = 0;
    Selector_Lookahead lookahead_result;
    in_at_root = true;
    if (peek< exactly<'('> >()) {
      expr = parse_at_root_expression();
      body = parse_block();
    }
    else if (peek< exactly<'{'> >()) {
      body = parse_block();
    }
    else if ((lookahead_result = lookahead_for_selector(position)).found) {
      Ruleset* r = parse_ruleset(lookahead_result);
      body = new (ctx.mem) Block(r->pstate(), 1);
      *body << r;
    }
    in_at_root = false;
    At_Root_Block* at_root = new (ctx.mem) At_Root_Block(at_source_position, body);
    if (expr) at_root->expression(expr);
    return at_root;
  }

  At_Root_Expression* Parser::parse_at_root_expression()
  {
    lex< exactly<'('> >();
    if (peek< exactly<')'> >()) error("at-root feature required in at-root expression", pstate);

    if (!peek< alternatives< with_directive, without_directive > >()) {
      const char* i = position;
      const char* p = peek< until_closing_paren >(i);
      Token* t = new Token(i, p, Position(0, 0));
      error("Invalid CSS after \"(\": expected \"with\" or \"without\", was \""+t->to_string()+"\"", pstate);
    }

    Declaration* declaration = parse_declaration();
    List* value = new (ctx.mem) List(declaration->value()->pstate(), 1);

    if (declaration->value()->concrete_type() == Expression::LIST) {
        value = static_cast<List*>(declaration->value());
    }
    else *value << declaration->value();

    At_Root_Expression* cond = new (ctx.mem) At_Root_Expression(declaration->pstate(),
                                                                declaration->property(),
                                                                value);
    if (!lex< exactly<')'> >()) error("unclosed parenthesis in @at-root expression", pstate);
    return cond;
  }

  At_Rule* Parser::parse_at_rule()
  {
    lex<at_keyword>();
    string kwd(lexed);
    ParserState at_source_position = pstate;
    Selector* sel = 0;
    Expression* val = 0;
    Selector_Lookahead lookahead = lookahead_for_extension_target(position);
    if (lookahead.found) {
      if (lookahead.has_interpolants) {
        sel = parse_selector_schema(lookahead.found);
      }
      else {
        sel = parse_selector_group();
      }
    }
    else if (!(peek<exactly<'{'> >() || peek<exactly<'}'> >() || peek<exactly<';'> >())) {
      val = parse_list();
    }
    Block* body = 0;
    if (peek< exactly<'{'> >()) body = parse_block();
    At_Rule* rule = new (ctx.mem) At_Rule(at_source_position, kwd, sel, body);
    if (!sel) rule->value(val);
    return rule;
  }

  Warning* Parser::parse_warning()
  {
    lex< warn >();
    return new (ctx.mem) Warning(pstate, parse_list());
  }

  Error* Parser::parse_error()
  {
    lex< err >();
    return new (ctx.mem) Error(pstate, parse_list());
  }

  Debug* Parser::parse_debug()
  {
    lex< dbg >();
    return new (ctx.mem) Debug(pstate, parse_list());
  }

  Selector_Lookahead Parser::lookahead_for_selector(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;
    bool saw_stuff = false;
    bool saw_interpolant = false;

    while ((q = peek< identifier >(p))                             ||
           (q = peek< hyphens_and_identifier >(p))                 ||
           (q = peek< hyphens_and_name >(p))                       ||
           (q = peek< type_selector >(p))                          ||
           (q = peek< id_name >(p))                                ||
           (q = peek< class_name >(p))                             ||
           (q = peek< sequence< pseudo_prefix, identifier > >(p))  ||
           (q = peek< percentage >(p))                             ||
           (q = peek< dimension >(p))                              ||
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
           (q = peek< sequence< exactly<'&'>,
                                identifier_fragment > >(p))        ||
           (q = peek< exactly<'&'> >(p))                           ||
           (q = peek< exactly<'%'> >(p))                           ||
           (q = peek< alternatives<exact_match,
                                   class_match,
                                   dash_match,
                                   prefix_match,
                                   suffix_match,
                                   substring_match> >(p))          ||
           (q = peek< sequence< exactly<'.'>, interpolant > >(p))  ||
           (q = peek< sequence< exactly<'#'>, interpolant > >(p))  ||
           (q = peek< sequence< one_plus< exactly<'-'> >, interpolant > >(p))  ||
           (q = peek< sequence< pseudo_prefix, interpolant > >(p)) ||
           (q = peek< interpolant >(p))) {
      saw_stuff = true;
      p = q;
      if (*(p - 1) == '}') saw_interpolant = true;
    }

    Selector_Lookahead result;
    result.found            = saw_stuff && peek< exactly<'{'> >(p) ? p : 0;
    result.has_interpolants = saw_interpolant;

    return result;
  }

  Selector_Lookahead Parser::lookahead_for_extension_target(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;
    bool saw_interpolant = false;
    bool saw_stuff = false;

    while ((q = peek< identifier >(p))                             ||
           (q = peek< type_selector >(p))                          ||
           (q = peek< id_name >(p))                                ||
           (q = peek< class_name >(p))                             ||
           (q = peek< sequence< pseudo_prefix, identifier > >(p))  ||
           (q = peek< percentage >(p))                             ||
           (q = peek< dimension >(p))                              ||
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
           (q = peek< sequence< exactly<'&'>,
                                identifier_fragment > >(p))        ||
           (q = peek< exactly<'&'> >(p))                           ||
           (q = peek< exactly<'%'> >(p))                           ||
           (q = peek< alternatives<exact_match,
                                   class_match,
                                   dash_match,
                                   prefix_match,
                                   suffix_match,
                                   substring_match> >(p))          ||
           (q = peek< sequence< exactly<'.'>, interpolant > >(p))  ||
           (q = peek< sequence< exactly<'#'>, interpolant > >(p))  ||
           (q = peek< sequence< one_plus< exactly<'-'> >, interpolant > >(p))  ||
           (q = peek< sequence< pseudo_prefix, interpolant > >(p)) ||
           (q = peek< interpolant >(p))                            ||
           (q = peek< optional >(p))) {
      p = q;
      if (*(p - 1) == '}') saw_interpolant = true;
      saw_stuff = true;
    }

    Selector_Lookahead result;
    result.found            = peek< alternatives< exactly<';'>, exactly<'}'>, exactly<'{'> > >(p) && saw_stuff ? p : 0;
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
      skip = check_bom_chars(source, end, utf_8_bom, 3);
      encoding = "UTF-8";
      utf_8 = true;
      break;
    case 0xFE:
      skip = check_bom_chars(source, end, utf_16_bom_be, 2);
      encoding = "UTF-16 (big endian)";
      break;
    case 0xFF:
      skip = check_bom_chars(source, end, utf_16_bom_le, 2);
      skip += (skip ? check_bom_chars(source, end, utf_32_bom_le, 4) : 0);
      encoding = (skip == 2 ? "UTF-16 (little endian)" : "UTF-32 (little endian)");
      break;
    case 0x00:
      skip = check_bom_chars(source, end, utf_32_bom_be, 4);
      encoding = "UTF-32 (big endian)";
      break;
    case 0x2B:
      skip = check_bom_chars(source, end, utf_7_bom_1, 4)
           | check_bom_chars(source, end, utf_7_bom_2, 4)
           | check_bom_chars(source, end, utf_7_bom_3, 4)
           | check_bom_chars(source, end, utf_7_bom_4, 4)
           | check_bom_chars(source, end, utf_7_bom_5, 5);
      encoding = "UTF-7";
      break;
    case 0xF7:
      skip = check_bom_chars(source, end, utf_1_bom, 3);
      encoding = "UTF-1";
      break;
    case 0xDD:
      skip = check_bom_chars(source, end, utf_ebcdic_bom, 4);
      encoding = "UTF-EBCDIC";
      break;
    case 0x0E:
      skip = check_bom_chars(source, end, scsu_bom, 3);
      encoding = "SCSU";
      break;
    case 0xFB:
      skip = check_bom_chars(source, end, bocu_1_bom, 3);
      encoding = "BOCU-1";
      break;
    case 0x84:
      skip = check_bom_chars(source, end, gb_18030_bom, 4);
      encoding = "GB-18030";
      break;
    }
    if (skip > 0 && !utf_8) error("only UTF-8 documents are currently supported; your document appears to be " + encoding, pstate);
    position += skip;
  }

  size_t check_bom_chars(const char* src, const char *end, const unsigned char* bom, size_t len)
  {
    size_t skip = 0;
    if (src + len > end) return 0;
    for (size_t i = 0; i < len; ++i, ++skip) {
      if ((unsigned char) src[i] != bom[i]) return 0;
    }
    return skip;
  }


  Expression* Parser::fold_operands(Expression* base, vector<Expression*>& operands, Binary_Expression::Type op)
  {
    for (size_t i = 0, S = operands.size(); i < S; ++i) {
      base = new (ctx.mem) Binary_Expression(pstate, op, base, operands[i]);
      Binary_Expression* b = static_cast<Binary_Expression*>(base);
      if (op == Binary_Expression::DIV && b->left()->is_delayed() && b->right()->is_delayed()) {
        base->is_delayed(true);
      }
      else {
        b->left()->is_delayed(false);
        b->right()->is_delayed(false);
      }
    }
    return base;
  }

  Expression* Parser::fold_operands(Expression* base, vector<Expression*>& operands, vector<Binary_Expression::Type>& ops)
  {
    for (size_t i = 0, S = operands.size(); i < S; ++i) {
      base = new (ctx.mem) Binary_Expression(base->pstate(), ops[i], base, operands[i]);
      Binary_Expression* b = static_cast<Binary_Expression*>(base);
      if (ops[i] == Binary_Expression::DIV && b->left()->is_delayed() && b->right()->is_delayed()) {
        base->is_delayed(true);
      }
      else {
        b->left()->is_delayed(false);
        b->right()->is_delayed(false);
      }
    }
    return base;
  }

  void Parser::error(string msg, Position pos)
  {
    throw Sass_Error(Sass_Error::syntax, ParserState(path, pos.line ? pos : before_token, Offset(0, 0)), msg);
  }

}
