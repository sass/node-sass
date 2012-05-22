#include <map>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

#include "context.hpp"

namespace Sass {
  using std::string;
  using std::vector;
  using std::map;
  using namespace Prelexer;

  struct Document {
    enum CSS_Style { nested, expanded, compact, compressed, echo };
    
    string path;
    char* source;
    const char* position;
    const char* end;
    size_t line_number;
    bool own_source;

    Context& context;
    
    Node root;
    Token lexed;

  private:
    // force the use of the "make_from_..." factory funtions
    Document(Context& ctx);
    ~Document();
  public:

    static Document make_from_file(Context& ctx, string path);
    static Document make_from_source_chars(Context& ctx, char* src, string path = "");
    static Document make_from_token(Context& ctx, Token t, string path = "", size_t line_number = 1);

    template <prelexer mx>
    const char* peek(const char* start = 0)
    {
      if (!start) start = position;
      const char* after_whitespace;
      if (mx == block_comment) {
        after_whitespace = // start;
          zero_plus< alternatives<spaces, line_comment> >(start);
      }
      else if (/*mx == ancestor_of ||*/ mx == no_spaces) {
        after_whitespace = position;
      }
      else if (mx == spaces || mx == ancestor_of) {
        after_whitespace = mx(start);
        if (after_whitespace) {
          return after_whitespace;
        }
        else {
          return 0;
        }
      }
      else if (mx == optional_spaces) {
        after_whitespace = optional_spaces(start);
      }
      else {
        after_whitespace = spaces_and_comments(start);
      }
      const char* after_token = mx(after_whitespace);
      if (after_token) {
        return after_token;
      }
      else {
        return 0;
      }
    }
    
    template <prelexer mx>
    const char* lex()
    {
      const char* after_whitespace;
      if (mx == block_comment) {
        after_whitespace = // position;
          zero_plus< alternatives<spaces, line_comment> >(position);
      }
      else if (mx == ancestor_of || mx == no_spaces) {
        after_whitespace = position;
      }
      else if (mx == spaces) {
        after_whitespace = spaces(position);
        if (after_whitespace) {
          line_number += count_interval<'\n'>(position, after_whitespace);
          lexed = Token::make(position, after_whitespace);
          return position = after_whitespace;
        }
        else {
          return 0;
        }
      }
      else if (mx == optional_spaces) {
        after_whitespace = optional_spaces(position);
      }
      else {
        after_whitespace = spaces_and_comments(position);
      }
      const char* after_token = mx(after_whitespace);
      if (after_token) {
        line_number += count_interval<'\n'>(position, after_token);
        lexed = Token::make(after_whitespace, after_token);
        return position = after_token;
      }
      else {
        return 0;
      }
    }
    
    void parse_scss();
    Node parse_import();
    Node parse_include();
    Node parse_mixin_definition();
    Node parse_mixin_parameters();
    Node parse_parameter();
    Node parse_mixin_call();
    Node parse_arguments();
    Node parse_argument();
    Node parse_assignment();
    Node parse_propset();
    Node parse_ruleset(bool definition = false);
    Node parse_selector_group();
    Node parse_selector();
    Node parse_selector_combinator();
    Node parse_simple_selector_sequence();
    Node parse_simple_selector();
    Node parse_pseudo();
    Node parse_attribute_selector();
    Node parse_block(bool definition = false);
    Node parse_rule();
    Node parse_values();
    Node parse_list();
    Node parse_comma_list();
    Node parse_space_list();
    Node parse_disjunction();
    Node parse_conjunction();
    Node parse_relation();
    Node parse_expression();
    Node parse_term();
    Node parse_factor();
    Node parse_value();
    Node parse_identifier();
    Node parse_variable();
    Node parse_function_call();
    Node parse_string();
    Node parse_value_schema();
    
    const char* lookahead_for_selector(const char* start = 0);
    
    const char* look_for_rule(const char* start = 0);
    const char* look_for_values(const char* start = 0);
    
    const char* look_for_selector_group(const char* start = 0);
    const char* look_for_selector(const char* start = 0);
    const char* look_for_simple_selector_sequence(const char* start = 0);
    const char* look_for_simple_selector(const char* start = 0);
    const char* look_for_pseudo(const char* start = 0);
    const char* look_for_attrib(const char* start = 0);
    
    void syntax_error(string message, size_t ln = 0);
    void read_error(string message, size_t ln = 0);
    
    string emit_css(CSS_Style style);

  };
}