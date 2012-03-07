#include "document.hpp"

namespace Sass {

  void Document::parse_scss() {
    // try_munching<optional_spaces>();
    // while (*position) {
    //   statements.push_back(parse_statement());
    //   try_munching<optional_spaces>();
    // }
    
    lex<optional_spaces>();
    while(*position) {
      statements.push_back(parse_statement());
      lex<optional_spaces>();
    }
  }

  Node Document::parse_statement() {
    // if (try_munching<block_comment>()) {
    //   return Node(line_number, Node::comment, top);
    // }
    // else if (try_munching<variable>()) {
    //   return parse_var_def();
    // }
    // else return parse_ruleset();
    
    if (lex<block_comment>()) {
      return Node(line_number, Node::comment, lexed);
    }
    else if (lex<variable>()) {
      return parse_var_def();
    }
    else return parse_ruleset();
  }

  Node Document::parse_var_def() {
    // const Token key(top);
    // try_munching<exactly<':'> >();
    // environment[key] = parse_values();
    // try_munching<exactly<';'> >();
    // return Node(line_number, Node::nil, top);
    
    const Token key(lexed);
    lex< exactly<':'> >();
    environment[key] = parse_values();
    lex< exactly<';'> >();
    return Node();
  }

  Node Document::parse_ruleset() {
    Node ruleset(line_number, Node::ruleset, 2);
    ruleset << parse_selector();
    ruleset << parse_block();
    return ruleset;
  }

  Node Document::parse_selector() {
    try_munching<identifier>();
    return Node(line_number, Node::selector, top);
  }

  Node Document::parse_block() {
    try_munching<exactly<'{'> >();
    Node decls(line_number, Node::block);
    while(!try_munching<exactly<'}'> >()) {
      if (try_munching<block_comment>()) {
        decls << Node(line_number, Node::comment, top);
        continue;
      }
      else if (try_munching<variable>()) {
        decls << parse_var_def();
        continue;
      }
      try_munching<identifier>();
      Token id = top;
      if (try_munching<exactly<':'> >()) {
        Node rule(line_number, Node::rule, 2);
        rule << Node(line_number, Node::property, id);
        rule << parse_values();
        decls << rule;
        decls.has_rules = true;
        try_munching<exactly<';'> >();
      }
      else {
        Node ruleset(line_number, Node::ruleset, 2);
        ruleset << Node(line_number, Node::selector, id);
        ruleset << parse_block();
        decls << ruleset;
        decls.has_rulesets = true;
      }
    }
    return decls;
  }

  Node Document::parse_values() {
    Node values(line_number, Node::values);
    while(try_munching<identifier>() || try_munching<dimension>()  ||
          try_munching<percentage>() || try_munching<number>() ||
          try_munching<hex>()        || try_munching<string_constant>() ||
          try_munching<variable>()) {
      if (top.begin[0] == '$') {
        Node stuff(environment[top]);
        for (int i = 0; i < stuff.children->size(); ++i) {
          values << stuff.children->at(i);
        }
      }
      else
      {
        values << Node(line_number, Node::value, top);
      }
    }
    return values;      
  }

}