#include "node.hpp"

namespace Sass {
  using std::vector;
  using namespace Prelexer;

  struct Document {
    char* path;
    char* source;
    char* position;
    unsigned int line_number;
    vector<Node> statements;
    Token top;
    bool last_munch_succeeded;
    
    Document(char* _path, char* _source = 0);
    ~Document();
    
    inline Token& peek() { return top; }

    template <prelexer mx>
    bool try_munching() {
      char* after_whitespace;
      if (mx == block_comment) {
        after_whitespace = optional_spaces(position);
      }
      else if (mx == spaces || mx == ancestor_of) {
        after_whitespace = spaces(position);
        if (after_whitespace) {
          top = Token(mx, position, after_whitespace, line_number);
          line_number += count_interval<'\n'>(position, after_whitespace);
          position = after_whitespace;
          return last_munch_succeeded = true;
        }
        else {
          return last_munch_succeeded = false;
        }
      }
      else {
        after_whitespace = spaces_and_comments(position);
      }
      line_number += count_interval<'\n'>(position, after_whitespace);
      char* after_token = mx(after_whitespace);
      if (after_token) {
        top = Token(mx, after_whitespace, after_token, line_number);
        position = after_token;
        return last_munch_succeeded = true;
      }
      else {
        return last_munch_succeeded = false;
      }
    } 
      
  };
}