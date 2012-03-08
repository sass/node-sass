#include "document.hpp"

namespace Sass {

  void Document::parse_scss()
  {
    lex<optional_spaces>();
    while(*position) {
      statements.push_back(parse_statement());
      lex<optional_spaces>();
    }
  }

  Node Document::parse_statement()
  {
    if (lex<block_comment>()) {
      return Node(line_number, Node::comment, lexed);
    }
    else if (lex<variable>()) {
      return parse_var_def();
    }
    else return parse_ruleset();
  }

  Node Document::parse_var_def()
  {
    const Token key(lexed);
    lex< exactly<':'> >();
    environment[key] = parse_values();
    lex< exactly<';'> >();
    return Node();
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
    lex<identifier>();
    return Node(line_number, Node::selector, lexed);
  }

  Node Document::parse_block()
  {
    lex< exactly<'{'> >();
    Node block(line_number, Node::block);
    while (!lex< exactly<'}'> >()) {
      if (lex< block_comment >()) {
        block << Node(line_number, Node::comment, lexed);
        block.has_comments = true;
        continue;
      }
      else if (lex< variable >()) {
        block << parse_var_def();
        continue;
      }
      else if (look_for_rule()) {
        block << parse_rule();
        block.has_rules = true;
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