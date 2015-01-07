#ifndef SASS_PARSER_H
#define SASS_PARSER_H

#include <map>
#include <vector>
#include <iostream>

#include "ast.hpp"
#include "token.hpp"
#include "context.hpp"
#include "position.hpp"
#include "prelexer.hpp"

struct Selector_Lookahead {
  const char* found;
  bool has_interpolants;
};

namespace Sass {
  using std::string;
  using std::vector;
  using std::map;
  using namespace Prelexer;

  class Parser : public ParserState {
  private:
    void add_single_file (Import* imp, string import_path);
  public:
    class AST_Node;

    enum Syntactic_Context { nothing, mixin_def, function_def };

    Context& ctx;
    vector<Syntactic_Context> stack;
    const char* source;
    const char* position;
    const char* end;
    Position before_token;
    Position after_token;
    ParserState pstate;


    Token lexed;
    bool dequote;
    bool in_at_root;

    Parser(Context& ctx, ParserState pstate)
    : ParserState(pstate), ctx(ctx), stack(vector<Syntactic_Context>()),
      source(0), position(0), end(0), before_token(pstate), after_token(pstate), pstate("[NULL]")
    { dequote = false; in_at_root = false; stack.push_back(nothing); }

    static Parser from_string(string src, Context& ctx, ParserState pstate = ParserState("[STRING]"));
    static Parser from_c_str(const char* src, Context& ctx, ParserState pstate = ParserState("[CSTRING]"));
    static Parser from_token(Token t, Context& ctx, ParserState pstate = ParserState("[TOKEN]"));

#ifdef __clang__

    // lex and peak uses the template parameter to branch on the action, which
    // triggers clangs tautological comparison on the single-comparison
    // branches. This is not a bug, just a merging of behaviour into
    // one function

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"

#endif

    template <prelexer mx>
    const char* peek(const char* start = 0)
    {
      if (!start) start = position;
      const char* it_before_token;
      if (mx == block_comment) {
        it_before_token = // start;
          zero_plus< alternatives<spaces, line_comment> >(start);
      }
      else if (/*mx == ancestor_of ||*/ mx == no_spaces) {
        it_before_token = position;
      }
      else if (mx == spaces || mx == ancestor_of) {
        it_before_token = mx(start);
        if (it_before_token) {
          return it_before_token;
        }
        else {
          return 0;
        }
      }
      else if (mx == optional_spaces) {
        it_before_token = optional_spaces(start);
      }
      else if (mx == line_comment_prefix || mx == block_comment_prefix) {
        it_before_token = position;
      }
      else {
        it_before_token = spaces_and_comments(start);
      }
      const char* it_after_token = mx(it_before_token);
      if (it_after_token) {
        return it_after_token;
      }
      else {
        return 0;
      }
    }

    // white-space handling is built into the lexer
    // this way you do not need to parse it yourself
    // some matchers don't accept certain white-space
    template <prelexer mx>
    const char* lex()
    {

      // advance position for next call
      before_token = after_token;

      // after optional whitespace
      const char* it_before_token;

      if (mx == block_comment) {
        // a block comment can be preceded by spaces and/or line comments
        it_before_token = zero_plus< alternatives<spaces, line_comment> >(position);
      }
      else if (mx == url || mx == ancestor_of || mx == no_spaces) {
        // parse everything literally
        it_before_token = position;
      }

      else if (mx == spaces) {
        it_before_token = spaces(position);
        if (it_before_token) {
          return position = it_before_token;
        }
        else {
          return 0;
        }
      }

      else if (mx == optional_spaces) {
        // ToDo: what are optiona_spaces ???
        it_before_token = optional_spaces(position);
      }
      else {
        // most can be preceded by spaces and comments
        it_before_token = spaces_and_comments(position);
      }

      // now call matcher to get position after token
      const char* it_after_token = mx(it_before_token);
      // assertion that we got a valid match
      if (it_after_token == 0) return 0;

      // add whitespace after previous and before this token
      while (position < it_before_token && *position) {
        if (*position == '\n') {
          ++ before_token.line;
          before_token.column = 0;
        } else {
          ++ before_token.column;
        }
        ++position;
      }

      // copy position
      after_token = before_token;

      Offset size(0, 0);

      // increase position to include current token
      while (position < it_after_token && *position) {
        if (*position == '\n') {
          ++ size.line;
          size.column = 0;
        } else {
          ++ size.column;
        }
        ++position;
      }

      after_token = after_token + size;

      // create parsed token string (public member)
      lexed = Token(it_before_token, it_after_token, before_token);

      pstate = ParserState(path, Position(before_token.file, before_token.line, before_token.column), size);

      // advance internal char iterator
      return position = it_after_token;

    }

#ifdef __clang__

#pragma clang diagnostic pop

#endif

