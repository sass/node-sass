#include "document.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  void Document::parse_scss()
  {
    // cerr << "parse_scss" << endl;
    lex<optional_spaces>();
    while(*position) {
      if (lex< block_comment >()) {
        statements.push_back(Node(line_number, Node::comment, lexed));
      }
      else if (lex< variable >()) {
        parse_var_def();
      }
      else {
        statements.push_back(parse_ruleset());
      }
      lex<optional_spaces>();
    }
  }

  // Node Document::parse_statement()
  // {
  //   if (lex<block_comment>()) {
  //     return Node(line_number, Node::comment, lexed);
  //   }
  //   else if (lex<variable>()) {
  //     parse_var_def();
  //   }
  //   else return parse_ruleset();
  // }

  void Document::parse_var_def()
  {
    // cerr << "parse_var_def" << endl;
    const Token key(lexed);
    lex< exactly<':'> >();
    environment[key] = parse_values();
    lex< exactly<';'> >();
  }

  Node Document::parse_ruleset()
  {
    // cerr << "parse_ruleset" << endl;
    Node ruleset(line_number, Node::ruleset, 2);
    ruleset << parse_selector_group();
    ruleset << parse_block();
    return ruleset;
  }
  
  Node Document::parse_selector_group()
  {
    // cerr << "parse_selector_group" << endl;
    Node group(line_number, Node::selector_group, 1);
    group << parse_selector();
    while (lex< exactly<','> >()) group << parse_selector();
    return group;
  }

  Node Document::parse_selector()
  {
    // cerr << "parse_selector" << endl;
    lex<identifier>();
    return Node(line_number, Node::selector, lexed);
  }

  Node Document::parse_block()
  {
    // cerr << "parse_block" << endl;
    lex< exactly<'{'> >();
    bool semicolon = false;
    Node block(line_number, Node::block);
    while (!lex < exactly<'}'> >()) {
      if (semicolon) {
        lex< exactly<';'> >(); // enforce terminal ';' here
        semicolon = false;
        if (lex< exactly<'}'> >()) break;
      }
      if (lex< block_comment >()) {
        // cerr << "grabbed a comment" << endl;
        block << Node(line_number, Node::comment, lexed);
        block.has_rules_or_comments = true;
        semicolon = true;
        continue;
      }
      else if (lex< variable >()) {
        parse_var_def();
        continue;
      }
      else if (look_for_rule(position)) {
        block << parse_rule();
        block.has_rules_or_comments = true;
        semicolon = true;
        continue;
      }
      else {
        block << parse_ruleset();
        block.has_rulesets = true;
        continue;
      }
    }
    return block;
    //   lex< identifier >();
    //   // Token id(lexed);
    //   if (peek< exactly<':'> >()) {
    //     Node rule(line_number, Node::rule, 2);
    //     rule << Node(line_number, Node::property, lexed);
    //     lex< exactly<':'> >();
    //     rule << parse_values();
    //     block << rule;
    //     block.has_rules = true;
    //     lex< exactly<';'> >();
    //   }
    //   else {
    //     Node ruleset(line_number, Node::ruleset, 2);
    //     ruleset << Node(line_number, Node::selector, lexed);
    //     ruleset << parse_block();
    //     block << ruleset;
    //     block.has_rulesets = true;
    //   }
    // }
    // return block;   
  }
  
  Node Document::parse_rule() {
    // cerr << "parse_rule" << endl;
    Node rule(line_number, Node::rule, 2);
    lex< identifier >();
    rule << Node(line_number, Node::property, lexed);
    lex< exactly<':'> >();
    rule << parse_values();
    return rule;
  }

  Node Document::parse_values()
  {
    // cerr << "parse_value" << endl;
    Node values(line_number, Node::values);
    while (lex< identifier >() || lex < dimension >()       ||
           lex< percentage >() || lex < number >()          ||
           lex< hex >()        || lex < string_constant >() ||
           lex< variable >()) {
      if (lexed.begin[0] == '$') {
        Node fetched(environment[lexed]);
        for (int i = 0; i < fetched.children->size(); ++i) {
          values << fetched.children->at(i);
        }
      }
      else {
        values << Node(line_number, Node::value, lexed);
      }
    }
    return values;
  }
  
  char* Document::look_for_rule(char* start)
  {
    // cerr << "look_for_rule" << endl;
    char* p = start ? start : position;
    // if (p = peek<identifier>(p)) cerr << string(Token(start, p)) << endl;
    // if (p = peek<exactly<':'> >(p)) cerr << string(Token(start, p)) << endl;
    // if (p = look_for_values(p)) cerr << string(Token(start, p)) << endl;
    // if (p = peek< alternatives< exactly<';'>, exactly<'}'> > >(p)) cerr << string(Token(start, p)) << endl;
    (p = peek< identifier >(p))   &&
    (p = peek< exactly<':'> >(p)) &&
    (p = look_for_values(p))      &&
    (p = peek< alternatives< exactly<';'>, exactly<'}'> > >(p));
    return p;
  }
  
  char* Document::look_for_values(char* start)
  {
    // cerr << "look_for_values" << endl;
    char* p = start ? start : position;
    char* q;
    while ((q = peek< identifier >(p)) || (q = peek< dimension >(p))       ||
           (q = peek< percentage >(p)) || (q = peek< number >(p))          ||
           (q = peek< hex >(p))        || (q = peek< string_constant >(p)) ||
           (q = peek< variable >(p)))
    { /* cerr << *q; */ p = q; }
    // cerr << string(Token(start, p)) << "blah" ;
    // cerr << (p == start ? "nothing" : "something") << endl;
    return p == start ? 0 : p;
  }
}