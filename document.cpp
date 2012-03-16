#include <cstdio>
#include "document.hpp"
#include <iostream>

namespace Sass {

  Document::Document(string path, char* source)
  : path(path), source(source), //source_refs(vector<char*>()),
    line_number(1), own_source(false),
    context(*(new Context())),
    root(Node(1, Node::root)),
    // statements(vector<Node>()),
    lexed(Token())
  {
    // if (!source) read_file();
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
  : path(path), source(0), //source_refs(vector<char*>()),
    line_number(1), own_source(false),
    context(context),
    root(Node(1, Node::root)),
    // statements(vector<Node>()),
    lexed(Token())
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
    // if (own_source) delete [] source;
    --context.ref_count;
    if (context.ref_count == 0) delete &context;
    // for (int i = 0; i < source_refs.size(); ++i) {
    //   delete [] source_refs[i];
    // }
  }
  
}