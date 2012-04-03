#include <cstdio>
#include "document.hpp"
#include "eval_apply.hpp"
#include <iostream>

namespace Sass {

  Document::Document(string path, char* source)
  : path(path), source(source),
    line_number(1), own_source(false),
    context(*(new Context())),
    root(Node(Node::root, 1)),
    lexed(Token::make())
  {
    if (!source) {
      std::FILE *f;
      // TO DO: CHECK f AGAINST NULL/0
      f = std::fopen(path.c_str(), "rb");
      std::fseek(f, 0, SEEK_END);
      int len = std::ftell(f);
      std::rewind(f);
      // TO DO: WRAP THE new[] IN A TRY/CATCH BLOCK
      source = new char[len + 1];
      std::fread(source, sizeof(char), len, f);
      source[len] = '\0';
      std::fclose(f);
      own_source = true;
    }
    position = source;
    context.source_refs.push_back(source);
    ++context.ref_count;
  }
  
  Document::Document(string path, Context& context)
  : path(path), source(0),
    line_number(1), own_source(false),
    context(context),
    root(Node(Node::root, 1)),
    lexed(Token::make())
  {
    std::FILE *f;
    // TO DO: CHECK f AGAINST NULL/0
    f = std::fopen(path.c_str(), "rb");
    std::fseek(f, 0, SEEK_END);
    int len = std::ftell(f);
    std::rewind(f);
    // TO DO: WRAP THE new[] IN A TRY/CATCH BLOCK
    source = new char[len + 1];
    std::fread(source, sizeof(char), len, f);
    source[len] = '\0';
    std::fclose(f);
    position = source;
    context.source_refs.push_back(source);
    ++context.ref_count;
  }

  Document::~Document() {
    --context.ref_count;
    if (context.ref_count == 0) delete &context;
  }
  
  // void Document::eval_pending()
  // {
  //   for (int i = 0; i < context.pending.size(); ++i) {
  //     eval(context.pending[i], context.global_env);
  //   }
  // }
  
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