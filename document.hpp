#include <map>
#include "node.hpp"

namespace Sass {
  using std::vector;
  using std::map;
  using namespace Prelexer;

  struct Document {
    enum CSS_Style { nested, expanded, compact, compressed, echo };
    
    char* path;
    char* source;
    char* position;
    size_t line_number;
    
    // TO DO: move the environment up into the context class when it's ready
    map<Token, Node> environment;
    
    vector<Node> statements;
    Token lexed;
    
    Document(char* _path, char* _source = 0);
    ~Document();
    
    template <prelexer mx>
    char* peek(char* start = 0)
    {
      if (!start) start = position;
      char* after_whitespace;
      if (mx == block_comment) {
        after_whitespace =
          zero_plus< alternatives<spaces, line_comment> >(start);
      }
      else if (mx == ancestor_of || mx == no_spaces) {
        after_whitespace = position;
      }
      else if (mx == spaces) {
        after_whitespace = spaces(start);
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
      char* after_token = mx(after_whitespace);
      if (after_token) {
        return after_token;
      }
      else {
        return 0;
      }
    }
    
    template <prelexer mx>
    char* lex()
    {
      char* after_whitespace;
      if (mx == block_comment) {
        after_whitespace =
          zero_plus< alternatives<spaces, line_comment> >(position);
      }
      else if (mx == ancestor_of || mx == no_spaces) {
        after_whitespace = position;
      }
      else if (mx == spaces) {
        after_whitespace = spaces(position);
        if (after_whitespace) {
          line_number += count_interval<'\n'>(position, after_whitespace);
          lexed = Token(position, after_whitespace);
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
      char* after_token = mx(after_whitespace);
      if (after_token) {
        line_number += count_interval<'\n'>(position, after_token);
        lexed = Token(after_whitespace, after_token);
        return position = after_token;
      }
      else {
        return 0;
      }
    }
    
    void parse_scss();
    void parse_var_def();
    Node parse_ruleset();
    Node parse_selector_group();
    Node parse_selector();
    Node parse_simple_selector_sequence();
    Node parse_simple_selector();
    Node parse_attribute_selector();
    Node parse_block();
    Node parse_rule();
    Node parse_values();
    
    char* look_for_rule(char* start = 0);
    char* look_for_values(char* start = 0);
    
    string emit_css(CSS_Style style);
  };
}