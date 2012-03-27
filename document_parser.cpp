#include "document.hpp"
#include "evaluator.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  void Document::parse_scss(bool definition)
  {
    lex<optional_spaces>();
    while(*position) {
      if (lex< block_comment >()) {
        root << Node(line_number, Node::comment, lexed);
      }
      else if (peek< import >(position)) {
        root += parse_import(definition);
        lex< exactly<';'> >();
      }
      else if (peek< mixin >(position)) {
        // TO DO: check to see if we're already inside a definition in order
        // to disallow definitions that are sneakily nested via imports.
        if (!definition) context.pending.push_back(parse_mixin_definition());
      }
      else if (peek< include >(position)) {
        Node call(parse_mixin_call());
        root << call;
        root.has_rules_or_comments |= call.has_rules_or_comments;
        root.has_rulesets          |= call.has_rulesets;
        root.has_propsets          |= call.has_propsets;
        lex< exactly<';'> >();
        if (!definition) context.pending.push_back(call);
      }
      else if (peek< variable >(position)) {
        Node assn(parse_assignment());
        lex< exactly<';'> >();
        if (!definition) {
          context.pending.push_back(assn);
        }
        else {
          root << assn;
        }
      }
      else {
        root << parse_ruleset(definition);
      }
      lex<optional_spaces>();
    }
  }

  Node Document::parse_import(bool definition)
  {
    lex< import >();
    lex< string_constant >();
    string import_path(lexed.unquote());
    const char* curr_path_start = path.c_str();
    const char* curr_path_end   = folders(curr_path_start);
    string current_path(curr_path_start, curr_path_end - curr_path_start);
    Document importee(current_path + import_path, context);
    importee.parse_scss(definition);
    return importee.root;
  }

  Node Document::parse_mixin_definition()
  {
    lex< mixin >();
    lex< identifier >();
    Node name(line_number, Node::identifier, lexed);
    Node params(parse_mixin_parameters());
    Node body(parse_block(true));
    Node mixin(line_number, Node::mixin, 3);
    mixin << name << params << body;
    // 
    // cerr << "parsed mixin definition: ";
    // cerr << string(mixin[0].token) << "(";
    // if (params.size() > 0) {
    //   cerr << string(params[0].token);
    //   for (int i = 1; i < params.size(); ++i) {
    //     cerr << ", " << string(params[i].token);
    //   }
    // }
    // cerr << ")" << endl;
    // 
    return mixin;
  }

  Node Document::parse_mixin_parameters()
  {
    Node params(line_number, Node::parameters);
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
    Node var(line_number, Node::variable, lexed);
    if (lex< exactly<':'> >()) { // default value
      Node val(parse_space_list());
      Node par_and_val(line_number, Node::assignment, 2);
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
    Node name(line_number, Node::identifier, lexed);
    Node args(parse_mixin_arguments());
    Node call(line_number, Node::mixin_expansion, 2);
    call << name << args;
    return call;
  }
  
  Node Document::parse_mixin_arguments()
  {
    Node args(line_number, Node::arguments);
    lex< exactly<'('> >();
    if (!peek< exactly<')'> >(position)) {
      args << parse_argument();
      while (lex< exactly<','> >()) {
        args << parse_argument();
      }
    }
    lex< exactly<')'> >();
    return args;
  }
  
  Node Document::parse_argument()
  {

    if (peek< sequence < variable, spaces_and_comments, exactly<':'> > >()) {
      lex< variable >();
      Node var(line_number, Node::variable, lexed);
      lex< exactly<':'> >();
      Node val(parse_space_list());
      Node assn(line_number, Node::assignment, 2);
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
    Node var(line_number, Node::variable, lexed);
    lex< exactly<':'> >();
    Node val(parse_list());
    // val.from_variable = true;
    // val.eval_me = true;
    Node assn(line_number, Node::assignment, 2);
    assn << var << val;
    return assn;
    
    // Node evaled(eval(val));
    // evaled.from_variable = true;
    // val.eval_me = true;
    // context.environment[key] = evaled;
  }

  Node Document::parse_ruleset(bool definition)
  {
    Node ruleset(line_number, Node::ruleset, 2);
    ruleset << parse_selector_group();
    ruleset << parse_block(definition);
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
             peek < exactly<')'> >(position) ||
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
    if (lex< pseudo_not >()) {
      Node ps_not(line_number, Node::pseudo_negation, 2);
      ps_not << Node(line_number, Node::value, lexed);
      ps_not << parse_selector_group();
      lex< exactly<')'> >();
      return ps_not;
    }
    else if (lex< sequence< pseudo_prefix, functional > >()) {
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

  Node Document::parse_block(bool definition)
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
      else if (peek< import >(position)) {
        Node imported_tree(parse_import());
        for (int i = 0; i < imported_tree.size(); ++i) {
          if (imported_tree[i].type == Node::comment ||
              imported_tree[i].type == Node::rule) {
            block.has_rules_or_comments = true;
          }
          else if (imported_tree[i].type == Node::ruleset) {
            block.has_rulesets = true;
          }
          block << imported_tree[i];
        }
        semicolon = true;
      }
      else if (peek< include >(position)) {
        Node call(parse_mixin_call());
        block << call;
        block.has_rules_or_comments |= call.has_rules_or_comments;
        block.has_rulesets          |= call.has_rulesets;
        block.has_propsets          |= call.has_propsets;
        semicolon = true;
        if (!definition) context.pending.push_back(call);
      }
      else if (lex< variable >()) {
        Node assn(parse_assignment());
        semicolon = true;
        if (!definition) context.pending.push_back(assn);
      }
      // else if (look_for_rule(position)) {
      //   block << parse_rule();
      //   block.has_rules_or_comments = true;
      //   semicolon = true;
      // }
      // else if (!peek< exactly<';'> >()) {
      //   block << parse_ruleset();
      //   block.has_rulesets = true;
      // }
      else if (const char* p = look_for_selector_group(position)) {
        block << parse_ruleset(definition);
        block.has_rulesets = true;
      }
      else if (!peek< exactly<';'> >()) {
        Node rule(parse_rule());
        block << rule;
        block.has_rules_or_comments = true;
        semicolon = true;        
        if (!definition && rule[1].eval_me) context.pending.push_back(rule);
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
    // rule << parse_values();
    rule << parse_list();
    return rule;
  }
  
  Node Document::parse_list()
  {
    Node val(parse_comma_list());
    // if (val.type != Node::comma_list && val.type != Node::space_list && val.eval_me) {
    //   return eval(val);
    // }
    // else if (val.type == Node::comma_list || val.type == Node::space_list) {
    //   for (int i = 0; i < val.children->size(); ++i) {
    //     if (val.children->at(i).eval_me) {
    //       val.children->at(i) = eval(val.children->at(i));
    //     }
    //   }
    // }
    return val;
  }
  
  Node Document::parse_comma_list()
  {
    if (peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<')'> >(position)) {
      return Node(line_number, Node::nil);
    }
    Node list1(parse_space_list());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< exactly<','> >(position)) return list1;
    
    Node comma_list(line_number, Node::comma_list, 2);
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
    
    Node space_list(line_number, Node::space_list, 2);
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
          peek< exactly<'-'> >(position)))
    { return term1; }
    
    Node expression(line_number, Node::expression, 3);
    term1.eval_me = true;
    expression << term1;
    
    while (lex< exactly<'+'> >() || lex< exactly<'-'> >()) {
      if (lexed.begin[0] == '+') {
        expression << Node(line_number, Node::add, lexed);
      }
      else {
        expression << Node(line_number, Node::sub, lexed);
      }
      Node term(parse_term());
      term.eval_me = true;
      expression << term;
    }
    
    expression.eval_me = true;
    // for (int i = 0; i < expression.children->size(); ++i) {
    //   Node::Type t = expression.children->at(i).type;
    //   if (t == Node::string_constant ||
    //       t == Node::uri             ||
    //       t == Node::identifier      ||
    //       t == Node::comma_list      ||
    //       t == Node::space_list      ||
    //       t == Node::nil             ||
    //       t == Node::expression && !expression.children->at(i).eval_me)
    //   { expression.eval_me = false; }
    // }
    return expression;
  }
  
  Node Document::parse_term()
  {
    Node fact1(parse_factor());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'*'> >(position) ||
          peek< exactly<'/'> >(position)))
    { return fact1; }

    Node term(line_number, Node::term, 3);
    term << fact1;
    if (fact1.eval_me) term.eval_me = true;

    while (lex< exactly<'*'> >() || lex< exactly<'/'> >()) {
      if (lexed.begin[0] == '*') {
        term << Node(line_number, Node::mul, lexed);
        term.eval_me = true;
      }
      else {
        term << Node(line_number, Node::div, lexed);
      }
      Node fact(parse_factor());
      if (fact.eval_me) term.eval_me = true;
      term << fact;
    }

    // if (term.eval_me) {
    //   for (int i = 0; i < term.children->size(); i += 2) {
    //     term.children->at(i).eval_me = true;
    //   }
    // }
    return term;
  }
  
  Node Document::parse_factor()
  {
    if (lex< exactly<'('> >()) {
      // Node value(parse_list());
      Node value(parse_comma_list());
      value.eval_me = true;
      if (value.type == Node::comma_list || value.type == Node::space_list) {
        value.children->front().eval_me = true;
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
      Node result(line_number, Node::uri, lexed);
      lex< exactly<')'> >();
      return result;
    }
    
    if (lex< rgb_prefix >())
    {
      Node result(line_number, Node::hex_triple, 3);
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
    { return Node(line_number, Node::identifier, lexed); }

    if (lex< percentage >())
    { return Node(line_number, Node::textual_percentage, lexed); }

    if (lex< dimension >())
    { return Node(line_number, Node::textual_dimension, lexed); }

    if (lex< number >())
    { return Node(line_number, Node::textual_number, lexed); }

    if (lex< hex >())
    { return Node(line_number, Node::textual_hex, lexed); }

    if (lex< string_constant >())
    { return Node(line_number, Node::string_constant, lexed); }

    if (lex< variable >())
    {
      Node var(line_number, Node::variable, lexed);
      var.eval_me = true;
      return var;
    }
  }
  
  Node Document::parse_identifier() {
    lex< identifier >();
    return Node(line_number, Node::identifier, lexed);
  }
  
  Node Document::parse_variable() {
    lex< variable >();
    return Node(line_number, Node::variable, lexed);
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
    // cerr << (q ? string(Token(q,q+8)) : "nothing") << endl;
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