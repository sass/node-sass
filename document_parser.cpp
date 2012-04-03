#include "document.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  void Document::parse_scss()
  {
    lex<optional_spaces>();
    root << Node(Node::flags);
    while(*position) {
      if (lex< block_comment >()) {
        root << Node(Node::comment, line_number, lexed);
      }
      else if (peek< import >(position)) {
        // TO DO: don't splice in place at parse-time -- use an expansion node
        root += parse_import();
        lex< exactly<';'> >();
      }
      else if (peek< mixin >(position)) {
        root << parse_mixin_definition();
      }
      else if (peek< include >(position)) {
        root << parse_mixin_call();
        root[0].has_expansions = true;
        lex< exactly<';'> >();
      }
      else if (peek< variable >(position)) {
        root << parse_assignment();
        lex< exactly<';'> >();
      }
      else {
        root << parse_ruleset();
      }
      lex<optional_spaces>();
    }
  }

  Node Document::parse_import()
  {
    lex< import >();
    lex< string_constant >();
    string import_path(lexed.unquote());
    const char* curr_path_start = path.c_str();
    const char* curr_path_end   = folders(curr_path_start);
    string current_path(curr_path_start, curr_path_end - curr_path_start);
    Document importee(current_path + import_path, context);
    importee.parse_scss();
    return importee.root;
  }

  Node Document::parse_mixin_definition()
  {
    lex< mixin >();
    lex< identifier >();
    Node name(Node::identifier, line_number, lexed);
    Node params(parse_mixin_parameters());
    Node body(parse_block(true));
    Node mixin(Node::mixin, line_number, 3);
    mixin << name << params << body;
    return mixin;
  }

  Node Document::parse_mixin_parameters()
  {
    Node params(Node::parameters, line_number);
    lex< exactly<'('> >();
    if (peek< variable >()) {
      params << parse_parameter();
      while (lex< exactly<','> >()) {
        params << parse_parameter();
      }
    }
    lex< exactly<')'> >();
    return params;
  }

  Node Document::parse_parameter() {
    lex< variable >();
    Node var(Node::variable, line_number, lexed);
    if (lex< exactly<':'> >()) { // default value
      Node val(parse_space_list());
      Node par_and_val(Node::assignment, line_number, 2);
      par_and_val << var << val;
      return par_and_val;
    }
    else {
      return var;
    }
  }

  Node Document::parse_mixin_call()
  {
    lex< include >();
    lex< identifier >();
    Node name(Node::identifier, line_number, lexed);
    Node args(parse_mixin_arguments());
    Node call(Node::expansion, line_number, 3);
    call << name << args;
    return call;
  }
  
  Node Document::parse_mixin_arguments()
  {
    Node args(Node::arguments, line_number);
    if (lex< exactly<'('> >()) {
      if (!peek< exactly<')'> >(position)) {
        args << parse_argument();
        while (lex< exactly<','> >()) {
          args << parse_argument();
        }
      }
      lex< exactly<')'> >();
    }
    return args;
  }
  
  Node Document::parse_argument()
  {
    if (peek< sequence < variable, spaces_and_comments, exactly<':'> > >()) {
      lex< variable >();
      Node var(Node::variable, line_number, lexed);
      lex< exactly<':'> >();
      Node val(parse_space_list());
      Node assn(Node::assignment, line_number, 2);
      assn << var << val;
      return assn;
    }
    else {
      return parse_space_list();
    }
  }

  Node Document::parse_assignment()
  {
    lex< variable >();
    Node var(Node::variable, line_number, lexed);
    lex< exactly<':'> >();
    Node val(parse_list());
    Node assn(Node::assignment, line_number, 2);
    assn << var << val;
    return assn;
  }

  Node Document::parse_ruleset(bool definition)
  {
    Node ruleset(Node::ruleset, line_number, 2);
    ruleset << parse_selector_group();
    ruleset << parse_block(definition);
    return ruleset;
  }

  Node Document::parse_selector_group()
  {
    // Node group(Node::selector_group, line_number, 1);
    // group << parse_selector();
    // while (lex< exactly<','> >()) group << parse_selector();
    // return group;
    
    Node sel1(parse_selector());
    if (!lex< exactly<','> >()) return sel1;
    
    Node group(Node::selector_group, line_number, 2);
    group << sel1;
    while (lex< exactly<','> >()) group << parse_selector();
    return group;
  }

  Node Document::parse_selector()
  {
    // Node selector(Node::selector, line_number, 1);
    // if (lex< exactly<'+'> >() ||
    //     lex< exactly<'~'> >() ||
    //     lex< exactly<'>'> >()) {
    //   selector << Node(Node::selector_combinator, line_number, lexed);
    // }
    // Node s(parse_simple_selector_sequence());
    // if (s.has_backref) selector.has_backref = true;
    // selector << s;
    // while (lex< exactly<'+'> >() ||
    //        lex< exactly<'~'> >() ||
    //        lex< exactly<'>'> >() ||
    //        lex< ancestor_of >() /*||
    //        s.terminal_backref && lex< no_spaces >()*/) {
    //   selector << Node(Node::selector_combinator, line_number, lexed);
    //   s = parse_simple_selector_sequence();
    //   if (s.has_backref) selector.has_backref = true;
    //   selector << s;
    // }
    // return selector;
    
    Node seq1(parse_simple_selector_sequence());
    if (!lex< exactly<','> >()) return seq1;
    
    Node selector(Node::selector, line_number, 2);
    if (seq1.has_backref) selector.has_backref = true;
    selector << seq1;
    while (lex< exactly<','> >()) {
      Node seq(parse_simple_selector_sequence());
      if (seq.has_backref) selector.has_backref = true;
      selector << seq;
    }
    return selector;
  }

  Node Document::parse_simple_selector_sequence()
  {
    Node seq(Node::simple_selector_sequence, line_number, 1);
    if (lex< alternatives < type_selector, universal > >()) {
      seq << Node(Node::simple_selector, line_number, lexed);
    }
    else if (lex< exactly<'&'> >()) {
      seq << Node(Node::backref, line_number, lexed);
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
             peek < exactly<')'> >(position) ||
             peek < exactly<'{'> >(position))) {
      seq << parse_simple_selector();
    }
    return seq; 
  }
  
  Node Document::parse_simple_selector()
  {
    if (lex< id_name >() || lex< class_name >()) {
      return Node(Node::simple_selector, line_number, lexed);
    }
    else if (peek< exactly<':'> >(position)) {
      return parse_pseudo();
    }
    else if (peek< exactly<'['> >(position)) {
      return parse_attribute_selector();
    }
  }
  
  Node Document::parse_pseudo() {
    if (lex< pseudo_not >()) {
      Node ps_not(Node::pseudo_negation, line_number, 2);
      ps_not << Node(Node::value, line_number, lexed);
      ps_not << parse_selector_group();
      lex< exactly<')'> >();
      return ps_not;
    }
    else if (lex< sequence< pseudo_prefix, functional > >()) {
      Node pseudo(Node::functional_pseudo, line_number, 2);
      pseudo << Node(Node::value, line_number, lexed);
      if (lex< alternatives< even, odd > >()) {
        pseudo << Node(Node::value, line_number, lexed);
      }
      else if (peek< binomial >(position)) {
        lex< coefficient >();
        pseudo << Node(Node::value, line_number, lexed);
        lex< exactly<'n'> >();
        pseudo << Node(Node::value, line_number, lexed);
        lex< sign >();
        pseudo << Node(Node::value, line_number, lexed);
        lex< digits >();
        pseudo << Node(Node::value, line_number, lexed);
      }
      else if (lex< sequence< optional<sign>,
                              optional<digits>,
                              exactly<'n'> > >()) {
        pseudo << Node(Node::value, line_number, lexed);
      }
      else if (lex< sequence< optional<sign>, digits > >()) {
        pseudo << Node(Node::value, line_number, lexed);
      }
      lex< exactly<')'> >();
      return pseudo;
    }
    else if (lex < sequence< pseudo_prefix, identifier > >()) {
      return Node(Node::pseudo, line_number, lexed);
    }
  }
  
  Node Document::parse_attribute_selector()
  {
    Node attr_sel(Node::attribute_selector, line_number, 3);
    lex< exactly<'['> >();
    lex< type_selector >();
    attr_sel << Node(Node::value, line_number, lexed);
    lex< alternatives< exact_match, class_match, dash_match,
                       prefix_match, suffix_match, substring_match > >();
    attr_sel << Node(Node::value, line_number, lexed);
    lex< string_constant >();
    attr_sel << Node(Node::value, line_number, lexed);
    lex< exactly<']'> >();
    return attr_sel;
  }

  Node Document::parse_block(bool definition)
  {
    lex< exactly<'{'> >();
    bool semicolon = false;
    Node block(Node::block, line_number, 1);
    block << Node(Node::flags);
    while (!lex< exactly<'}'> >()) {
      if (semicolon) {
        lex< exactly<';'> >(); // enforce terminal ';' here
        semicolon = false;
        if (lex< exactly<'}'> >()) break;
      }
      if (lex< block_comment >()) {
        block << Node(Node::comment, line_number, lexed);
        block[0].has_statements = true;
        semicolon = true;
      }
      else if (peek< import >(position)) {
        // TO DO: disallow imports inside of definitions
        Node imported_tree(parse_import());
        for (int i = 0; i < imported_tree.size(); ++i) {
          if (imported_tree[i].type == Node::comment ||
              imported_tree[i].type == Node::rule) {
            block[0].has_statements = true;
          }
          else if (imported_tree[i].type == Node::ruleset) {
            block[0].has_blocks = true;
          }
          block << imported_tree[i];
        }
        semicolon = true;
      }
      else if (peek< include >(position)) {
        block << parse_mixin_call();
        block[0].has_expansions = true;
      }
      else if (lex< variable >()) {
        block << parse_assignment();
        semicolon = true;
      }
      // else if (look_for_rule(position)) {
      //   block << parse_rule();
      //   block.has_statements = true;
      //   semicolon = true;
      // }
      // else if (!peek< exactly<';'> >()) {
      //   block << parse_ruleset();
      //   block.has_blocks = true;
      // }
      else if (const char* p = look_for_selector_group(position)) {
        block << parse_ruleset(definition);
        block[0].has_blocks = true;
      }
      else if (!peek< exactly<';'> >()) {
        block << parse_rule();
        block[0].has_statements = true;
        semicolon = true;
        lex< exactly<';'> >(); // TO DO: clean up the semicolon handling stuff
      }
      else lex< exactly<';'> >();
      while (lex< block_comment >()) {
        block << Node(Node::comment, line_number, lexed);
        block[0].has_statements = true;
      }
    }
    return block;
  }

  Node Document::parse_rule() {
    Node rule(Node::rule, line_number, 2);
    lex< identifier >();
    rule << Node(Node::property, line_number, lexed);
    lex< exactly<':'> >();
    // rule << parse_values();
    rule << parse_list();
    return rule;
  }
  
  Node Document::parse_list()
  {
    return parse_comma_list();
  }
  
  Node Document::parse_comma_list()
  {
    if (peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<')'> >(position)) {
      return Node(Node::nil, line_number);  // TO DO: maybe use Node::none?
    }
    Node list1(parse_space_list());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< exactly<','> >(position)) return list1;
    
    Node comma_list(Node::comma_list, line_number, 2);
    comma_list << list1;
    comma_list.eval_me |= list1.eval_me;
    
    while (lex< exactly<','> >())
    {
      Node list(parse_space_list());
      comma_list << list;
      comma_list.eval_me |= list.eval_me;
    }
    
    return comma_list;
  }
  
  Node Document::parse_space_list()
  {
    Node expr1(parse_expression());
    // if it's a singleton, return it directly; don't wrap it
    if (peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<')'> >(position) ||
        peek< exactly<','> >(position))
    { return expr1; }
    
    Node space_list(Node::space_list, line_number, 2);
    space_list << expr1;
    space_list.eval_me |= expr1.eval_me;
    
    while (!(peek< exactly<';'> >(position) ||
             peek< exactly<'}'> >(position) ||
             peek< exactly<')'> >(position) ||
             peek< exactly<','> >(position)))
    {
      Node expr(parse_expression());
      space_list << expr;
      space_list.eval_me |= expr.eval_me;
    }
    
    return space_list;
  }
  
  Node Document::parse_expression()
  {
    Node term1(parse_term());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'+'> >(position) ||
          peek< sequence< negate< number >, exactly<'-'> > >(position)))
    { return term1; }
    
    Node expression(Node::expression, line_number, 3);
    term1.eval_me = true;
    expression << term1;
    
    while (lex< exactly<'+'> >() || lex< sequence< negate< number >, exactly<'-'> > >()) {
      if (lexed.begin[0] == '+') {
        expression << Node(Node::add, line_number, lexed);
      }
      else {
        expression << Node(Node::sub, line_number, lexed);
      }
      Node term(parse_term());
      term.eval_me = true;
      expression << term;
    }
    expression.eval_me = true;

    return expression;
  }
  
  Node Document::parse_term()
  {
    Node fact1(parse_factor());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'*'> >(position) ||
          peek< exactly<'/'> >(position)))
    { return fact1; }

    Node term(Node::term, line_number, 3);
    term << fact1;
    if (fact1.eval_me) term.eval_me = true;

    while (lex< exactly<'*'> >() || lex< exactly<'/'> >()) {
      if (lexed.begin[0] == '*') {
        term << Node(Node::mul, line_number, lexed);
        term.eval_me = true;
      }
      else {
        term << Node(Node::div, line_number, lexed);
      }
      Node fact(parse_factor());
      if (fact.eval_me) term.eval_me = true;
      term << fact;
    }

    return term;
  }
  
  Node Document::parse_factor()
  {
    if (lex< exactly<'('> >()) {
      Node value(parse_comma_list());
      value.eval_me = true;
      if (value.type == Node::comma_list || value.type == Node::space_list) {
        value[0].eval_me = true;
      }
      lex< exactly<')'> >();
      return value;
    }
    else {
      return parse_value();
    }
  }
  
  Node Document::parse_value()
  {
    if (lex< uri_prefix >())
    {
      lex< string_constant >();
      Node result(Node::uri, line_number, lexed);
      lex< exactly<')'> >();
      return result;
    }
    
    if (lex< rgb_prefix >())
    {
      Node result(Node::numeric_color, line_number, 3);
      lex< number >();
      result << Node(line_number, std::atof(lexed.begin));
      lex< exactly<','> >();
      lex< number >();
      result << Node(line_number, std::atof(lexed.begin));
      lex< exactly<','> >();
      lex< number >();
      result << Node(line_number, std::atof(lexed.begin));
      lex< exactly<')'> >();
      return result;
    }

    if (lex< identifier >())
    { return Node(Node::identifier, line_number, lexed); }

    if (lex< percentage >())
    { return Node(Node::textual_percentage, line_number, lexed); }

    if (lex< dimension >())
    { return Node(Node::textual_dimension, line_number, lexed); }

    if (lex< number >())
    { return Node(Node::textual_number, line_number, lexed); }

    if (lex< hex >())
    { return Node(Node::textual_hex, line_number, lexed); }

    if (lex< string_constant >())
    { return Node(Node::string_constant, line_number, lexed); }

    if (lex< variable >())
    {
      Node var(Node::variable, line_number, lexed);
      var.eval_me = true;
      return var;
    }
  }
  
  Node Document::parse_identifier() {
    lex< identifier >();
    return Node(Node::identifier, line_number, lexed);
  }
  
  Node Document::parse_variable() {
    lex< variable >();
    return Node(Node::variable, line_number, lexed);
  }

  // const char* Document::look_for_rule(const char* start)
  // {
  //   const char* p = start ? start : position;
  //   (p = peek< identifier >(p))   &&
  //   (p = peek< exactly<':'> >(p)) &&
  //   (p = look_for_values(p))      &&
  //   (p = peek< alternatives< exactly<';'>, exactly<'}'> > >(p));
  //   return p;
  // }
  // 
  // const char* Document::look_for_values(const char* start)
  // {
  //   const char* p = start ? start : position;
  //   const char* q;
  //   while ((q = peek< identifier >(p)) || (q = peek< dimension >(p))       ||
  //          (q = peek< percentage >(p)) || (q = peek< number >(p))          ||
  //          (q = peek< hex >(p))        || (q = peek< string_constant >(p)) ||
  //          (q = peek< variable >(p)))
  //   { p = q; }
  //   return p == start ? 0 : p;
  // }
  
  // NEW LOOKAHEAD FUNCTIONS. THIS ESSENTIALLY IMPLEMENTS A BACKTRACKING
  // PARSER, BECAUSE SELECTORS AND VALUES ARE NOT EXPRESSIBLE IN A
  // REGULAR LANGUAGE.
  const char* Document::look_for_selector_group(const char* start)
  {
    const char* p = start ? start : position;
    const char* q = look_for_selector(p);

    if (!q) { return 0; }
    else    { p = q; }

    while ((q = peek< exactly<','> >(p)) && (q = look_for_selector(q)))
    { p = q; }
    
    // return peek< exactly<'{'> >(p) ? p : 0;
    return peek< alternatives< exactly<'{'>, exactly<')'> > >(p) ? p : 0;
  }
  
  const char* Document::look_for_selector(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;

    if ((q = peek< exactly<'+'> >(p)) ||
        (q = peek< exactly<'~'> >(p)) ||
        (q = peek< exactly<'>'> >(p)))
    { p = q; }
    
    p = look_for_simple_selector_sequence(p);
    
    if (!p) return 0;
    
    while (((q = peek< exactly<'+'> >(p)) ||
            (q = peek< exactly<'~'> >(p)) ||
            (q = peek< exactly<'>'> >(p)) ||
            (q = peek< ancestor_of > (p))) &&
           (q = look_for_simple_selector_sequence(q)))
    { p = q; }
  
    return p;
  }
  
  const char* Document::look_for_simple_selector_sequence(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;
    
    if ((q = peek< type_selector >(p)) ||
        (q = peek< universal >(p))     ||
        (q = peek< exactly <'&'> >(p)) ||
        (q = look_for_simple_selector(p)))
    { p = q; }
    else
    { return 0; }
    
    while (!peek< spaces >(p) &&
           !(peek < exactly<'+'> >(p) ||
             peek < exactly<'~'> >(p) ||
             peek < exactly<'>'> >(p) ||
             peek < exactly<','> >(p) ||
             peek < exactly<')'> >(p) ||
             peek < exactly<'{'> >(p)) &&
           (q = look_for_simple_selector(p)))
    { p = q; }
    
    return p;
  }
  
  const char* Document::look_for_simple_selector(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;
    (q = peek< id_name >(p)) || (q = peek< class_name >(p)) ||
    (q = look_for_pseudo(p)) || (q = look_for_attrib(p));
    // cerr << "looking for simple selector; found:" << endl;
    // cerr << (q ? string(Token::make(q,q+8)) : "nothing") << endl;
    return q;
  }
  
  const char* Document::look_for_pseudo(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;
    
    if (q = peek< pseudo_not >(p)) {
      // (q = look_for_simple_selector(q)) && (q = peek< exactly<')'> >(q));
      (q = look_for_selector_group(q)) && (q = peek< exactly<')'> >(q));
    }
    else if (q = peek< sequence< pseudo_prefix, functional > >(p)) {
      p = q;
      (q = peek< alternatives< even, odd > >(p)) ||
      (q = peek< binomial >(p))                  ||
      (q = peek< sequence< optional<sign>,
                           optional<digits>,
                           exactly<'n'> > >(p))  ||
      (q = peek< sequence< optional<sign>,
                           digits > >(p));
      p = q;
      q = peek< exactly<')'> >(p);
    }
    else {
      q = peek< sequence< pseudo_prefix, identifier > >(p);
    }
    return q ? q : 0;
  }
    
  const char* Document::look_for_attrib(const char* start)
  {
    const char* p = start ? start : position;
    
    (p = peek< exactly<'['> >(p))                  &&
    (p = peek< type_selector >(p))                 &&
    (p = peek< alternatives<exact_match,
                            class_match,
                            dash_match,
                            prefix_match,
                            suffix_match,
                            substring_match> >(p)) &&
    (p = peek< string_constant >(p))               &&
    (p = peek< exactly<']'> >(p));
    
    return p;
  }
}