    void error(string msg, Position pos);
    void read_bom();

    Block* parse();
    Import* parse_import();
    Definition* parse_definition();
    Parameters* parse_parameters();
    Parameter* parse_parameter();
    Mixin_Call* parse_mixin_call();
    Arguments* parse_arguments();
    Argument* parse_argument();
    Assignment* parse_assignment();
    Propset* parse_propset();
    Ruleset* parse_ruleset(Selector_Lookahead lookahead);
    Selector_Schema* parse_selector_schema(const char* end_of_selector);
    Selector_List* parse_selector_group();
    Complex_Selector* parse_selector_combination();
    Compound_Selector* parse_simple_selector_sequence();
    Simple_Selector* parse_simple_selector();
    Wrapped_Selector* parse_negated_selector();
    Simple_Selector* parse_pseudo_selector();
    Attribute_Selector* parse_attribute_selector();
    Block* parse_block();
    Declaration* parse_declaration();
    Expression* parse_map_value();
    Expression* parse_map();
    Expression* parse_list();
    Expression* parse_comma_list();
    Expression* parse_space_list();
    Expression* parse_disjunction();
    Expression* parse_conjunction();
    Expression* parse_relation();
    Expression* parse_expression();
    Expression* parse_term();
    Expression* parse_factor();
    Expression* parse_value();
    Function_Call* parse_calc_function();
    Function_Call* parse_function_call();
    Function_Call_Schema* parse_function_call_schema();
    String* parse_interpolated_chunk(Token);
    String* parse_string();
    String_Constant* parse_static_value();
    String* parse_ie_property();
    String* parse_ie_keyword_arg();
    String_Schema* parse_value_schema();
    String* parse_identifier_schema();
    String_Schema* parse_url_schema();
    If* parse_if_directive(bool else_if = false);
    For* parse_for_directive();
    Each* parse_each_directive();
    While* parse_while_directive();
    Media_Block* parse_media_block();
    List* parse_media_queries();
    Media_Query* parse_media_query();
    Media_Query_Expression* parse_media_expression();
    Feature_Block* parse_feature_block();
    Feature_Query* parse_feature_queries();
    Feature_Query_Condition* parse_feature_query();
    Feature_Query_Condition* parse_feature_query_in_parens();
    Feature_Query_Condition* parse_supports_negation();
    Feature_Query_Condition* parse_supports_conjunction();
    Feature_Query_Condition* parse_supports_disjunction();
    Feature_Query_Condition* parse_supports_declaration();
    At_Root_Block* parse_at_root_block();
    At_Root_Expression* parse_at_root_expression();
    At_Rule* parse_at_rule();
    Warning* parse_warning();
    Error* parse_error();
    Debug* parse_debug();

    Selector_Lookahead lookahead_for_selector(const char* start = 0);
    Selector_Lookahead lookahead_for_extension_target(const char* start = 0);

    Expression* fold_operands(Expression* base, vector<Expression*>& operands, Binary_Expression::Type op);
    Expression* fold_operands(Expression* base, vector<Expression*>& operands, vector<Binary_Expression::Type>& ops);

    void throw_syntax_error(string message, size_t ln = 0);
    void throw_read_error(string message, size_t ln = 0);
  };

  size_t check_bom_chars(const char* src, const char *end, const unsigned char* bom, size_t len);
}

#endif
