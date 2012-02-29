#include <cstdio>
#include "document.hpp"
#include <iostream>

namespace Sass {
  using namespace Prelexer;
  using std::cout; using std::endl;

  Document::Document(char* _path, char* _source) {
    path = _path;
    if (!_source) {
      std::FILE *f;
      // TO DO: CHECK f AGAINST NULL/0
      f = std::fopen(path, "rb");
      std::fseek(f, 0, SEEK_END);
      int len = std::ftell(f);
      std::rewind(f);
      // TO DO: WRAP THE new[] IN A TRY/CATCH BLOCK
      source = new char[len + 1];
      std::fread(source, sizeof(char), len, f);
      source[len] = '\0';
      std::fclose(f);
    }
    else {
      source = _source;
    }
    position = source;
    line_number = 1;
    last_munch_succeeded = false;
  }
  Document::~Document() {
    delete [] source;
  }
  
  void Document::parse_scss() {
    try_munching<optional_spaces>();
    while (*position) {
      statements.push_back(parse_statement());
      try_munching<optional_spaces>();
    }
  }
  
  Node Document::parse_statement() {
    if (try_munching<block_comment>()) {
      return Node(Node::comment, top);
    }
    else if (try_munching<variable>()) {
      return parse_var_def();
    }
    else return parse_ruleset();
  }
  
  Node Document::parse_var_def() {
    const Token key = top;
    try_munching<exactly<':'> >();
    environment[string(key)] = parse_values();
    try_munching<exactly<';'> >();
    return Node(Node::null);
  }
  
  Node Document::parse_ruleset() {
    Node ruleset(Node::ruleset);
    ruleset.push_child(parse_selector());
    ruleset.push_child(parse_clauses());
    return ruleset;
  }
  
  Node Document::parse_selector() {
    try_munching<identifier>();
    return Node(Node::selector, top);
  }
  
  Node Document::parse_clauses() {
    try_munching<exactly<'{'> >();
    Node decls(Node::clauses);
    while(!try_munching<exactly<'}'> >()) {
      if (try_munching<block_comment>()) {
        decls.push_child(Node(Node::comment, top));
        continue;
      }
      else if (try_munching<variable>()) {
        decls.push_child(parse_var_def());
        continue;
      }
      try_munching<identifier>();
      Token id = top;
      if (try_munching<exactly<':'> >()) {
        Node rule(Node::rule);
        rule.push_child(Node(Node::property, id));
        rule.push_child(parse_values());
        decls.push_child(rule);
        try_munching<exactly<';'> >();
      }
      else {
        Node ruleset(Node::ruleset);
        ruleset.push_child(Node(Node::selector, id));
        ruleset.push_child(parse_clauses());
        decls.push_opt_child(ruleset);
      }
    }
    return decls;
  }
  
  Node Document::parse_values() {
    Node values(Node::values);
    while(try_munching<identifier>() || try_munching<dimension>()  ||
          try_munching<percentage>() || try_munching<number>() ||
          try_munching<hex>()        || try_munching<variable>()) {
      if (top.begin[0] == '$') {
        Node stuff(environment[string(top)]);
        for (int i = 0; i < stuff.children.size(); ++i) {
          values.push_child(stuff.children[i]);
        }
      }
      else
        values.push_child(Node(Node::value, top));
    }
    return values;      
  }

}