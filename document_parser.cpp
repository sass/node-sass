#include "document.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  void Document::parse_scss()
  {
    lex<optional_spaces>();
    while(*position) {
      if (lex< block_comment >()) {
        statements.push_back(Node(line_number, Node::comment, lexed));
      }
      else if (lex< variable >()) {
        parse_var_def();
        lex< exactly<';'> >();
      }
      else {
        statements.push_back(parse_ruleset());
      }
      lex<optional_spaces>();
    }
  }

  void Document::parse_var_def()
  {
    const Token key(lexed);
    lex< exactly<':'> >();
    environment[key] = parse_values();
  }

  Node Document::parse_ruleset()
  {
    Node ruleset(line_number, Node::ruleset, 2);
    ruleset << parse_selector_group();
    ruleset << parse_block();
    return ruleset;
  }
  
  Node Document::parse_selector_group()
  {
    Node group(line_number, Node::selector_group, 1);
    group << parse_selector();
    while (lex< exactly<','> >()) group << parse_selector();
    return group;
  }

  Node Document::parse_selector()
  {
    Node selector(line_number, Node::selector, 1);
    selector << parse_simple_selector_sequence();
    while (lex< exactly<'+'> >() ||
           lex< exactly<'~'> >() ||
           lex< exactly<'>'> >() ||
           lex< ancestor_of >()) {
      selector << Node(line_number, Node::selector_combinator, lexed);
      selector << parse_simple_selector_sequence();
    }
    return selector;
  }
  
  Node Document::parse_simple_selector_sequence()
  {
    lex<identifier>();
    return Node(line_number, Node::simple_selector_sequence, lexed);
  }

  Node Document::parse_block()
  {
    lex< exactly<'{'> >();
    bool semicolon = false;
    Node block(line_number, Node::block);
    while (!lex< exactly<'}'> >()) {
      if (semicolon) {
        lex< exactly<';'> >(); // enforce terminal ';' here
        semicolon = false;
        if (lex< exactly<'}'> >()) break;
      }
      if (lex< block_comment >()) {
        block << Node(line_number, Node::comment, lexed);
        block.has_rules_or_comments = true;
        semicolon = true;
      }
      else if (lex< variable >()) {
        parse_var_def();
        semicolon = true;
      }
      else if (look_for_rule(position)) {
        block << parse_rule();
        block.has_rules_or_comments = true;
        semicolon = true;
      }
      else if (!peek< exactly<';'> >()) {
        block << parse_ruleset();
        block.has_rulesets = true;
      }
      else lex< exactly<';'> >();
    }
    return block;
  }
  
  Node Document::parse_rule() {
    Node rule(line_number, Node::rule, 2);
    lex< identifier >();
    rule << Node(line_number, Node::property, lexed);
    lex< exactly<':'> >();
    rule << parse_values();
    return rule;
  }

  Node Document::parse_values()
  {
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
    char* p = start ? start : position;
    (p = peek< identifier >(p))   &&
    (p = peek< exactly<':'> >(p)) &&
    (p = look_for_values(p))      &&
    (p = peek< alternatives< exactly<';'>, exactly<'}'> > >(p));
    return p;
  }
  
  char* Document::look_for_values(char* start)
  {
    char* p = start ? start : position;
    char* q;
    while ((q = peek< identifier >(p)) || (q = peek< dimension >(p))       ||
           (q = peek< percentage >(p)) || (q = peek< number >(p))          ||
           (q = peek< hex >(p))        || (q = peek< string_constant >(p)) ||
           (q = peek< variable >(p)))
    { p = q; }
    return p == start ? 0 : p;
  }
}