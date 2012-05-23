#include <cstdio>
#include <cstring>
#include "document.hpp"
#include "eval_apply.hpp"
#include "error.hpp"
#include <iostream>
#include <sstream>

namespace Sass {

  Document::Document(Context& ctx) : context(ctx)
  { ++context.ref_count; }

  Document::Document(const Document& doc)
  : path(doc.path),
    source(doc.source),
    position(doc.position),
    end(doc.end),
    line(doc.line),
    own_source(doc.own_source),
    context(doc.context),
    root(doc.root),
    lexed(doc.lexed)
  { ++doc.context.ref_count; }

  Document::~Document()
  { --context.ref_count; }

  Document Document::make_from_file(Context& ctx, string path)
  {
    std::FILE *f;
    f = std::fopen(path.c_str(), "rb");
    if (!f) throw path;
    if (std::fseek(f, 0, SEEK_END)) throw path;
    int len = std::ftell(f);
    if (len < 0) throw path;
    std::rewind(f);
    char* source = new char[len + 1];
    std::fread(source, sizeof(char), len, f);
    if (std::ferror(f)) throw path;
    source[len] = '\0';
    char* end = source + len;
    if (std::fclose(f)) throw path;

    Document doc(ctx);
    doc.path        = path;
    doc.line = 1;
    doc.root        = ctx.new_Node(Node::root, path, 1, 0);
    doc.lexed       = Token::make();
    doc.own_source  = true;
    doc.source      = source;
    doc.end         = end;
    doc.position    = source;
    doc.context.source_refs.push_back(source);

    return doc;
  }

  Document Document::make_from_source_chars(Context& ctx, char* src, string path)
  {
    Document doc(ctx);
    doc.path = path;
    doc.line = 1;
    doc.root = ctx.new_Node(Node::root, path, 1, 0);
    doc.lexed = Token::make();
    doc.own_source = false;
    doc.source = src;
    doc.end = src + std::strlen(src);
    doc.position = doc.end;

    return doc;
  }

  Document Document::make_from_token(Context& ctx, Token t, string path, size_t line_number)
  {
    Document doc(ctx);
    doc.path = path;
    doc.line = line_number;
    doc.root = ctx.new_Node(Node::root, path, 1, 0);
    doc.lexed = Token::make();
    doc.own_source = false;
    doc.source = const_cast<char*>(t.begin);
    doc.end = t.end;
    doc.position = doc.source;

    return doc;
  }
  
  void Document::throw_syntax_error(string message, size_t ln)
  { throw Error(Error::syntax, ln ? ln : line, path, message); }
  
  void Document::throw_read_error(string message, size_t ln)
  { throw Error(Error::read, ln ? ln : line, path, message); }
  
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
