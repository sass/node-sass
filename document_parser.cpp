#include "document.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  void Document::parse_scss()
  {
    lex<optional_spaces>();
    // while(*position) {
    //   if (lex< block_comment >()) {
    //     statements.push_back(Node(line_number, Node::comment, lexed));
    //   }
    //   else if (peek< variable >(position)) {
    //     parse_var_def();
    //     lex< exactly<';'> >();
    //   }
    //   else {
    //     statements.push_back(parse_ruleset());
    //   }
    //   lex<optional_spaces>();
    // }
    
    while(*position) {
      if (lex< block_comment >()) {
        root << Node(line_number, Node::comment, lexed);
      }
      else if (peek< variable >(position)) {
        parse_var_def();
        lex< exactly<';'> >();
      }
      else {
        root << parse_ruleset();
      }
      lex<optional_spaces>();
    }
  }

  void Document::parse_var_def()
  {
    lex< variable >();
    const Token key(lexed);
    lex< exactly<':'> >();
    context.environment[key] = parse_values();
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
    if (lex< exactly<'+'> >() ||
        lex< exactly<'~'> >() ||
        lex< exactly<'>'> >()) {
      selector << Node(line_number, Node::selector_combinator, lexed);
    }
    Node s(parse_simple_selector_sequence());
    if (s.has_backref) selector.has_backref = true;
    selector << s;
    while (lex< exactly<'+'> >() ||
           lex< exactly<'~'> >() ||
           lex< exactly<'>'> >() ||
           lex< ancestor_of >() /*||
           s.terminal_backref && lex< no_spaces >()*/) {
      selector << Node(line_number, Node::selector_combinator, lexed);
      s = parse_simple_selector_sequence();
      if (s.has_backref) selector.has_backref = true;
      selector << s;
    }
    return selector;
  }

  Node Document::parse_simple_selector_sequence()
  {
    Node seq(line_number, Node::simple_selector_sequence, 1);
    if (lex< alternatives < type_selector, universal > >()) {
      seq << Node(line_number, Node::simple_selector, lexed);
    }
    else if (lex< exactly<'&'> >()) {
      seq << Node(line_number, Node::backref, lexed);
      seq.has_backref = true;
      // if (peek< sequence< no_spaces, alternatives< type_selector, universal > > >(position)) {
      //   seq.terminal_backref = true;
      //   return seq;
      // }
    }
    else {
      seq << parse_simple_selector();
    }
    while (!peek< spaces >(position) &&
           !(peek < exactly<'+'> >(position) ||
             peek < exactly<'~'> >(position) ||
             peek < exactly<'>'> >(position) ||
             peek < exactly<','> >(position) ||
             peek < exactly<'{'> >(position))) {
      seq << parse_simple_selector();
    }
    return seq; 
  }
  
  Node Document::parse_simple_selector()
  {
    if (lex< id_name >() || lex< class_name >()) {
      return Node(line_number, Node::simple_selector, lexed);
    }
    else if (peek< exactly<':'> >(position)) {
      return parse_pseudo();
    }
    else if (peek< exactly<'['> >(position)) {
      return parse_attribute_selector();
    }
  }
  
  Node Document::parse_pseudo() {
    if (lex< sequence< pseudo_prefix, functional > >()) {
      Node pseudo(line_number, Node::functional_pseudo, 2);
      pseudo << Node(line_number, Node::value, lexed);
      if (lex< alternatives< even, odd > >()) {
        pseudo << Node(line_number, Node::value, lexed);
      }
      else if (peek< binomial >(position)) {
        lex< coefficient >();
        pseudo << Node(line_number, Node::value, lexed);
        lex< exactly<'n'> >();
        pseudo << Node(line_number, Node::value, lexed);
        lex< sign >();
        pseudo << Node(line_number, Node::value, lexed);
        lex< digits >();
        pseudo << Node(line_number, Node::value, lexed);
      }
      else if (lex< sequence< optional<sign>,
                              optional<digits>,
                              exactly<'n'> > >()) {
        pseudo << Node(line_number, Node::value, lexed);
      }
      else if (lex< sequence< optional<sign>, digits > >()) {
        pseudo << Node(line_number, Node::value, lexed);
      }
      lex< exactly<')'> >();
      return pseudo;
    }
    else if (lex < sequence< pseudo_prefix, identifier > >()) {
      return Node(line_number, Node::pseudo, lexed);
    }
  }
  
  Node Document::parse_attribute_selector()
  {
    Node attr_sel(line_number, Node::attribute_selector, 3);
    lex< exactly<'['> >();
    lex< type_selector >();
    attr_sel << Node(line_number, Node::value, lexed);
    lex< alternatives< exact_match, class_match, dash_match,
                       prefix_match, suffix_match, substring_match > >();
    attr_sel << Node(line_number, Node::value, lexed);
    lex< string_constant >();
    attr_sel << Node(line_number, Node::value, lexed);
    lex< exactly<']'> >();
    return attr_sel;
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
        Node fetched(context.environment[lexed]);
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