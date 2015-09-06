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
#include "color_maps.hpp"
#include "sass/functions.h"
#include "error_handling.hpp"

#include <typeinfo>
#include <tuple>

namespace Sass {
  using namespace Constants;
  using namespace Prelexer;

  Parser Parser::from_c_str(const char* str, Context& ctx, ParserState pstate)
  {
    Parser p(ctx, pstate);
    p.source   = str;
    p.position = p.source;
    p.end      = str + strlen(str);
    Block* root = SASS_MEMORY_NEW(ctx.mem, Block, pstate);
    p.block_stack.push_back(root);
    root->is_root(true);
    return p;
  }

  Parser Parser::from_c_str(const char* beg, const char* end, Context& ctx, ParserState pstate)
  {
    Parser p(ctx, pstate);
    p.source   = beg;
    p.position = p.source;
    p.end      = end;
    Block* root = SASS_MEMORY_NEW(ctx.mem, Block, pstate);
    p.block_stack.push_back(root);
    root->is_root(true);
    return p;
  }

  Selector_List* Parser::parse_selector(const char* src, Context& ctx, ParserState pstate)
  {
    Parser p = Parser::from_c_str(src, ctx, pstate);
    // ToDo: ruby sass errors on parent references
    // ToDo: remap the source-map entries somehow
    return p.parse_selector_list(false);
  }

  bool Parser::peek_newline(const char* start)
  {
    return peek_linefeed(start ? start : position)
           && ! peek_css<exactly<'{'>>(start);
  }

  Parser Parser::from_token(Token t, Context& ctx, ParserState pstate)
  {
    Parser p(ctx, pstate);
    p.source   = t.begin;
    p.position = p.source;
    p.end      = t.end;
    Block* root = SASS_MEMORY_NEW(ctx.mem, Block, pstate);
    p.block_stack.push_back(root);
    root->is_root(true);
    return p;
  }

  /* main entry point to parse root block */
  Block* Parser::parse()
  {
    bool is_root = false;
    Block* root = SASS_MEMORY_NEW(ctx.mem, Block, pstate, 0, true);
    read_bom();

    if (ctx.queue.size() == 1) {
      is_root = true;
      Import* pre = SASS_MEMORY_NEW(ctx.mem, Import, pstate);
      std::string load_path(ctx.queue[0].load_path);
      do_import(load_path, pre, ctx.c_headers, false);
      ctx.head_imports = ctx.queue.size() - 1;
      if (!pre->urls().empty()) (*root) << pre;
      if (!pre->files().empty()) {
        for (size_t i = 0, S = pre->files().size(); i < S; ++i) {
          (*root) << SASS_MEMORY_NEW(ctx.mem, Import_Stub, pstate, pre->files()[i]);
        }
      }
    }

    block_stack.push_back(root);
    /* bool rv = */ parse_block_nodes(is_root);
    block_stack.pop_back();

    // update for end position
    root->update_pstate(pstate);

    if (position != end) {
      css_error("Invalid CSS", " after ", ": expected selector or at-rule, was ");
    }

    return root;
  }


  // convenience function for block parsing
  // will create a new block ad-hoc for you
  // this is the base block parsing function
  Block* Parser::parse_css_block(bool is_root)
  {

    // parse comments before block
    // lex < optional_css_comments >();
    // lex mandatory opener or error out
    if (!lex_css < exactly<'{'> >()) {
      css_error("Invalid CSS", " after ", ": expected \"{\", was ");
    }
    // create new block and push to the selector stack
    Block* block = SASS_MEMORY_NEW(ctx.mem, Block, pstate, 0, is_root);
    block_stack.push_back(block);

    if (!parse_block_nodes()) css_error("Invalid CSS", " after ", ": expected \"}\", was ");;

    if (!lex_css < exactly<'}'> >()) {
      css_error("Invalid CSS", " after ", ": expected \"}\", was ");
    }

    // update for end position
    block->update_pstate(pstate);

    // parse comments before block
    // lex < optional_css_comments >();

    block_stack.pop_back();

    return block;
  }

  // convenience function for block parsing
  // will create a new block ad-hoc for you
  // also updates the `in_at_root` flag
  Block* Parser::parse_block(bool is_root)
  {
    LOCAL_FLAG(in_at_root, is_root);
    return parse_css_block(is_root);
  }

  // the main block parsing function
  // parses stuff between `{` and `}`
  bool Parser::parse_block_nodes(bool is_root)
  {

    // loop until end of string
    while (position < end) {

      // we should be able to refactor this
      parse_block_comments();
      lex < css_whitespace >();

      if (lex < exactly<';'> >()) continue;
      if (peek < end_of_file >()) return true;
      if (peek < exactly<'}'> >()) return true;

      if (parse_block_node(is_root)) continue;

      parse_block_comments();

      if (lex_css < exactly<';'> >()) continue;
      if (peek_css < end_of_file >()) return true;
      if (peek_css < exactly<'}'> >()) return true;

      // illegal sass
      return false;
    }
    // return success
    return true;
  }

  // parser for a single node in a block
  // semicolons must be lexed beforehand
  bool Parser::parse_block_node(bool is_root) {

    Block* block = block_stack.back();

    while (lex< block_comment >()) {
      bool is_important = lexed.begin[2] == '!';
      String*  contents = parse_interpolated_chunk(lexed);
      (*block) << SASS_MEMORY_NEW(ctx.mem, Comment, pstate, contents, is_important);
    }

    // throw away white-space
    // includes line comments
    lex < css_whitespace >();

    Lookahead lookahead_result;

    // also parse block comments

    // first parse everything that is allowed in functions
    if (lex < variable >(true)) { (*block) << parse_assignment(); }
    else if (lex < kwd_err >(true)) { (*block) << parse_error(); }
    else if (lex < kwd_dbg >(true)) { (*block) << parse_debug(); }
    else if (lex < kwd_warn >(true)) { (*block) << parse_warning(); }
    else if (lex < kwd_if_directive >(true)) { (*block) << parse_if_directive(); }
    else if (lex < kwd_for_directive >(true)) { (*block) << parse_for_directive(); }
    else if (lex < kwd_each_directive >(true)) { (*block) << parse_each_directive(); }
    else if (lex < kwd_while_directive >(true)) { (*block) << parse_while_directive(); }
    else if (lex < kwd_return_directive >(true)) { (*block) << parse_return_directive(); }

    // abort if we are in function context and have nothing parsed yet
    else if (stack.back() == function_def) {
      error("Functions can only contain variable declarations and control directives", pstate);
    }

    // parse imports to process later
    else if (lex < kwd_import >(true)) {
      if (stack.back() == mixin_def || stack.back() == function_def) {
        error("Import directives may not be used within control directives or mixins.", pstate);
      }
      Import* imp = parse_import();
      // if it is a url, we only add the statement
      if (!imp->urls().empty()) (*block) << imp;
      // if it is a file(s), we should process them
      if (!imp->files().empty()) {
        for (size_t i = 0, S = imp->files().size(); i < S; ++i) {
          (*block) << SASS_MEMORY_NEW(ctx.mem, Import_Stub, pstate, imp->files()[i]);
        }
      }
    }

    else if (lex < kwd_extend >(true)) {
      if (block->is_root()) {
        error("Extend directives may only be used within rules.", pstate);
      }

      Lookahead lookahead = lookahead_for_include(position);
      if (!lookahead.found) css_error("Invalid CSS", " after ", ": expected selector, was ");
      Selector* target;
      if (lookahead.has_interpolants) target = parse_selector_schema(lookahead.found);
      else                            target = parse_selector_list(true);
      (*block) << SASS_MEMORY_NEW(ctx.mem, Extension, pstate, target);
    }

    // selector may contain interpolations which need delayed evaluation
    else if (!(lookahead_result = lookahead_for_selector(position)).error)
    { (*block) << parse_ruleset(lookahead_result, is_root); }

    // parse multiple specific keyword directives
    else if (lex < kwd_media >(true)) { (*block) << parse_media_block(); }
    else if (lex < kwd_at_root >(true)) { (*block) << parse_at_root_block(); }
    else if (lex < kwd_include_directive >(true)) { (*block) << parse_include_directive(); }
    else if (lex < kwd_content_directive >(true)) { (*block) << parse_content_directive(); }
    else if (lex < kwd_supports_directive >(true)) { (*block) << parse_supports_directive(); }
    else if (lex < kwd_mixin >(true)) { (*block) << parse_definition(Definition::MIXIN); }
    else if (lex < kwd_function >(true)) { (*block) << parse_definition(Definition::FUNCTION); }

    // ignore the @charset directive for now
    else if (lex< kwd_charset_directive >(true)) { parse_charset_directive(); }

    // generic at keyword (keep last)
    else if (lex< at_keyword >(true)) { (*block) << parse_at_rule(); }

    else if (block->is_root()) {
      lex< css_whitespace >();
      if (position >= end) return true;
      css_error("Invalid CSS", " after ", ": expected 1 selector or at-rule, was ");
    }
    // parse a declaration
    else
    {
      // ToDo: how does it handle parse errors?
      // maybe we are expected to parse something?
      Declaration* decl = parse_declaration();
      decl->tabs(indentation);
      (*block) << decl;
      // maybe we have a "sub-block"
      if (peek< exactly<'{'> >()) {
        if (decl->is_indented()) ++ indentation;
        // parse a propset that rides on the declaration's property
        (*block) << SASS_MEMORY_NEW(ctx.mem, Propset, pstate, decl->property(), parse_block());
        if (decl->is_indented()) -- indentation;
      }
    }
    // something matched
    return true;
  }
  // EO parse_block_nodes

