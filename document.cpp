#include <cstdio>
#include <cstring>
#include "document.hpp"
#include "eval_apply.hpp"
#include "error.hpp"
#include <iostream>

namespace Sass {
  
  Document::Document(char* path_str, char* source_str, Context& ctx)
  : path(string()),
    source(source_str),
    line_number(1),
    context(ctx),
    root(Node(Node::root, context.registry, 1)),
    lexed(Token::make())
  {
    if (source_str) {
      own_source = false;
      position = source;
      end = position + std::strlen(source);
    }
    else if (path_str) {
      path = string(path_str);
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
      end = source + len;
      std::fclose(f);
      own_source = true;
      position = source;
      context.source_refs.push_back(source);
    }
    else {
      // report an error
    }
    ++context.ref_count;
  }
      

  Document::Document(string path, char* source)
  : path(path), source(source),
    line_number(1), own_source(false),
    context(*(new Context())),
    root(Node(Node::root, context.registry, 1)),
    lexed(Token::make())
  {
    if (!source) {
      std::FILE *f;
      f = std::fopen(path.c_str(), "rb");
      if (!f) throw path;
      if (std::fseek(f, 0, SEEK_END)) throw path;
      int len = std::ftell(f);
      if (len < 0) throw path;
      std::rewind(f);
      // TO DO: CATCH THE POTENTIAL badalloc EXCEPTION
      source = new char[len + 1];
      std::fread(source, sizeof(char), len, f);
      if (std::ferror(f)) throw path;
      source[len] = '\0';
      end = source + len;
      if (std::fclose(f)) throw path;
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
    root(Node(Node::root, context.registry, 1)),
    lexed(Token::make())
  {
    std::FILE *f;
    f = std::fopen(path.c_str(), "rb");
    if (!f) throw path;
    if (std::fseek(f, 0, SEEK_END)) throw path;
    int len = std::ftell(f);
    if (len < 0) throw path;
    std::rewind(f);
    // TO DO: CATCH THE POTENTIAL badalloc EXCEPTION
    source = new char[len + 1];
    std::fread(source, sizeof(char), len, f);
    if (std::ferror(f)) throw path;
    source[len] = '\0';
    end = source + len;
    if (std::fclose(f)) throw path;
    position = source;
    context.source_refs.push_back(source);
    ++context.ref_count;
  }
  
  Document::Document(const string& path, size_t line_number, Token t, Context& context)
  : path(path),
    source(const_cast<char*>(t.begin)),
    position(t.begin),
    end(t.end),
    line_number(line_number),
    own_source(false),
    context(context),
    root(Node(Node::root, context.registry, 1)),
    lexed(Token::make())
  { }

  Document::~Document() {
    --context.ref_count;
    // if (context.ref_count == 0) delete &context;
  }
  
  void Document::syntax_error(string message, size_t ln)
  { throw Error(Error::syntax, ln ? ln : line_number, path, message); }
  
  void Document::read_error(string message, size_t ln)
  { throw Error(Error::read, ln ? ln : line_number, path, message); }
  
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
    default:
      break;
    }
    string retval(output.str());
    if (!retval.empty()) retval.resize(retval.size()-1);
    return retval;
  }
}
