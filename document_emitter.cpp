#include "document.hpp"

namespace Sass {
  using std::string;
  using std::stringstream;
  using std::endl;
  
  string Document::emit_css(CSS_Style style) {
    stringstream output;
    switch (style) {
    case echo:
      root.echo(output);
      break;
    case nested:
      root.emit_nested_css(output, 0, vector<string>());
      break;
    case expanded:
      root.emit_expanded_css(output, "");
      break;
    }
    string retval(output.str());
    if (!retval.empty()) retval.resize(retval.size()-1);
    return retval;
  }
  
}