  void Parser::add_single_file (Import* imp, std::string import_path) {

    std::string extension;
    std::string unquoted(unquote(import_path));
    if (unquoted.length() > 4) { // 2 quote marks + the 4 chars in .css
      // a string constant is guaranteed to end with a quote mark, so make sure to skip it when indexing from the end
      extension = unquoted.substr(unquoted.length() - 4, 4);
    }

    if (extension == ".css") {
      String_Constant* loc = SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, unquote(import_path));
      Argument* loc_arg = SASS_MEMORY_NEW(ctx.mem, Argument, pstate, loc);
      Arguments* loc_args = SASS_MEMORY_NEW(ctx.mem, Arguments, pstate);
      (*loc_args) << loc_arg;
      Function_Call* new_url = SASS_MEMORY_NEW(ctx.mem, Function_Call, pstate, "url", loc_args);
      imp->urls().push_back(new_url);
    }
    else {
      std::string current_dir = File::dir_name(path);
      std::string resolved(ctx.add_file(current_dir, unquoted, *this));
      if (resolved.empty()) error("file to import not found or unreadable: " + unquoted + "\nCurrent dir: " + current_dir, pstate);
      imp->files().push_back(resolved);
    }

  }

  void Parser::import_single_file (Import* imp, std::string import_path) {

    if (imp->media_queries() ||
        !unquote(import_path).substr(0, 7).compare("http://") ||
        !unquote(import_path).substr(0, 8).compare("https://") ||
        !unquote(import_path).substr(0, 2).compare("//"))
    {
      imp->urls().push_back(SASS_MEMORY_NEW(ctx.mem, String_Quoted, pstate, import_path));
    }
    else {
      add_single_file(imp, import_path);
    }

  }

  bool Parser::do_import(const std::string& import_path, Import* imp, std::vector<Sass_Importer_Entry> importers, bool only_one)
  {
    size_t i = 0;
    bool has_import = false;
    std::string load_path = unquote(import_path);
    // std::cerr << "-- " << load_path << "\n";
    for (Sass_Importer_Entry& importer : importers) {
      // int priority = sass_importer_get_priority(importer);
      Sass_Importer_Fn fn = sass_importer_get_function(importer);
      if (Sass_Import_List includes =
          fn(load_path.c_str(), importer, ctx.c_compiler)
      ) {
        Sass_Import_List list = includes;
        while (*includes) { ++i;
          std::string uniq_path = load_path;
          if (!only_one && i) {
            std::stringstream pathstrm;
            pathstrm << uniq_path << ":" << i;
            uniq_path = pathstrm.str();
          }
          Sass_Import_Entry include = *includes;
          const char *abs_path = sass_import_get_abs_path(include);
          char* source = sass_import_take_source(include);
          size_t line = sass_import_get_error_line(include);
          size_t column = sass_import_get_error_column(include);
          const char* message = sass_import_get_error_message(include);
          if (message) {
            if (line == std::string::npos && column == std::string::npos) error(message, pstate);
            else error(message, ParserState(message, source, Position(line, column)));
          } else if (source) {
            if (abs_path) {
              ctx.add_source(uniq_path, abs_path, source);
              imp->files().push_back(uniq_path);
              size_t i = ctx.queue.size() - 1;
              ctx.process_queue_entry(ctx.queue[i], i);
            } else {
              ctx.add_source(uniq_path, uniq_path, source);
              imp->files().push_back(uniq_path);
              size_t i = ctx.queue.size() - 1;
              ctx.process_queue_entry(ctx.queue[i], i);
            }
          } else if(abs_path) {
            import_single_file(imp, abs_path);
          }
          ++includes;
        }
        // deallocate returned memory
        sass_delete_import_list(list);
        // set success flag
        has_import = true;
        // break import chain
        if (only_one) return true;
      }
    }
    // return result
    return has_import;
  }

  Import* Parser::parse_import()
  {
    Import* imp = SASS_MEMORY_NEW(ctx.mem, Import, pstate);
    std::vector<std::pair<std::string,Function_Call*>> to_import;
    bool first = true;
    do {
      while (lex< block_comment >());
      if (lex< quoted_string >()) {
        if (!do_import(lexed, imp, ctx.c_importers, true))
        {
          // push single file import
          // import_single_file(imp, lexed);
          to_import.push_back(std::pair<std::string,Function_Call*>(std::string(lexed), 0));
        }
      }
      else if (lex< uri_prefix >()) {
        Arguments* args = SASS_MEMORY_NEW(ctx.mem, Arguments, pstate);
        Function_Call* result = SASS_MEMORY_NEW(ctx.mem, Function_Call, pstate, "url", args);
        if (lex< quoted_string >()) {
          Expression* the_url = parse_string();
          *args << SASS_MEMORY_NEW(ctx.mem, Argument, the_url->pstate(), the_url);
        }
        else if (lex < uri_value >(false)) { // don't skip comments
          String* the_url = parse_interpolated_chunk(lexed);
          *args << SASS_MEMORY_NEW(ctx.mem, Argument, the_url->pstate(), the_url);
        }
        else if (peek < skip_over_scopes < exactly < '(' >, exactly < ')' > > >(position)) {
          Expression* the_url = parse_list(); // parse_interpolated_chunk(lexed);
          *args << SASS_MEMORY_NEW(ctx.mem, Argument, the_url->pstate(), the_url);
        }
        else {
          error("malformed URL", pstate);
        }
        if (!lex< exactly<')'> >()) error("URI is missing ')'", pstate);
        // imp->urls().push_back(result);
        to_import.push_back(std::pair<std::string,Function_Call*>("", result));
      }
      else {
        if (first) error("@import directive requires a url or quoted path", pstate);
        else error("expecting another url or quoted path in @import list", pstate);
      }
      first = false;
    } while (lex_css< exactly<','> >());

    if (!peek_css<alternatives<exactly<';'>,end_of_file>>()) {
      List* media_queries = parse_media_queries();
      imp->media_queries(media_queries);
    }

    for(auto location : to_import) {
      if (location.second) {
        imp->urls().push_back(location.second);
      } else {
        import_single_file(imp, location.first);
      }
    }

    return imp;
  }

  Definition* Parser::parse_definition(Definition::Type which_type)
  {
    std::string which_str(lexed);
    if (!lex< identifier >()) error("invalid name in " + which_str + " definition", pstate);
    std::string name(Util::normalize_underscores(lexed));
    if (which_type == Definition::FUNCTION && (name == "and" || name == "or" || name == "not"))
    { error("Invalid function name \"" + name + "\".", pstate); }
    ParserState source_position_of_def = pstate;
    Parameters* params = parse_parameters();
    if (which_type == Definition::MIXIN) stack.push_back(mixin_def);
    else stack.push_back(function_def);
    Block* body = parse_block();
    stack.pop_back();
    Definition* def = SASS_MEMORY_NEW(ctx.mem, Definition, source_position_of_def, name, params, body, which_type);
    return def;
  }

  Parameters* Parser::parse_parameters()
  {
    std::string name(lexed);
    Position position = after_token;
    Parameters* params = SASS_MEMORY_NEW(ctx.mem, Parameters, pstate);
    if (lex_css< exactly<'('> >()) {
      // if there's anything there at all
      if (!peek_css< exactly<')'> >()) {
        do (*params) << parse_parameter();
        while (lex_css< exactly<','> >());
      }
      if (!lex_css< exactly<')'> >()) error("expected a variable name (e.g. $x) or ')' for the parameter list for " + name, position);
    }
    return params;
  }

  Parameter* Parser::parse_parameter()
  {
    while (lex< alternatives < spaces, block_comment > >());
    lex < variable >();
    std::string name(Util::normalize_underscores(lexed));
    ParserState pos = pstate;
    Expression* val = 0;
    bool is_rest = false;
    while (lex< alternatives < spaces, block_comment > >());
    if (lex< exactly<':'> >()) { // there's a default value
      while (lex< block_comment >());
      val = parse_space_list();
      val->is_delayed(false);
    }
    else if (lex< exactly< ellipsis > >()) {
      is_rest = true;
    }
    Parameter* p = SASS_MEMORY_NEW(ctx.mem, Parameter, pos, name, val, is_rest);
    return p;
  }

  Arguments* Parser::parse_arguments(bool has_url)
  {
    std::string name(lexed);
    Position position = after_token;
    Arguments* args = SASS_MEMORY_NEW(ctx.mem, Arguments, pstate);
    if (lex_css< exactly<'('> >()) {
      // if there's anything there at all
      if (!peek_css< exactly<')'> >()) {
        do (*args) << parse_argument(has_url);
        while (lex_css< exactly<','> >());
      }
      if (!lex_css< exactly<')'> >()) error("expected a variable name (e.g. $x) or ')' for the parameter list for " + name, position);
    }
    return args;
  }

  Argument* Parser::parse_argument(bool has_url)
  {

    Argument* arg;
    // some urls can look like line comments (parse literally - chunk would not work)
    if (has_url && lex< sequence < uri_value, lookahead < loosely<')'> > > >(false)) {
      String* the_url = parse_interpolated_chunk(lexed);
      arg = SASS_MEMORY_NEW(ctx.mem, Argument, the_url->pstate(), the_url);
    }
    else if (peek_css< sequence < variable, optional_css_comments, exactly<':'> > >()) {
      lex_css< variable >();
      std::string name(Util::normalize_underscores(lexed));
      ParserState p = pstate;
      lex_css< exactly<':'> >();
      Expression* val = parse_space_list();
      val->is_delayed(false);
      arg = SASS_MEMORY_NEW(ctx.mem, Argument, p, val, name);
    }
    else {
      bool is_arglist = false;
      bool is_keyword = false;
      Expression* val = parse_space_list();
      val->is_delayed(false);
      if (lex_css< exactly< ellipsis > >()) {
        if (val->concrete_type() == Expression::MAP) is_keyword = true;
        else is_arglist = true;
      }
      arg = SASS_MEMORY_NEW(ctx.mem, Argument, pstate, val, "", is_arglist, is_keyword);
    }
    return arg;
  }

  Assignment* Parser::parse_assignment()
  {
    std::string name(Util::normalize_underscores(lexed));
    ParserState var_source_position = pstate;
    if (!lex< exactly<':'> >()) error("expected ':' after " + name + " in assignment statement", pstate);
    Expression* val;
    Lookahead lookahead = lookahead_for_value(position);
    if (lookahead.has_interpolants && lookahead.found) {
      val = parse_value_schema(lookahead.found);
    } else {
      val = parse_list();
    }
    val->is_delayed(false);
    bool is_default = false;
    bool is_global = false;
    while (peek< alternatives < default_flag, global_flag > >()) {
      if (lex< default_flag >()) is_default = true;
      else if (lex< global_flag >()) is_global = true;
    }
    Assignment* var = SASS_MEMORY_NEW(ctx.mem, Assignment, var_source_position, name, val, is_default, is_global);
    return var;
  }

  // a ruleset connects a selector and a block
  Ruleset* Parser::parse_ruleset(Lookahead lookahead, bool is_root)
  {
    // make sure to move up the the last position
    lex < optional_css_whitespace >(false, true);
    // create the connector object (add parts later)
    Ruleset* ruleset = SASS_MEMORY_NEW(ctx.mem, Ruleset, pstate);
    // parse selector static or as schema to be evaluated later
    if (lookahead.parsable) ruleset->selector(parse_selector_list(is_root));
    else ruleset->selector(parse_selector_schema(lookahead.found));
    // then parse the inner block
    ruleset->block(parse_block());
    // update for end position
    ruleset->update_pstate(pstate);
    // inherit is_root from parent block
    // need this info for sanity checks
    ruleset->is_root(is_root);
    // return AST Node
    return ruleset;
  }

  // parse a selector schema that will be evaluated in the eval stage
  // uses a string schema internally to do the actual schema handling
  // in the eval stage we will be re-parse it into an actual selector
  Selector_Schema* Parser::parse_selector_schema(const char* end_of_selector)
  {
    // move up to the start
    lex< optional_spaces >();
    const char* i = position;
    // selector schema re-uses string schema implementation
    String_Schema* schema = SASS_MEMORY_NEW(ctx.mem, String_Schema, pstate);
    // the selector schema is pretty much just a wrapper for the string schema
    Selector_Schema* selector_schema = SASS_MEMORY_NEW(ctx.mem, Selector_Schema, pstate, schema);

    // process until end
    while (i < end_of_selector) {
      // try to parse mutliple interpolants
      if (const char* p = find_first_in_interval< exactly<hash_lbrace> >(i, end_of_selector)) {
        // accumulate the preceding segment if the position has advanced
        if (i < p) (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, std::string(i, p));
        // check if the interpolation only contains white-space (error out)
        if (peek < sequence < optional_spaces, exactly<rbrace> > >(p+2)) { position = p+2;
          css_error("Invalid CSS", " after ", ": expected expression (e.g. 1px, bold), was ");
        }
        // skip over all nested inner interpolations up to our own delimiter
        const char* j = skip_over_scopes< exactly<hash_lbrace>, exactly<rbrace> >(p + 2, end_of_selector);
        // pass inner expression to the parser to resolve nested interpolations
        Expression* interpolant = Parser::from_c_str(p+2, j, ctx, pstate).parse_list();
        // set status on the list expression
        interpolant->is_interpolant(true);
        // add to the string schema
        (*schema) << interpolant;
        // advance position
        i = j;
      }
      // no more interpolants have been found
      // add the last segment if there is one
      else {
        // make sure to add the last bits of the string up to the end (if any)
        if (i < end_of_selector) (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, std::string(i, end_of_selector));
        // exit loop
        i = end_of_selector;
      }
    }
    // EO until eos

    // update position
    position = i;

    // update for end position
    selector_schema->update_pstate(pstate);

    // return parsed result
    return selector_schema;
  }
  // EO parse_selector_schema

  void Parser::parse_charset_directive()
  {
    lex <
      sequence <
        quoted_string,
        optional_spaces,
        exactly <';'>
      >
    >();
  }

  // called after parsing `kwd_include_directive`
  Mixin_Call* Parser::parse_include_directive()
  {
    // lex identifier into `lexed` var
    lex_identifier(); // may error out
    // normalize underscores to hyphens
    std::string name(Util::normalize_underscores(lexed));
    // create the initial mixin call object
    Mixin_Call* call = SASS_MEMORY_NEW(ctx.mem, Mixin_Call, pstate, name, 0, 0);
    // parse mandatory arguments
    call->arguments(parse_arguments());
    // parse optional block
    if (peek < exactly <'{'> >()) {
      call->block(parse_block());
    }
    // return ast node
    return call;
  }
  // EO parse_include_directive

  // parse a list of complex selectors
  // this is the main entry point for most
  Selector_List* Parser::parse_selector_list(bool in_root)
  {
    bool reloop = true;
    bool had_linefeed = false;
    Complex_Selector* sel = 0;
    To_String to_string(&ctx);
    Selector_List* group = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate);

    do {
      reloop = false;

      had_linefeed = had_linefeed || peek_newline();

      if (peek_css< class_char < selector_list_delims > >())
        break; // in case there are superfluous commas at the end


      // now parse the complex selector
      sel = parse_complex_selector(in_root);

      if (!sel) return group;

      sel->has_line_feed(had_linefeed);

      had_linefeed = false;

      while (peek_css< exactly<','> >())
      {
        lex< css_comments >(false);
        // consume everything up and including the comma speparator
        reloop = lex< exactly<','> >() != 0;
        // remember line break (also between some commas)
        had_linefeed = had_linefeed || peek_newline();
        // remember line break (also between some commas)
      }
      (*group) << sel;
    }
    while (reloop);
    while (lex_css< kwd_optional >()) {
      group->is_optional(true);
    }
    // update for end position
    group->update_pstate(pstate);
    if (sel) sel->last()->has_line_break(false);
    return group;
  }
  // EO parse_selector_list

  // a complex selector combines a compound selector with another
  // complex selector, with one of four combinator operations.
  // the compound selector (head) is optional, since the combinator
  // can come first in the whole selector sequence (like `> DIV').
  Complex_Selector* Parser::parse_complex_selector(bool in_root)
  {

    String* reference = 0;
    lex < block_comment >();
    // parse the left hand side
    Compound_Selector* lhs = 0;
    // special case if it starts with combinator ([+~>])
    if (!peek_css< class_char < selector_combinator_ops > >()) {
      // parse the left hand side
      lhs = parse_compound_selector();
    }

    // check for end of file condition
    if (peek < end_of_file >()) return 0;

    // parse combinator between lhs and rhs
    Complex_Selector::Combinator combinator;
    if      (lex< exactly<'+'> >()) combinator = Complex_Selector::ADJACENT_TO;
    else if (lex< exactly<'~'> >()) combinator = Complex_Selector::PRECEDES;
    else if (lex< exactly<'>'> >()) combinator = Complex_Selector::PARENT_OF;
    else if (lex< sequence < exactly<'/'>, negate < exactly < '*' > > > >()) {
      // comments are allowed, but not spaces?
      combinator = Complex_Selector::REFERENCE;
      if (!lex < re_reference_combinator >()) return 0;
      reference = SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed);
      if (!lex < exactly < '/' > >()) return 0; // ToDo: error msg?
    }
    else /* if (lex< zero >()) */   combinator = Complex_Selector::ANCESTOR_OF;

    if (!lhs && combinator == Complex_Selector::ANCESTOR_OF) return 0;

    // lex < block_comment >();
    // source position of a complex selector points to the combinator
    // ToDo: make sure we update pstate for ancestor of (lex < zero >());
    Complex_Selector* sel = SASS_MEMORY_NEW(ctx.mem, Complex_Selector, pstate, combinator, lhs);
    if (combinator == Complex_Selector::REFERENCE) sel->reference(reference);
    // has linfeed after combinator?
    sel->has_line_break(peek_newline());
    // sel->has_line_feed(has_line_feed);

    // check if we got the abort condition (ToDo: optimize)
    if (!peek_css< class_char < complex_selector_delims > >()) {
      // parse next selector in sequence
      sel->tail(parse_complex_selector(true));
      if (sel->tail()) {
        // ToDo: move this logic below into tail setter
        if (sel->tail()->has_reference()) sel->has_reference(true);
        if (sel->tail()->has_placeholder()) sel->has_placeholder(true);
      }
    }

    // add a parent selector if we are not in a root
    // also skip adding parent ref if we only have refs
    if (!sel->has_reference() && !in_at_root && !in_root) {
      // create the objects to wrap parent selector reference
      Parent_Selector* parent = SASS_MEMORY_NEW(ctx.mem, Parent_Selector, pstate);
      Compound_Selector* head = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, pstate);
      // add simple selector
      (*head) << parent;
      // selector may not have any head yet
      if (!sel->head()) { sel->head(head); }
      // otherwise we need to create a new complex selector and set the old one as its tail
      else { sel = SASS_MEMORY_NEW(ctx.mem, Complex_Selector, pstate, Complex_Selector::ANCESTOR_OF, head, sel); }
      // peek for linefeed and remember result on head
      // if (peek_newline()) head->has_line_break(true);
    }

    // complex selector
    return sel;
  }
  // EO parse_complex_selector

  // parse one compound selector, which is basically
  // a list of simple selectors (directly adjancent)
  // lex them exactly (without skipping white-space)
  Compound_Selector* Parser::parse_compound_selector()
  {
    // init an empty compound selector wrapper
    Compound_Selector* seq = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, pstate);

    // skip initial white-space
    lex< css_whitespace >();

    // parse list
    while (true)
    {
      // remove all block comments (don't skip white-space)
      lex< delimited_by< slash_star, star_slash, false > >(false);
      // parse functional
      if (peek < re_pseudo_selector >())
      {
        (*seq) << parse_simple_selector();
      }
      // parse parent selector
      else if (lex< exactly<'&'> >(false))
      {
        // this produces a linefeed!?
        seq->has_parent_reference(true);
        (*seq) << SASS_MEMORY_NEW(ctx.mem, Parent_Selector, pstate);
      }
      // parse type selector
      else if (lex< re_type_selector >(false))
      {
        (*seq) << SASS_MEMORY_NEW(ctx.mem, Type_Selector, pstate, lexed);
      }
      // peek for abort conditions
      else if (peek< spaces >()) break;
      else if (peek< end_of_file >()) { break; }
      else if (peek_css < class_char < selector_combinator_ops > >()) break;
      else if (peek_css < class_char < complex_selector_delims > >()) break;
      // otherwise parse another simple selector
      else {
        Simple_Selector* sel = parse_simple_selector();
        if (!sel) return 0;
        (*seq) << sel;
      }
    }

    if (seq && !peek_css<exactly<'{'>>()) {
      seq->has_line_break(peek_newline());
    }

    // EO while true
    return seq;

  }
  // EO parse_compound_selector

  Simple_Selector* Parser::parse_simple_selector()
  {
    lex < css_comments >(false);
    if (lex< alternatives < id_name, class_name > >()) {
      return SASS_MEMORY_NEW(ctx.mem, Selector_Qualifier, pstate, lexed);
    }
    else if (lex< quoted_string >()) {
      return SASS_MEMORY_NEW(ctx.mem, Type_Selector, pstate, unquote(lexed));
    }
    else if (lex< alternatives < variable, number, static_reference_combinator > >()) {
      return SASS_MEMORY_NEW(ctx.mem, Type_Selector, pstate, lexed);
    }
    else if (peek< pseudo_not >()) {
      return parse_negated_selector();
    }
    else if (peek< re_pseudo_selector >()) {
      return parse_pseudo_selector();
    }
    else if (peek< exactly<':'> >()) {
      return parse_pseudo_selector();
    }
    else if (lex < exactly<'['> >()) {
      return parse_attribute_selector();
    }
    else if (lex< placeholder >()) {
      return SASS_MEMORY_NEW(ctx.mem, Selector_Placeholder, pstate, lexed);
    }
    // failed
    return 0;
  }

  Wrapped_Selector* Parser::parse_negated_selector()
  {
    lex< pseudo_not >();
    std::string name(lexed);
    ParserState nsource_position = pstate;
    Selector* negated = parse_selector_list(true);
    if (!lex< exactly<')'> >()) {
      error("negated selector is missing ')'", pstate);
    }
    name.erase(name.size() - 1);
    return SASS_MEMORY_NEW(ctx.mem, Wrapped_Selector, nsource_position, name, negated);
  }

  // a pseudo selector often starts with one or two colons
  // it can contain more selectors inside parantheses
  Simple_Selector* Parser::parse_pseudo_selector() {
    if (lex< sequence<
          optional < pseudo_prefix >,
          // we keep the space within the name, strange enough
          // ToDo: refactor output to schedule the space for it
          // or do we really want to keep the real white-space?
          sequence< identifier, optional < block_comment >, exactly<'('> >
        > >())
    {

      std::string name(lexed);
      name.erase(name.size() - 1);
      ParserState p = pstate;

      // specially parse static stuff
      // ToDo: really everything static?
      if (peek_css <
            sequence <
              alternatives <
                static_value,
                binomial
              >,
              optional_css_whitespace,
              exactly<')'>
            >
          >()
      ) {
        lex_css< alternatives < static_value, binomial > >();
        String_Constant* expr = SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed);
        if (expr && lex_css< exactly<')'> >()) {
          expr->can_compress_whitespace(true);
          return SASS_MEMORY_NEW(ctx.mem, Pseudo_Selector, p, name, expr);
        }
      }
      else if (Selector* wrapped = parse_selector_list(true)) {
        if (wrapped && lex_css< exactly<')'> >()) {
          return SASS_MEMORY_NEW(ctx.mem, Wrapped_Selector, p, name, wrapped);
        }
      }

    }
    // EO if pseudo selector

    else if (lex < sequence< optional < pseudo_prefix >, identifier > >()) {
      return SASS_MEMORY_NEW(ctx.mem, Pseudo_Selector, pstate, lexed);
    }
    else if(lex < pseudo_prefix >()) {
      css_error("Invalid CSS", " after ", ": expected pseudoclass or pseudoelement, was ");
    }

    css_error("Invalid CSS", " after ", ": expected \")\", was ");

    // unreachable statement
    return 0;
  }

  Attribute_Selector* Parser::parse_attribute_selector()
  {
    ParserState p = pstate;
    if (!lex_css< attribute_name >()) error("invalid attribute name in attribute selector", pstate);
    std::string name(lexed);
    if (lex_css< alternatives < exactly<']'>, exactly<'/'> > >()) return SASS_MEMORY_NEW(ctx.mem, Attribute_Selector, p, name, "", 0);
    if (!lex_css< alternatives< exact_match, class_match, dash_match,
                                prefix_match, suffix_match, substring_match > >()) {
      error("invalid operator in attribute selector for " + name, pstate);
    }
    std::string matcher(lexed);

    String* value = 0;
    if (lex_css< identifier >()) {
      value = SASS_MEMORY_NEW(ctx.mem, String_Constant, p, lexed);
    }
    else if (lex_css< quoted_string >()) {
      value = parse_interpolated_chunk(lexed, true); // needed!
    }
    else {
      error("expected a string constant or identifier in attribute selector for " + name, pstate);
    }

    if (!lex_css< alternatives < exactly<']'>, exactly<'/'> > >()) error("unterminated attribute selector for " + name, pstate);
    return SASS_MEMORY_NEW(ctx.mem, Attribute_Selector, p, name, matcher, value);
  }

  /* parse block comment and add to block */
  void Parser::parse_block_comments()
  {
    Block* block = block_stack.back();
    while (lex< block_comment >()) {
      bool is_important = lexed.begin[2] == '!';
      String*  contents = parse_interpolated_chunk(lexed);
      (*block) << SASS_MEMORY_NEW(ctx.mem, Comment, pstate, contents, is_important);
    }
  }

  Declaration* Parser::parse_declaration() {
    String* prop = 0;
    if (lex< sequence< optional< exactly<'*'> >, identifier_schema > >()) {
      prop = parse_identifier_schema();
    }
    else if (lex< sequence< optional< exactly<'*'> >, identifier, zero_plus< block_comment > > >()) {
      prop = SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed);
      prop->is_delayed(true);
    }
    else {
      css_error("Invalid CSS", " after ", ": expected \"}\", was ");
    }
    bool is_indented = true;
    const std::string property(lexed);
    if (!lex_css< one_plus< exactly<':'> > >()) error("property \"" + property + "\" must be followed by a ':'", pstate);
    lex < css_comments >(false);
    if (peek_css< exactly<';'> >()) error("style declaration must contain a value", pstate);
    if (peek_css< exactly<'{'> >()) is_indented = false; // don't indent if value is empty
    if (peek_css< static_value >()) {
      return SASS_MEMORY_NEW(ctx.mem, Declaration, prop->pstate(), prop, parse_static_value()/*, lex<kwd_important>()*/);
    }
    else {
      Expression* value;
      Lookahead lookahead = lookahead_for_value(position);
      if (lookahead.found) {
        if (lookahead.has_interpolants) {
          value = parse_value_schema(lookahead.found);
        } else {
          value = parse_list();
        }
      }
      else {
        value = parse_list();
        if (List* list = dynamic_cast<List*>(value)) {
          if (list->length() == 0 && !peek< exactly <'{'> >()) {
            css_error("Invalid CSS", " after ", ": expected expression (e.g. 1px, bold), was ");
          }
        }
      }
      lex < css_comments >(false);
      auto decl = SASS_MEMORY_NEW(ctx.mem, Declaration, prop->pstate(), prop, value/*, lex<kwd_important>()*/);
      decl->is_indented(is_indented);
      return decl;
    }
  }

  // parse +/- and return false if negative
  bool Parser::parse_number_prefix()
  {
    bool positive = true;
    while(true) {
      if (lex < block_comment >()) continue;
      if (lex < number_prefix >()) continue;
      if (lex < exactly < '-' > >()) {
        positive = !positive;
        continue;
      }
      break;
    }
    return positive;
  }

  Expression* Parser::parse_map()
  {
    Expression* key = parse_list();
    Map* map = SASS_MEMORY_NEW(ctx.mem, Map, pstate, 1);
    if (String_Quoted* str = dynamic_cast<String_Quoted*>(key)) {
      if (!str->quote_mark() && !str->is_delayed()) {
        if (const Color* col = name_to_color(str->value())) {
          Color* c = SASS_MEMORY_NEW(ctx.mem, Color, *col);
          c->pstate(str->pstate());
          c->disp(str->value());
          key = c;
        }
      }
    }

    // it's not a map so return the lexed value as a list value
    if (!peek< exactly<':'> >())
    { return key; }

    lex< exactly<':'> >();

    Expression* value = parse_space_list();

    (*map) << std::make_pair(key, value);

    while (lex_css< exactly<','> >())
    {
      // allow trailing commas - #495
      if (peek_css< exactly<')'> >(position))
      { break; }

      Expression* key = parse_list();
      if (String_Quoted* str = dynamic_cast<String_Quoted*>(key)) {
        if (!str->quote_mark() && !str->is_delayed()) {
          if (const Color* col = name_to_color(str->value())) {
            Color* c = SASS_MEMORY_NEW(ctx.mem, Color, *col);
            c->pstate(str->pstate());
            c->disp(str->value());
            key = c;
          }
        }
      }

      if (!(lex< exactly<':'> >()))
      { error("invalid syntax", pstate); }

      Expression* value = parse_space_list();

      (*map) << std::make_pair(key, value);
    }

    ParserState ps = map->pstate();
    ps.offset = pstate - ps + pstate.offset;
    map->pstate(ps);

    return map;
  }

  // parse list returns either a space separated list,
  // a comma separated list or any bare expression found.
  // so to speak: we unwrap items from lists if possible here!
  Expression* Parser::parse_list()
  {
    // parse list is relly just an alias
    return parse_comma_list();
  }

  // will return singletons unwrapped
  Expression* Parser::parse_comma_list()
  {
    // check if we have an empty list
    // return the empty list as such
    if (peek_css< alternatives <
          // exactly<'!'>,
          exactly<';'>,
          exactly<'}'>,
          exactly<'{'>,
          exactly<')'>,
          exactly<':'>,
          exactly<ellipsis>,
          default_flag,
          global_flag
        > >(position))
    { return SASS_MEMORY_NEW(ctx.mem, List, pstate, 0); }

    // now try to parse a space list
    Expression* list = parse_space_list();
    // if it's a singleton, return it (don't wrap it)
    if (!peek_css< exactly<','> >(position)) return list;

    // if we got so far, we actually do have a comma list
    List* comma_list = SASS_MEMORY_NEW(ctx.mem, List, pstate, 2, SASS_COMMA);
    // wrap the first expression
    (*comma_list) << list;

    while (lex_css< exactly<','> >())
    {
      // check for abort condition
      if (peek_css< alternatives <
            exactly<';'>,
            exactly<'}'>,
            exactly<'{'>,
            exactly<')'>,
            exactly<':'>,
            exactly<ellipsis>,
            default_flag,
            global_flag
          > >(position)
      ) { break; }
      // otherwise add another expression
      (*comma_list) << parse_space_list();
    }
    // return the list
    return comma_list;
  }
  // EO parse_comma_list

  // will return singletons unwrapped
  Expression* Parser::parse_space_list()
  {
    Expression* disj1 = parse_disjunction();
    // if it's a singleton, return it (don't wrap it)
    if (peek_css< alternatives <
          // exactly<'!'>,
          exactly<';'>,
          exactly<'}'>,
          exactly<'{'>,
          exactly<')'>,
          exactly<','>,
          exactly<':'>,
          exactly<ellipsis>,
          default_flag,
          global_flag
        > >(position)
    ) { return disj1; }

    List* space_list = SASS_MEMORY_NEW(ctx.mem, List, pstate, 2, SASS_SPACE);
    (*space_list) << disj1;

    while (!(peek_css< alternatives <
               // exactly<'!'>,
               exactly<';'>,
               exactly<'}'>,
               exactly<'{'>,
               exactly<')'>,
               exactly<','>,
               exactly<':'>,
               exactly<ellipsis>,
               default_flag,
               global_flag
           > >(position)) && peek_css< optional_css_whitespace >() != end
    ) {
      // the space is parsed implicitly?
      (*space_list) << parse_disjunction();
    }
    // return the list
    return space_list;
  }
  // EO parse_space_list

  // parse logical OR operation
  Expression* Parser::parse_disjunction()
  {
    // parse the left hand side conjunction
    Expression* conj = parse_conjunction();
    // parse multiple right hand sides
    std::vector<Expression*> operands;
    while (lex_css< kwd_or >())
      operands.push_back(parse_conjunction());
    // if it's a singleton, return it directly
    if (operands.size() == 0) return conj;
    // fold all operands into one binary expression
    return fold_operands(conj, operands, Sass_OP::OR);
  }
  // EO parse_disjunction

  // parse logical AND operation
  Expression* Parser::parse_conjunction()
  {
    // parse the left hand side relation
    Expression* rel = parse_relation();
    // parse multiple right hand sides
    std::vector<Expression*> operands;
    while (lex_css< kwd_and >())
      operands.push_back(parse_relation());
    // if it's a singleton, return it directly
    if (operands.size() == 0) return rel;
    // fold all operands into one binary expression
    return fold_operands(rel, operands, Sass_OP::AND);
  }
  // EO parse_conjunction

  // parse comparison operations
  Expression* Parser::parse_relation()
  {
    // parse the left hand side expression
    Expression* lhs = parse_expression();
    // if it's a singleton, return it (don't wrap it)
    if (!(peek< alternatives <
            kwd_eq,
            kwd_neq,
            kwd_gte,
            kwd_gt,
            kwd_lte,
            kwd_lt
          > >(position)))
    { return lhs; }
    // parse the operator
    enum Sass_OP op
    = lex<kwd_eq>()  ? Sass_OP::EQ
    : lex<kwd_neq>() ? Sass_OP::NEQ
    : lex<kwd_gte>() ? Sass_OP::GTE
    : lex<kwd_lte>() ? Sass_OP::LTE
    : lex<kwd_gt>()  ? Sass_OP::GT
    : lex<kwd_lt>()  ? Sass_OP::LT
    // we checked the possibilites on top of fn
    :                  Sass_OP::EQ;
    // parse the right hand side expression
    Expression* rhs = parse_expression();
    // return binary expression with a left and a right hand side
    return SASS_MEMORY_NEW(ctx.mem, Binary_Expression, lhs->pstate(), op, lhs, rhs);
  }
  // parse_relation

  // parse expression valid for operations
  // called from parse_relation
  // called from parse_for_directive
  // called from parse_media_expression
  // parse addition and subtraction operations
  Expression* Parser::parse_expression()
  {
    Expression* lhs = parse_operators();
    // if it's a singleton, return it (don't wrap it)
    if (!(peek< exactly<'+'> >(position) ||
          // condition is a bit misterious, but some combinations should not be counted as operations
          (peek< no_spaces >(position) && peek< sequence< negate< unsigned_number >, exactly<'-'>, negate< space > > >(position)) ||
          (peek< sequence< negate< unsigned_number >, exactly<'-'>, negate< unsigned_number > > >(position))) ||
          peek< identifier >(position))
    { return lhs; }

    std::vector<Expression*> operands;
    std::vector<Sass_OP> operators;
    while (lex< exactly<'+'> >() || lex< sequence< negate< digit >, exactly<'-'> > >()) {
      operators.push_back(lexed.to_string() == "+" ? Sass_OP::ADD : Sass_OP::SUB);
      operands.push_back(parse_operators());
    }

    if (operands.size() == 0) return lhs;
    return fold_operands(lhs, operands, operators);
  }

  // parse addition and subtraction operations
  Expression* Parser::parse_operators()
  {
    Expression* factor = parse_factor();
    // Special case: Ruby sass never tries to modulo if the lhs contains an interpolant
    if (peek_css< exactly<'%'> >() && factor->concrete_type() == Expression::STRING) {
      String_Schema* ss = dynamic_cast<String_Schema*>(factor);
      if (ss && ss->has_interpolants()) return factor;
    }
    // if it's a singleton, return it (don't wrap it)
    if (!peek_css< class_char< static_ops > >()) return factor;
    // parse more factors and operators
    std::vector<Expression*> operands; // factors
    std::vector<enum Sass_OP> operators; // ops
    // lex operations to apply to lhs
    while (lex_css< class_char< static_ops > >()) {
      switch(*lexed.begin) {
        case '*': operators.push_back(Sass_OP::MUL); break;
        case '/': operators.push_back(Sass_OP::DIV); break;
        case '%': operators.push_back(Sass_OP::MOD); break;
        default: throw std::runtime_error("unknown static op parsed"); break;
      }
      operands.push_back(parse_factor());
    }
    // operands and operators to binary expression
    return fold_operands(factor, operands, operators);
  }
  // EO parse_operators


  // called from parse_operators
  // called from parse_value_schema
  Expression* Parser::parse_factor()
  {
    lex < css_comments >(false);
    if (lex_css< exactly<'('> >()) {
      // parse_map may return a list
      Expression* value = parse_map();
      // lex the expected closing parenthesis
      if (!lex_css< exactly<')'> >()) error("unclosed parenthesis", pstate);
      // expression can be evaluated
      value->is_delayed(false);
      // make sure wrapped lists and division expressions are non-delayed within parentheses
      if (value->concrete_type() == Expression::LIST) {
        List* l = static_cast<List*>(value);
        if (!l->empty()) (*l)[0]->is_delayed(false);
      } else if (typeid(*value) == typeid(Binary_Expression)) {
        Binary_Expression* b = static_cast<Binary_Expression*>(value);
        Binary_Expression* lhs = static_cast<Binary_Expression*>(b->left());
        if (lhs && lhs->type() == Sass_OP::DIV) lhs->is_delayed(false);
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
             peek< exactly< ms_calc_kwd > >() ||
             peek< exactly< webkit_calc_kwd > >()) {
      return parse_calc_function();
    }
    else if (lex < functional_schema >()) {
      return parse_function_call_schema();
    }
    else if (lex< identifier_schema >()) {
      String* string = parse_identifier_schema();
      if (String_Schema* schema = dynamic_cast<String_Schema*>(string)) {
        if (lex < exactly < '(' > >()) {
          *schema << parse_list();
          lex < exactly < ')' > >();
        }
      }
      return string;
    }
    else if (peek< re_functional >()) {
      return parse_function_call();
    }
    else if (lex< exactly<'+'> >()) {
      return SASS_MEMORY_NEW(ctx.mem, Unary_Expression, pstate, Unary_Expression::PLUS, parse_factor());
    }
    else if (lex< exactly<'-'> >()) {
      return SASS_MEMORY_NEW(ctx.mem, Unary_Expression, pstate, Unary_Expression::MINUS, parse_factor());
    }
    else if (lex< sequence< kwd_not > >()) {
      return SASS_MEMORY_NEW(ctx.mem, Unary_Expression, pstate, Unary_Expression::NOT, parse_factor());
    }
    else if (peek < sequence < one_plus < alternatives < css_whitespace, exactly<'-'>, exactly<'+'> > >, number > >()) {
      if (parse_number_prefix()) return parse_value(); // prefix is positive
      return SASS_MEMORY_NEW(ctx.mem, Unary_Expression, pstate, Unary_Expression::MINUS, parse_value());
    }
    else {
      return parse_value();
    }
  }

  // parse one value for a list
  Expression* Parser::parse_value()
  {
    lex< css_comments >(false);
    if (lex< ampersand >())
    {
      return SASS_MEMORY_NEW(ctx.mem, Parent_Selector, pstate); }

    if (lex< kwd_important >())
    { return SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, "!important"); }

    if (const char* stop = peek< value_schema >())
    { return parse_value_schema(stop); }

    // string may be interpolated
    if (lex< quoted_string >())
    { return parse_string(); }

    if (lex< kwd_true >())
    { return SASS_MEMORY_NEW(ctx.mem, Boolean, pstate, true); }

    if (lex< kwd_false >())
    { return SASS_MEMORY_NEW(ctx.mem, Boolean, pstate, false); }

    if (lex< kwd_null >())
    { return SASS_MEMORY_NEW(ctx.mem, Null, pstate); }

    if (lex< identifier >()) {
      return SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed);
    }

    if (lex< percentage >())
    { return SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::PERCENTAGE, lexed); }

    // match hex number first because 0x000 looks like a number followed by an indentifier
    if (lex< sequence < alternatives< hex, hex0 >, negate < exactly<'-'> > > >())
    { return SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::HEX, lexed); }

    if (lex< sequence < exactly <'#'>, identifier > >())
    { return SASS_MEMORY_NEW(ctx.mem, String_Quoted, pstate, lexed); }

    // also handle the 10em- foo special case
    if (lex< sequence< dimension, optional< sequence< exactly<'-'>, negate< digit > > > > >())
    { return SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::DIMENSION, lexed); }

    if (lex< sequence< static_component, one_plus< strict_identifier > > >())
    { return SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed); }

    if (lex< number >())
    { return SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::NUMBER, lexed); }

    if (lex< variable >())
    { return SASS_MEMORY_NEW(ctx.mem, Variable, pstate, Util::normalize_underscores(lexed)); }

    // Special case handling for `%` proceeding an interpolant.
    if (lex< sequence< exactly<'%'>, optional< percentage > > >())
    { return SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed); }

    error("error reading values after " + lexed.to_string(), pstate);

    // unreachable statement
    return 0;
  }

  // this parses interpolation inside other strings
  // means the result should later be quoted again
  String* Parser::parse_interpolated_chunk(Token chunk, bool constant)
  {
    const char* i = chunk.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< exactly<hash_lbrace> >(i, chunk.end);
    if (!p) {
      String_Quoted* str_quoted = SASS_MEMORY_NEW(ctx.mem, String_Quoted, pstate, std::string(i, chunk.end));
      if (!constant && str_quoted->quote_mark()) str_quoted->quote_mark('*');
      str_quoted->is_delayed(true);
      return str_quoted;
    }

    String_Schema* schema = SASS_MEMORY_NEW(ctx.mem, String_Schema, pstate);
    while (i < chunk.end) {
      p = find_first_in_interval< exactly<hash_lbrace> >(i, chunk.end);
      if (p) {
        if (i < p) {
          // accumulate the preceding segment if it's nonempty
          (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, std::string(i, p));
        }
        // we need to skip anything inside strings
        // create a new target in parser/prelexer
        if (peek < sequence < optional_spaces, exactly<rbrace> > >(p+2)) { position = p+2;
          css_error("Invalid CSS", " after ", ": expected expression (e.g. 1px, bold), was ");
        }
        const char* j = skip_over_scopes< exactly<hash_lbrace>, exactly<rbrace> >(p + 2, chunk.end); // find the closing brace
        if (j) { --j;
          // parse the interpolant and accumulate it
          Expression* interp_node = Parser::from_token(Token(p+2, j), ctx, pstate).parse_list();
          interp_node->is_interpolant(true);
          (*schema) << interp_node;
          i = j;
        }
        else {
          // throw an error if the interpolant is unterminated
          error("unterminated interpolant inside string constant " + chunk.to_string(), pstate);
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        // check if we need quotes here (was not sure after merge)
        if (i < chunk.end) (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, std::string(i, chunk.end));
        break;
      }
      ++ i;
    }

    return schema;
  }

  String_Constant* Parser::parse_static_expression()
  {
    if (peek< sequence< number, optional_spaces, exactly<'/'>, optional_spaces, number > >()) {
      return parse_static_value();
    }
    return 0;
  }

  String_Constant* Parser::parse_static_value()
  {
    lex< static_value >();
    Token str(lexed);
    --str.end;
    --position;

    String_Constant* str_node = SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, str.time_wspace());
    str_node->is_delayed(true);
    return str_node;
  }

  String* Parser::parse_string()
  {
    return parse_interpolated_chunk(Token(lexed));
  }

  String* Parser::parse_ie_property()
  {
    lex< ie_property >();
    Token str(lexed);
    const char* i = str.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< exactly<hash_lbrace> >(str.begin, str.end);
    if (!p) {
      return SASS_MEMORY_NEW(ctx.mem, String_Quoted, pstate, std::string(str.begin, str.end));
    }

    String_Schema* schema = SASS_MEMORY_NEW(ctx.mem, String_Schema, pstate);
    while (i < str.end) {
      p = find_first_in_interval< exactly<hash_lbrace> >(i, str.end);
      if (p) {
        if (i < p) {
          (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, std::string(i, p)); // accumulate the preceding segment if it's nonempty
        }
        if (peek < sequence < optional_spaces, exactly<rbrace> > >(p+2)) { position = p+2;
          css_error("Invalid CSS", " after ", ": expected expression (e.g. 1px, bold), was ");
        }
        const char* j = skip_over_scopes< exactly<hash_lbrace>, exactly<rbrace> >(p+2, str.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          Expression* interp_node = Parser::from_token(Token(p+2, j), ctx, pstate).parse_list();
          interp_node->is_interpolant(true);
          (*schema) << interp_node;
          i = j;
        }
        else {
          // throw an error if the interpolant is unterminated
          error("unterminated interpolant inside IE function " + str.to_string(), pstate);
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < str.end) {
          (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, std::string(i, str.end));
        }
        break;
      }
    }
    return schema;
  }

  String* Parser::parse_ie_keyword_arg()
  {
    String_Schema* kwd_arg = SASS_MEMORY_NEW(ctx.mem, String_Schema, pstate, 3);
    if (lex< variable >()) {
      *kwd_arg << SASS_MEMORY_NEW(ctx.mem, Variable, pstate, Util::normalize_underscores(lexed));
    } else {
      lex< alternatives< identifier_schema, identifier > >();
      *kwd_arg << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed);
    }
    lex< exactly<'='> >();
    *kwd_arg << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed);
    if (peek< variable >()) *kwd_arg << parse_list();
    else if (lex< number >()) *kwd_arg << SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::NUMBER, Util::normalize_decimals(lexed));
    else if (peek < ie_keyword_arg_value >()) { *kwd_arg << parse_list(); }
    return kwd_arg;
  }

  String_Schema* Parser::parse_value_schema(const char* stop)
  {
    // initialize the string schema object to add tokens
    String_Schema* schema = SASS_MEMORY_NEW(ctx.mem, String_Schema, pstate);

    if (peek<exactly<'}'>>()) {
      css_error("Invalid CSS", " after ", ": expected expression (e.g. 1px, bold), was ");
    }

    const char* e = 0;
    size_t num_items = 0;
    while (position < stop) {
      // parse space between tokens
      if (lex< spaces >() && num_items) {
        (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, " ");
      }
      if ((e = peek< re_functional >()) && e < stop) {
        (*schema) << parse_function_call();
      }
      // lex an interpolant /#{...}/
      else if (lex< exactly < hash_lbrace > >()) {
        // Try to lex static expression first
        if (lex< re_static_expression >()) {
          (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed);
        } else {
          (*schema) << parse_list();
        }
        // ToDo: no error check here?
        lex < exactly < rbrace > >();
      }
      // lex some string constants
      else if (lex< alternatives < exactly<'%'>, exactly < '-' >, identifier > >()) {
        (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, lexed);
        if (*position == '"' || *position == '\'') {
          (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, " ");
        }
      }
      // lex a quoted string
      else if (lex< quoted_string >()) {
        (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Quoted, pstate, lexed, '"');
        if (*position == '"' || *position == '\'' || alpha(position)) {
          (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, " ");
        }
      }
      // lex (normalized) variable
      else if (lex< variable >()) {
        std::string name(Util::normalize_underscores(lexed));
        (*schema) << SASS_MEMORY_NEW(ctx.mem, Variable, pstate, name);
      }
      // lex percentage value
      else if (lex< percentage >()) {
        (*schema) << SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::PERCENTAGE, lexed);
      }
      // lex dimension value
      else if (lex< dimension >()) {
        (*schema) << SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::DIMENSION, lexed);
      }
      // lex number value
      else if (lex< number >()) {
        (*schema) <<  SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::NUMBER, lexed);
      }
      // lex hex color value
      else if (lex< sequence < hex, negate < exactly < '-' > > > >()) {
        (*schema) << SASS_MEMORY_NEW(ctx.mem, Textual, pstate, Textual::HEX, lexed);
      }
      else if (lex< sequence < exactly <'#'>, identifier > >()) {
        (*schema) << SASS_MEMORY_NEW(ctx.mem, String_Quoted, pstate, lexed);
      }
      // lex a value in parentheses
      else if (peek< parenthese_scope >()) {
        (*schema) << parse_factor();
      }
      else {
        return schema;
      }
      ++num_items;
    }
    return schema;
  }

  // this parses interpolation outside other strings
  // means the result must not be quoted again later
  String* Parser::parse_identifier_schema()
  {
    Token id(lexed);
    const char* i = id.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< exactly<hash_lbrace> >(id.begin, id.end);
    if (!p) {
      return SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, std::string(id.begin, id.end));
    }

    String_Schema* schema = SASS_MEMORY_NEW(ctx.mem, String_Schema, pstate);
    while (i < id.end) {
      p = find_first_in_interval< exactly<hash_lbrace> >(i, id.end);
      if (p) {
        if (i < p) {
          // accumulate the preceding segment if it's nonempty
          const char* o = position; position = i;
          *schema << parse_value_schema(p);
          position = o;
        }
        // we need to skip anything inside strings
        // create a new target in parser/prelexer
        if (peek < sequence < optional_spaces, exactly<rbrace> > >(p+2)) { position = p+2;
          css_error("Invalid CSS", " after ", ": expected expression (e.g. 1px, bold), was ");
        }
        const char* j = skip_over_scopes< exactly<hash_lbrace>, exactly<rbrace> >(p+2, id.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          Expression* interp_node = Parser::from_token(Token(p+2, j), ctx, pstate).parse_list();
          interp_node->is_interpolant(true);
          (*schema) << interp_node;
          schema->has_interpolants(true);
          i = j;
        }
        else {
          // throw an error if the interpolant is unterminated
          error("unterminated interpolant inside interpolated identifier " + id.to_string(), pstate);
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < end) {
          const char* o = position; position = i;
          *schema << parse_value_schema(id.end);
          position = o;
        }
        break;
      }
    }
    return schema;
  }

  // calc functions should preserve arguments
  Function_Call* Parser::parse_calc_function()
  {
    lex< identifier >();
    std::string name(lexed);
    ParserState call_pos = pstate;
    lex< exactly<'('> >();
    ParserState arg_pos = pstate;
    const char* arg_beg = position;
    parse_list();
    const char* arg_end = position;
    lex< skip_over_scopes <
          exactly < '(' >,
          exactly < ')' >
        > >();

    Argument* arg = SASS_MEMORY_NEW(ctx.mem, Argument, arg_pos, parse_interpolated_chunk(Token(arg_beg, arg_end)));
    Arguments* args = SASS_MEMORY_NEW(ctx.mem, Arguments, arg_pos);
    *args << arg;
    return SASS_MEMORY_NEW(ctx.mem, Function_Call, call_pos, name, args);
  }

  Function_Call* Parser::parse_function_call()
  {
    lex< identifier >();
    std::string name(lexed);

    ParserState call_pos = pstate;
    bool expect_url = name == "url" || name == "url-prefix";
    Arguments* args = parse_arguments(expect_url);
    return SASS_MEMORY_NEW(ctx.mem, Function_Call, call_pos, name, args);
  }

  Function_Call_Schema* Parser::parse_function_call_schema()
  {
    String* name = parse_identifier_schema();
    ParserState source_position_of_call = pstate;

    Function_Call_Schema* the_call = SASS_MEMORY_NEW(ctx.mem, Function_Call_Schema, source_position_of_call, name, parse_arguments());
    return the_call;
  }

  Content* Parser::parse_content_directive()
  {
    if (stack.back() != mixin_def) {
      error("@content may only be used within a mixin", pstate);
    }
    return SASS_MEMORY_NEW(ctx.mem, Content, pstate);
  }

  If* Parser::parse_if_directive(bool else_if)
  {
    ParserState if_source_position = pstate;
    Expression* predicate = parse_list();
    predicate->is_delayed(false);
    Block* block = parse_block();
    Block* alternative = 0;

    if (lex< elseif_directive >()) {
      alternative = SASS_MEMORY_NEW(ctx.mem, Block, pstate);
      (*alternative) << parse_if_directive(true);
    }
    else if (lex< kwd_else_directive >()) {
      alternative = parse_block();
    }
    return SASS_MEMORY_NEW(ctx.mem, If, if_source_position, predicate, block, alternative);
  }

  For* Parser::parse_for_directive()
  {
    ParserState for_source_position = pstate;
    lex_variable();
    std::string var(Util::normalize_underscores(lexed));
    if (!lex< kwd_from >()) error("expected 'from' keyword in @for directive", pstate);
    Expression* lower_bound = parse_expression();
    lower_bound->is_delayed(false);
    bool inclusive = false;
    if (lex< kwd_through >()) inclusive = true;
    else if (lex< kwd_to >()) inclusive = false;
    else                  error("expected 'through' or 'to' keyword in @for directive", pstate);
    Expression* upper_bound = parse_expression();
    upper_bound->is_delayed(false);
    Block* body = parse_block();
    return SASS_MEMORY_NEW(ctx.mem, For, for_source_position, var, lower_bound, upper_bound, body, inclusive);
  }

  // helper to parse a var token
  Token Parser::lex_variable()
  {
    // peek for dollar sign first
    if (!peek< exactly <'$'> >()) {
      css_error("Invalid CSS", " after ", ": expected \"$\", was ");
    }
    // we expect a simple identfier as the call name
    if (!lex< sequence < exactly <'$'>, identifier > >()) {
      lex< exactly <'$'> >(); // move pstate and position up
      css_error("Invalid CSS", " after ", ": expected identifier, was ");
    }
    // return object
    return token;
  }
  // helper to parse identifier
  Token Parser::lex_identifier()
  {
    // we expect a simple identfier as the call name
    if (!lex< identifier >()) { // ToDo: pstate wrong?
      css_error("Invalid CSS", " after ", ": expected identifier, was ");
    }
    // return object
    return token;
  }

  Each* Parser::parse_each_directive()
  {
    ParserState each_source_position = pstate;
    std::vector<std::string> vars;
    lex_variable();
    vars.push_back(Util::normalize_underscores(lexed));
    while (lex< exactly<','> >()) {
      if (!lex< variable >()) error("@each directive requires an iteration variable", pstate);
      vars.push_back(Util::normalize_underscores(lexed));
    }
    if (!lex< kwd_in >()) error("expected 'in' keyword in @each directive", pstate);
    Expression* list = parse_list();
    list->is_delayed(false);
    if (list->concrete_type() == Expression::LIST) {
      List* l = static_cast<List*>(list);
      for (size_t i = 0, L = l->length(); i < L; ++i) {
        (*l)[i]->is_delayed(false);
      }
    }
    Block* body = parse_block();
    return SASS_MEMORY_NEW(ctx.mem, Each, each_source_position, vars, list, body);
  }

  // called after parsing `kwd_while_directive`
  While* Parser::parse_while_directive()
  {
    // create the initial while call object
    While* call = SASS_MEMORY_NEW(ctx.mem, While, pstate, 0, 0);
    // parse mandatory predicate
    Expression* predicate = parse_list();
    predicate->is_delayed(false);
    call->predicate(predicate);
    // parse mandatory block
    call->block(parse_block());
    // return ast node
    return call;
  }

  // EO parse_while_directive
  Media_Block* Parser::parse_media_block()
  {
    Media_Block* media_block = SASS_MEMORY_NEW(ctx.mem, Media_Block, pstate, 0, 0);
    media_block->media_queries(parse_media_queries());

    media_block->block(parse_css_block());

    return media_block;
  }

  List* Parser::parse_media_queries()
  {
    List* media_queries = SASS_MEMORY_NEW(ctx.mem, List, pstate, 0, SASS_COMMA);
    if (!peek< exactly<'{'> >()) (*media_queries) << parse_media_query();
    while (lex< exactly<','> >()) (*media_queries) << parse_media_query();
    return media_queries;
  }

  // Expression* Parser::parse_media_query()
  Media_Query* Parser::parse_media_query()
  {
    Media_Query* media_query = SASS_MEMORY_NEW(ctx.mem, Media_Query, pstate);

    if (lex< exactly< not_kwd > >()) media_query->is_negated(true);
    else if (lex< exactly< only_kwd > >()) media_query->is_restricted(true);

    if (lex < identifier_schema >()) media_query->media_type(parse_identifier_schema());
    else if (lex< identifier >())    media_query->media_type(parse_interpolated_chunk(lexed));
    else                             (*media_query) << parse_media_expression();

    while (lex< exactly< and_kwd > >()) (*media_query) << parse_media_expression();
    if (lex < identifier_schema >()) {
      String_Schema* schema = SASS_MEMORY_NEW(ctx.mem, String_Schema, pstate);
      *schema << media_query->media_type();
      *schema << SASS_MEMORY_NEW(ctx.mem, String_Constant, pstate, " ");
      *schema << parse_identifier_schema();
      media_query->media_type(schema);
    }
    while (lex< exactly< and_kwd > >()) (*media_query) << parse_media_expression();
    return media_query;
  }

  Media_Query_Expression* Parser::parse_media_expression()
  {
    if (lex < identifier_schema >()) {
      String* ss = parse_identifier_schema();
      return SASS_MEMORY_NEW(ctx.mem, Media_Query_Expression, pstate, ss, 0, true);
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
    return SASS_MEMORY_NEW(ctx.mem, Media_Query_Expression, feature->pstate(), feature, expression);
  }

  // lexed after `kwd_supports_directive`
  // these are very similar to media blocks
  Supports_Block* Parser::parse_supports_directive()
  {
    Supports_Condition* cond = parse_supports_condition();
    // create the ast node object for the support queries
    Supports_Block* query = SASS_MEMORY_NEW(ctx.mem, Supports_Block, pstate, cond);
    // additional block is mandatory
    // parse inner block
    query->block(parse_block());
    // return ast node
    return query;
  }

  // parse one query operation
  // may encounter nested queries
  Supports_Condition* Parser::parse_supports_condition()
  {
    lex < css_whitespace >();
    Supports_Condition* cond = parse_supports_negation();
    if (!cond) cond = parse_supports_operator();
    if (!cond) cond = parse_supports_interpolation();
    return cond;
  }

  Supports_Condition* Parser::parse_supports_negation()
  {
    if (!lex < kwd_not >()) return 0;

    Supports_Condition* cond = parse_supports_condition_in_parens();
    return SASS_MEMORY_NEW(ctx.mem, Supports_Negation, pstate, cond);
  }

  Supports_Condition* Parser::parse_supports_operator()
  {
    Supports_Condition* cond = parse_supports_condition_in_parens();
    if (!cond) return 0;

    while (lex < kwd_and >() || lex < kwd_or >()) {
      Supports_Operator::Operand op = Supports_Operator::OR;
      if (lexed.to_string() == "and") op = Supports_Operator::AND;

      lex < css_whitespace >();
      Supports_Condition* right = parse_supports_condition_in_parens();

      // Supports_Condition* cc = SASS_MEMORY_NEW(ctx.mem, Supports_Condition, *static_cast<Supports_Condition*>(cond));
      cond = SASS_MEMORY_NEW(ctx.mem, Supports_Operator, pstate, cond, right, op);
    }
    return cond;
  }

  Supports_Condition* Parser::parse_supports_interpolation()
  {
    if (!lex < interpolant >()) return 0;

    String* interp = parse_interpolated_chunk(lexed);
    if (!interp) return 0;

    return SASS_MEMORY_NEW(ctx.mem, Supports_Interpolation, pstate, interp);
  }

  // TODO: This needs some major work. Although feature conditions
  // look like declarations their semantics differ siginificantly
  Supports_Condition* Parser::parse_supports_declaration()
  {
    Supports_Condition* cond = 0;
    // parse something declaration like
    Declaration* declaration = parse_declaration();
    if (!declaration) error("@supports condition expected declaration", pstate);
    cond = SASS_MEMORY_NEW(ctx.mem, Supports_Declaration,
                     declaration->pstate(),
                     declaration->property(),
                     declaration->value());
    // ToDo: maybe we need an additional error condition?
    return cond;
  }

  Supports_Condition* Parser::parse_supports_condition_in_parens()
  {
    Supports_Condition* interp = parse_supports_interpolation();
    if (interp != 0) return interp;

    if (!lex < exactly <'('> >()) return 0;
    lex < css_whitespace >();

    Supports_Condition* cond = parse_supports_condition();
    if (cond != 0) {
      if (!lex < exactly <')'> >()) error("unclosed parenthesis in @supports declaration", pstate);
    } else {
      cond = parse_supports_declaration();
      if (!lex < exactly <')'> >()) error("unclosed parenthesis in @supports declaration", pstate);
    }
    lex < css_whitespace >();
    return cond;
  }

  At_Root_Block* Parser::parse_at_root_block()
  {
    ParserState at_source_position = pstate;
    Block* body = 0;
    At_Root_Expression* expr = 0;
    Lookahead lookahead_result;
    LOCAL_FLAG(in_at_root, true);
    if (lex< exactly<'('> >()) {
      expr = parse_at_root_expression();
    }
    if (peek < exactly<'{'> >()) {
      body = parse_block(true);
    }
    else if ((lookahead_result = lookahead_for_selector(position)).found) {
      Ruleset* r = parse_ruleset(lookahead_result, false);
      body = SASS_MEMORY_NEW(ctx.mem, Block, r->pstate(), 1, true);
      *body << r;
    }
    At_Root_Block* at_root = SASS_MEMORY_NEW(ctx.mem, At_Root_Block, at_source_position, body);
    if (expr) at_root->expression(expr);
    return at_root;
  }

  At_Root_Expression* Parser::parse_at_root_expression()
  {
    if (peek< exactly<')'> >()) error("at-root feature required in at-root expression", pstate);

    if (!peek< alternatives< kwd_with_directive, kwd_without_directive > >()) {
      css_error("Invalid CSS", " after ", ": expected \"with\" or \"without\", was ");
    }

    Declaration* declaration = parse_declaration();
    List* value = SASS_MEMORY_NEW(ctx.mem, List, declaration->value()->pstate(), 1);

    if (declaration->value()->concrete_type() == Expression::LIST) {
        value = static_cast<List*>(declaration->value());
    }
    else *value << declaration->value();

    At_Root_Expression* cond = SASS_MEMORY_NEW(ctx.mem, At_Root_Expression,
                                               declaration->pstate(),
                                               declaration->property(),
                                               value);
    if (!lex< exactly<')'> >()) error("unclosed parenthesis in @at-root expression", pstate);
    return cond;
  }

  At_Rule* Parser::parse_at_rule()
  {
    std::string kwd(lexed);
    At_Rule* at_rule = SASS_MEMORY_NEW(ctx.mem, At_Rule, pstate, kwd);
    Lookahead lookahead = lookahead_for_include(position);
    if (lookahead.found && !lookahead.has_interpolants) {
      at_rule->selector(parse_selector_list(true));
    }

    lex < css_comments >(false);

    if (lex < static_property >()) {
      at_rule->value(parse_interpolated_chunk(Token(lexed)));
    } else if (!(peek < alternatives < exactly<'{'>, exactly<'}'>, exactly<';'> > >())) {
      at_rule->value(parse_list());
    }

    lex < css_comments >(false);

    if (peek< exactly<'{'> >()) {
      at_rule->block(parse_block());
    }

    return at_rule;
  }

  Warning* Parser::parse_warning()
  {
    return SASS_MEMORY_NEW(ctx.mem, Warning, pstate, parse_list());
  }

  Error* Parser::parse_error()
  {
    return SASS_MEMORY_NEW(ctx.mem, Error, pstate, parse_list());
  }

  Debug* Parser::parse_debug()
  {
    return SASS_MEMORY_NEW(ctx.mem, Debug, pstate, parse_list());
  }

  Return* Parser::parse_return_directive()
  {
    // check that we do not have an empty list (ToDo: check if we got all cases)
    if (peek_css < alternatives < exactly < ';' >, exactly < '}' >, end_of_file > >())
    { css_error("Invalid CSS", " after ", ": expected expression (e.g. 1px, bold), was "); }
    return SASS_MEMORY_NEW(ctx.mem, Return, pstate, parse_list());
  }

  Lookahead Parser::lookahead_for_selector(const char* start)
  {
    // init result struct
    Lookahead rv = Lookahead();
    // get start position
    const char* p = start ? start : position;
    // match in one big "regex"
    rv.error = p;
    if (const char* q =
      peek <
        one_plus <
          alternatives <
            // consume whitespace and comments
            spaces, block_comment, line_comment,
            // match `/deep/` selector (pass-trough)
            // there is no functionality for it yet
            schema_reference_combinator,
            // match selector ops /[*&%,()\[\]]/
            class_char < selector_lookahead_ops >,
            // match selector combinators /[>+~]/
            class_char < selector_combinator_ops >,
            // match attribute compare operators
            alternatives <
              exact_match, class_match, dash_match,
              prefix_match, suffix_match, substring_match
            >,
            // main selector match
            sequence <
              // allow namespace prefix
              optional < namespace_schema >,
              // modifiers prefixes
              alternatives <
                sequence <
                  exactly <'#'>,
                  // not for interpolation
                  negate < exactly <'{'> >
                >,
                // class match
                exactly <'.'>,
                // single or double colon
                optional < pseudo_prefix >
              >,
              // accept hypens in token
              one_plus < sequence <
                // can start with hyphens
                zero_plus < exactly<'-'> >,
                // now the main token
                alternatives <
                  kwd_optional,
                  exactly <'*'>,
                  quoted_string,
                  interpolant,
                  identifier,
                  percentage,
                  dimension,
                  variable,
                  alnum
                >
              > >,
              // can also end with hyphens
              zero_plus < exactly<'-'> >
            >
          >
        >
      >(p)
    ) {
      while (p < q) {
        // did we have interpolations?
        if (*p == '#' && *(p+1) == '{') {
          rv.has_interpolants = true;
          p = q; break;
        }
        ++ p;
      }
      // store anyway  }


      // ToDo: remove
      rv.error = q;
      rv.position = q;
      // check expected opening bracket
      // only after successfull matching
      if (peek < exactly<'{'> >(q)) rv.found = q;
      // else if (peek < exactly<';'> >(q)) rv.found = q;
      // else if (peek < exactly<'}'> >(q)) rv.found = q;
      if (rv.found || *p == 0) rv.error = 0;
    }

    rv.parsable = ! rv.has_interpolants;

    // return result
    return rv;

  }
  // EO lookahead_for_selector

  // used in parse_block_nodes and parse_at_rule
  // ToDo: actual usage is still not really clear to me?
  Lookahead Parser::lookahead_for_include(const char* start)
  {
    // we actually just lookahead for a selector
    Lookahead rv = lookahead_for_selector(start);
    // but the "found" rules are different
    if (const char* p = rv.position) {
      // check for additional abort condition
      if (peek < exactly<';'> >(p)) rv.found = p;
      else if (peek < exactly<'}'> >(p)) rv.found = p;
    }
    // return result
    return rv;
  }
  // EO lookahead_for_include

  // look ahead for a token with interpolation in it
  // we mostly use the result if there is an interpolation
  // everything that passes here gets parsed as one schema
  // meaning it will not be parsed as a space separated list
  Lookahead Parser::lookahead_for_value(const char* start)
  {
    // init result struct
    Lookahead rv = Lookahead();
    // get start position
    const char* p = start ? start : position;
    // match in one big "regex"
    if (const char* q =
      peek <
        one_plus <
          alternatives <
            // consume whitespace
            block_comment, spaces,
            // main tokens
            interpolant,
            identifier,
            variable,
            // issue #442
            sequence <
              parenthese_scope,
              interpolant
            >
          >
        >
      >(p)
    ) {
      while (p < q) {
        // did we have interpolations?
        if (*p == '#' && *(p+1) == '{') {
          rv.has_interpolants = true;
          p = q; break;
        }
        ++ p;
      }
      // store anyway
      // ToDo: remove
      rv.position = q;
      // check expected opening bracket
      // only after successfull matching
      if (peek < exactly<'{'> >(q)) rv.found = q;
      else if (peek < exactly<';'> >(q)) rv.found = q;
      else if (peek < exactly<'}'> >(q)) rv.found = q;
    }

    // return result
    return rv;
  }
  // EO lookahead_for_value

  void Parser::read_bom()
  {
    size_t skip = 0;
    std::string encoding;
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


  Expression* Parser::fold_operands(Expression* base, std::vector<Expression*>& operands, enum Sass_OP op)
  {
    for (size_t i = 0, S = operands.size(); i < S; ++i) {
      base = SASS_MEMORY_NEW(ctx.mem, Binary_Expression, pstate, op, base, operands[i]);
      Binary_Expression* b = static_cast<Binary_Expression*>(base);
      if (op == Sass_OP::DIV && b->left()->is_delayed() && b->right()->is_delayed()) {
        base->is_delayed(true);
      }
      else {
        b->left()->is_delayed(false);
        b->right()->is_delayed(false);
      }
    }
    return base;
  }

  Expression* Parser::fold_operands(Expression* base, std::vector<Expression*>& operands, std::vector<enum Sass_OP>& ops)
  {
    for (size_t i = 0, S = operands.size(); i < S; ++i) {
      base = SASS_MEMORY_NEW(ctx.mem, Binary_Expression, base->pstate(), ops[i], base, operands[i]);
      Binary_Expression* b = static_cast<Binary_Expression*>(base);
      if (ops[i] == Sass_OP::DIV && b->left()->is_delayed() && b->right()->is_delayed()) {
        base->is_delayed(true);
      }
      else {
        b->left()->is_delayed(false);
        b->right()->is_delayed(false);
      }
    }
    return base;
  }

  void Parser::error(std::string msg, Position pos)
  {
    throw Error_Invalid(Error_Invalid::syntax, ParserState(path, source, pos.line ? pos : before_token, Offset(0, 0)), msg);
  }

  // print a css parsing error with actual context information from parsed source
  void Parser::css_error(const std::string& msg, const std::string& prefix, const std::string& middle)
  {
    int max_len = 14;
    const char* pos = peek < optional_spaces >();

    const char* last_pos(pos - 1);
    // backup position to last significant char
    while ((!*last_pos || Prelexer::is_space(*last_pos)) && last_pos > source) -- last_pos;

    bool ellipsis_left = false;
    const char* pos_left(last_pos + 1);
    const char* end_left(last_pos + 1);
    while (pos_left > source) {
      if (end_left - pos_left > max_len) {
        ellipsis_left = true;
        break;
      }

      const char* prev = pos_left - 1;
      if (*prev == '\r') break;
      if (*prev == '\n') break;
      pos_left = prev;
    }
    if (pos_left < source) {
      pos_left = source;
    }

    bool ellipsis_right = false;
    const char* end_right(pos);
    const char* pos_right(pos);
    while (end_right <= end) {
      if (end_right - pos_right > max_len) {
        ellipsis_right = true;
        break;
      }
      if (*end_right == '\r') break;
      if (*end_right == '\n') break;
      ++ end_right;
    }
    if (end_right > end) end_right = end;

    std::string left(pos_left, end_left);
    std::string right(pos_right, end_right);
    if (ellipsis_left) left = ellipsis + left;
    if (ellipsis_right) right = right + ellipsis;
    // now pass new message to the more generic error function
    error(msg + prefix + quote(left) + middle + quote(right), pstate);
  }

}
