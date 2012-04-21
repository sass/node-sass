#include "document.hpp"
#include "error.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  extern const char plus_equal[] = "+=";

  void Document::parse_scss()
  {
    lex<optional_spaces>();
    root << Node(Node::flags);
    while(position < end) {
      if (lex< block_comment >()) {
        root << Node(Node::comment, line_number, lexed);
      }
      else if (peek< import >(position)) {
        // TO DO: don't splice in place at parse-time -- use an expansion node
        Node import(parse_import());
        if (import.type == Node::css_import) {
          root << import;
        }
        else {
          root += import;
        }
        if (!lex< exactly<';'> >()) syntax_error("top-level @import directive must be terminated by ';'");
      }
      else if (peek< mixin >(position) || peek< exactly<'='> >(position)) {
        root << parse_mixin_definition();
      }
      else if (peek< include >(position)) {
        root << parse_mixin_call();
        root[0].has_expansions = true;
        if (!lex< exactly<';'> >()) syntax_error("top-level @include directive must be terminated by ';'");
      }
      else if (peek< variable >(position)) {        
        root << parse_assignment();
        if (!lex< exactly<';'> >()) syntax_error("top-level variable binding must be terminated by ';'");
      }
      else if (look_for_selector_group(position)) {
        root << parse_ruleset();
      }
      else if (peek< exactly<'+'> >()) {
        root << parse_mixin_call();
        root[0].has_expansions = true;
        if (!lex< exactly<';'> >()) syntax_error("top-level @include directive must be terminated by ';'");
      }
      lex<optional_spaces>();
    }
  }

  Node Document::parse_import()
  {
    lex< import >();
    if (lex< uri_prefix >())
    {
      const char* beg = position;
      const char* end = find_first< exactly<')'> >(position);
      Node result(Node::css_import, line_number, Token::make(beg, end));
      position = end;
      lex< exactly<')'> >();
      return result;
    }
    if (!lex< string_constant >()) syntax_error("@import directive requires a url or quoted path");
    // TO DO: BETTER PATH HANDLING
    string import_path(lexed.unquote());
    const char* curr_path_start = path.c_str();
    const char* curr_path_end   = folders(curr_path_start);
    string current_path(curr_path_start, curr_path_end - curr_path_start);
    try {
      Document importee(current_path + import_path, context);
      importee.parse_scss();
      return importee.root;
    }
    catch (string& path) {
      read_error("error reading file \"" + path + "\"");
    }
  }

  Node Document::parse_mixin_definition()
  {
    lex< mixin >() || lex< exactly<'='> >();
    if (!lex< identifier >()) syntax_error("invalid name in @mixin directive");
    Node name(Node::identifier, line_number, lexed);
    Node params(parse_mixin_parameters());
    if (!peek< exactly<'{'> >()) syntax_error("body for mixin " + name.content.token.to_string() + " must begin with a '{'");
    Node body(parse_block(true));
    Node mixin(Node::mixin, context.registry, line_number, 3);
    mixin << name << params << body;
    return mixin;
  }

  Node Document::parse_mixin_parameters()
  {
    Node params(Node::parameters, context.registry, line_number);
    Token name(lexed);
    if (lex< exactly<'('> >()) {
      if (peek< variable >()) {
        params << parse_parameter();
        while (lex< exactly<','> >()) {
          if (!peek< variable >()) syntax_error("expected a variable name (e.g. $x) for the parameter list for " + name.to_string());
          params << parse_parameter();
        }
        if (!lex< exactly<')'> >()) syntax_error("parameter list for " + name.to_string() + " requires a ')'");
      }
      else if (!lex< exactly<')'> >()) syntax_error("expected a variable name (e.g. $x) or ')' for the parameter list for " + name.to_string());
    }
    return params;
  }

  Node Document::parse_parameter() {
    lex< variable >();
    Node var(Node::variable, line_number, lexed);
    if (lex< exactly<':'> >()) { // default value
      Node val(parse_space_list());
      Node par_and_val(Node::assignment, context.registry, line_number, 2);
      par_and_val << var << val;
      return par_and_val;
    }
    else {
      return var;
    }
  }

  Node Document::parse_mixin_call()
  {
    lex< include >() || lex< exactly<'+'> >();
    if (!lex< identifier >()) syntax_error("invalid name in @include directive");
    Node name(Node::identifier, line_number, lexed);
    Node args(parse_arguments());
    Node call(Node::expansion, context.registry, line_number, 3);
    call << name << args;
    return call;
  }
  
  Node Document::parse_arguments()
  {
    Token name(lexed);
    Node args(Node::arguments, context.registry, line_number);
    if (lex< exactly<'('> >()) {
      if (!peek< exactly<')'> >(position)) {
        args << parse_argument();
        args.content.children->back().eval_me = true;
        while (lex< exactly<','> >()) {
          args << parse_argument();
          args.content.children->back().eval_me = true;
        }
      }
      if (!lex< exactly<')'> >()) syntax_error("improperly terminated argument list for " + name.to_string());
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
      Node assn(Node::assignment, context.registry, line_number, 2);
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
    if (!lex< exactly<':'> >()) syntax_error("expected ':' after " + lexed.to_string() + " in assignment statement");
    Node val(parse_list());
    Node assn(Node::assignment, context.registry, line_number, 2);
    assn << var << val;
    return assn;
  }

  Node Document::parse_ruleset(bool definition)
  {
    Node ruleset(Node::ruleset, context.registry, line_number, 2);
    ruleset << parse_selector_group();
    // if (ruleset[0].type == Node::selector) cerr << "ruleset starts with selector" << endl;
    // if (ruleset[0].type == Node::selector_group) cerr << "ruleset starts with selector_group" << endl;
    if (!peek< exactly<'{'> >()) syntax_error("expected a '{' after the selector");
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
    if (!peek< exactly<','> >()) return sel1;
    
    Node group(Node::selector_group, context.registry, line_number, 2);
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
    if (peek< exactly<','> >() ||
        peek< exactly<')'> >() ||
        peek< exactly<'{'> >()) return seq1;
    
    Node selector(Node::selector, context.registry, line_number, 2);
    if (seq1.has_backref) selector.has_backref = true;
    selector << seq1;

    while (!peek< exactly<'{'> >() && !peek< exactly<','> >()) {
      Node seq(parse_simple_selector_sequence());
      if (seq.has_backref) selector.has_backref = true;
      selector << seq;
    }
    return selector;
  }

  Node Document::parse_simple_selector_sequence()
  {
    // check for initial and trailing combinators
    if (lex< exactly<'+'> >() ||
        lex< exactly<'~'> >() ||
        lex< exactly<'>'> >())
    { return Node(Node::selector_combinator, line_number, lexed); }
    
    // check for backref or type selector, which are only allowed at the front
    Node simp1;
    bool saw_backref = false;
    if (lex< exactly<'&'> >()) {
      simp1 = Node(Node::backref, line_number, lexed);
      simp1.has_backref = true;
      saw_backref = true;
    }
    else if (lex< alternatives< type_selector, universal > >()) {
      simp1 = Node(Node::simple_selector, line_number, lexed);
    }
    else {
      simp1 = parse_simple_selector();
    }
    
    // now we have one simple/atomic selector -- see if there are more
    if (peek< spaces >()       || peek< exactly<'>'> >() ||
        peek< exactly<'+'> >() || peek< exactly<'~'> >() ||
        peek< exactly<','> >() || peek< exactly<')'> >() ||
        peek< exactly<'{'> >())
    { return simp1; }

    // now we know we have a sequence of simple selectors
    Node seq(Node::simple_selector_sequence, context.registry, line_number, 2);
    seq << simp1;
    seq.has_backref = saw_backref;
    
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
    
    // 
    // Node seq(Node::simple_selector_sequence, line_number, 1);
    // if (lex< alternatives < type_selector, universal > >()) {
    //   seq << Node(Node::simple_selector, line_number, lexed);
    // }
    // else if (lex< exactly<'&'> >()) {
    //   seq << Node(Node::backref, line_number, lexed);
    //   seq.has_backref = true;
    //   // if (peek< sequence< no_spaces, alternatives< type_selector, universal > > >(position)) {
    //   //   seq.terminal_backref = true;
    //   //   return seq;
    //   // }
    // }
    // else {
    //   seq << parse_simple_selector();
    // }
    // while (!peek< spaces >(position) &&
    //        !(peek < exactly<'+'> >(position) ||
    //          peek < exactly<'~'> >(position) ||
    //          peek < exactly<'>'> >(position) ||
    //          peek < exactly<','> >(position) ||
    //          peek < exactly<')'> >(position) ||
    //          peek < exactly<'{'> >(position))) {
    //   seq << parse_simple_selector();
    // }
    // return seq; 
  }
  
  Node Document::parse_selector_combinator()
  {
    lex< exactly<'+'> >() || lex< exactly<'~'> >() ||
    lex< exactly<'>'> >() || lex< ancestor_of >();
    return Node(Node::selector_combinator, line_number, lexed);
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
    else {
      syntax_error("invalid selector after " + lexed.to_string());
    }
  }
  
  Node Document::parse_pseudo() {
    if (lex< pseudo_not >()) {
      Node ps_not(Node::pseudo_negation, context.registry, line_number, 2);
      ps_not << Node(Node::value, line_number, lexed);
      ps_not << parse_selector_group();
      lex< exactly<')'> >();
      return ps_not;
    }
    else if (lex< sequence< pseudo_prefix, functional > >()) {
      Node pseudo(Node::functional_pseudo, context.registry, line_number, 2);
      Token name(lexed);
      pseudo << Node(Node::value, line_number, name);
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
      else {
        syntax_error("invalid argument to " + name.to_string() + "...)");
      }
      if (!lex< exactly<')'> >()) syntax_error("unterminated argument to " + name.to_string() + "...)");
      return pseudo;
    }
    else if (lex < sequence< pseudo_prefix, identifier > >()) {
      return Node(Node::pseudo, line_number, lexed);
    }
    else {
      syntax_error("unrecognized pseudo-class or pseudo-element");
    }
  }
  
  Node Document::parse_attribute_selector()
  {
    Node attr_sel(Node::attribute_selector, context.registry, line_number, 3);
    lex< exactly<'['> >();
    if (!lex< type_selector >()) syntax_error("invalid attribute name in attribute selector");
    Token name(lexed);
    attr_sel << Node(Node::value, line_number, lexed);
    if (!lex< alternatives< exact_match, class_match, dash_match,
                            prefix_match, suffix_match, substring_match > >()) {
      syntax_error("invalid operator in attribute selector for " + name.to_string());
    }
    attr_sel << Node(Node::value, line_number, lexed);
    if (!lex< string_constant >()) syntax_error("expected a quoted string constant in attribute selector for " + name.to_string());
    attr_sel << Node(Node::value, line_number, lexed);
    if (!lex< exactly<']'> >()) syntax_error("unterminated attribute selector for " + name.to_string());
    return attr_sel;
  }

  Node Document::parse_block(bool definition)
  {
    lex< exactly<'{'> >();
    bool semicolon = false;
    Node block(Node::block, context.registry, line_number, 1);
    block << Node(Node::flags);
    while (!lex< exactly<'}'> >()) {
      if (semicolon) {
        if (!lex< exactly<';'> >()) syntax_error("non-terminal statement or declaration must end with ';'");
        semicolon = false;
        while (lex< block_comment >()) {
          block << Node(Node::comment, line_number, lexed);
          block[0].has_statements = true;
        }
        if (lex< exactly<'}'> >()) break;
      }
      if (lex< block_comment >()) {
        block << Node(Node::comment, line_number, lexed);
        block[0].has_statements = true;
        //semicolon = true;
      }
      else if (peek< import >(position)) {
        if (definition) {
          lex< import >(); // to adjust the line number
          syntax_error("@import directive not allowed inside mixin definition");
        }
        Node imported_tree(parse_import());
        if (imported_tree.type == Node::css_import) {
          cerr << "css import inside block" << endl;
          block << imported_tree;
          block.has_statements = true;
        }
        else {
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
      }
      else if (peek< include >(position)) {
        block << parse_mixin_call();
        block[0].has_expansions = true;
        semicolon = true;
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
      else if (peek< exactly<'+'> >()) {
        block << parse_mixin_call();
        block[0].has_expansions = true;
        semicolon = true;
      }
      else if (!peek< exactly<';'> >()) {
        block << parse_rule();
        block[0].has_statements = true;
        semicolon = true;
        //lex< exactly<';'> >(); // TO DO: clean up the semicolon handling stuff
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
    Node rule(Node::rule, context.registry, line_number, 2);
    if (!lex< identifier >()) syntax_error("invalid property name");
    rule << Node(Node::property, line_number, lexed);
    if (!lex< exactly<':'> >()) syntax_error("property \"" + lexed.to_string() + "\" must be followed by a ':'");
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
        peek< exactly<'{'> >(position) ||
        peek< exactly<')'> >(position))
    { return Node(Node::nil, context.registry, line_number); }
    Node list1(parse_space_list());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< exactly<','> >(position)) return list1;
    
    Node comma_list(Node::comma_list, context.registry, line_number, 2);
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
    Node disj1(parse_disjunction());
    // if it's a singleton, return it directly; don't wrap it
    if (peek< exactly<';'> >(position) ||
        peek< exactly<'}'> >(position) ||
        peek< exactly<'{'> >(position) ||
        peek< exactly<')'> >(position) ||
        peek< exactly<','> >(position))
    { return disj1; }
    
    Node space_list(Node::space_list, context.registry, line_number, 2);
    space_list << disj1;
    space_list.eval_me |= disj1.eval_me;
    
    while (!(peek< exactly<';'> >(position) ||
             peek< exactly<'}'> >(position) ||
             peek< exactly<'{'> >(position) ||
             peek< exactly<')'> >(position) ||
             peek< exactly<','> >(position)))
    {
      Node disj(parse_disjunction());
      space_list << disj;
      space_list.eval_me |= disj.eval_me;
    }
    
    return space_list;
  }
  
  Node Document::parse_disjunction()
  {
    Node conj1(parse_conjunction());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< sequence< or_kwd, negate< identifier > > >()) return conj1;
    
    Node disjunction(Node::disjunction, context.registry, line_number, 2);
    disjunction << conj1;
    while (lex< sequence< or_kwd, negate< identifier > > >()) disjunction << parse_conjunction();
    disjunction.eval_me = true;
    
    return disjunction;
  }
  
  Node Document::parse_conjunction()
  {
    Node rel1(parse_relation());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< sequence< and_kwd, negate< identifier > > >()) return rel1;
    
    Node conjunction(Node::conjunction, context.registry, line_number, 2);
    conjunction << rel1;
    while (lex< sequence< and_kwd, negate< identifier > > >()) conjunction << parse_relation();
    conjunction.eval_me = true;
    return conjunction;
  }
  
  Node Document::parse_relation()
  {
    Node expr1(parse_expression());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< eq_op >(position)  ||
          peek< neq_op >(position) ||
          peek< gt_op >(position)  ||
          peek< gte_op >(position) ||
          peek< lt_op >(position)  ||
          peek< lte_op >(position)))
    { return expr1; }
    
    Node relation(Node::relation, context.registry, line_number, 3);
    expr1.eval_me = true;
    relation << expr1;
        
    if (lex< eq_op >()) relation << Node(Node::eq, line_number, lexed);
    else if (lex< neq_op >()) relation << Node(Node::neq, line_number, lexed);
    else if (lex< gte_op >()) relation << Node(Node::gte, line_number, lexed);
    else if (lex< lte_op >()) relation << Node(Node::lte, line_number, lexed);
    else if (lex< gt_op >()) relation << Node(Node::gt, line_number, lexed);
    else if (lex< lt_op >()) relation << Node(Node::lt, line_number, lexed);
        
    Node expr2(parse_expression());
    expr2.eval_me = true;
    relation << expr2;
    
    relation.eval_me = true;
    return relation;
  }
  
  Node Document::parse_expression()
  {
    Node term1(parse_term());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'+'> >(position) ||
          peek< sequence< negate< number >, exactly<'-'> > >(position)))
    { return term1; }
    
    Node expression(Node::expression, context.registry, line_number, 3);
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

    Node term(Node::term, context.registry, line_number, 3);
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
      if (!lex< exactly<')'> >()) syntax_error("unclosed parenthesis");
      return value;
    }
    else if (lex< sequence< exactly<'+'>, negate< number > > >()) {
      Node plus(Node::unary_plus, context.registry, line_number, 1);
      plus << parse_factor();
      plus.eval_me = true;
      return plus;
    }
    else if (lex< sequence< exactly<'-'>, negate< number> > >()) {
      Node minus(Node::unary_minus, context.registry, line_number, 1);
      minus << parse_factor();
      minus.eval_me = true;
      return minus;
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
    
    if (lex< value_schema >())
    {
      cerr << "parsing value schema: " << lexed.to_string() << endl;
      
      Document schema_doc(path, line_number, lexed, context);
      return schema_doc.parse_value_schema();
    }
    
    if (lex< sequence< true_kwd, negate< identifier > > >())
    {
      Node T(Node::boolean);
      T.line_number = line_number;
      T.content.boolean_value = true;
      return T;
    }
    
    if (lex< sequence< false_kwd, negate< identifier > > >())
    {
      Node F(Node::boolean);
      F.line_number = line_number;
      F.content.boolean_value = false;
      return F;
    }
    
    if (peek< functional >())
    { return parse_function_call(); }
    
    if (lex< important >())
    { return Node(Node::important, line_number, lexed); }

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

    if (peek< string_constant >())
    // { return Node(Node::string_constant, line_number, lexed); }
    { return parse_string(); } 

    if (lex< variable >())
    {
      Node var(Node::variable, line_number, lexed);
      var.eval_me = true;
      return var;
    }
    
    syntax_error("error reading values after " + lexed.to_string());
  }
  
  extern const char hash_lbrace[] = "#{";
  extern const char rbrace[] = "}";
  
  Node Document::parse_string()
  {    
    lex< string_constant >();
    Token str(lexed);
    const char* i = str.begin;
    // see if there any interpolants
    const char* p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(str.begin, str.end);
    if (!p) {
      return Node(Node::string_constant, line_number, str);
    }
    
    Node schema(Node::string_schema, context.registry, line_number, 1);
    while (i < str.end) {
      if (p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, str.end)) {
        if (i < p) schema << Node(Node::identifier, line_number, Token::make(i, p)); // accumulate the preceding segment if it's nonempty
        const char* j = find_first_in_interval< exactly<rbrace> >(p, str.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          Document interp_doc(path, line_number, Token::make(p+2,j-1), context);
          Node interp_node(interp_doc.parse_list());
          interp_node.eval_me = true;
          schema << interp_node;
          i = j + 1;
        }
        else {
          // throw an error if the interpolant is unterminated
          syntax_error("unterminated interpolant inside string constant " + str.to_string());
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < str.end) schema << Node(Node::identifier, line_number, Token::make(i, str.end));
        break;
      }
    }
    return schema;
  }
  
  Node Document::parse_value_schema()
  {    
    Node schema(Node::value_schema, context.registry, line_number, 1);
    
    while (position < end) {
      if (lex< interpolant >()) {
        Token insides(Token::make(lexed.begin + 2, lexed.end - 1));
        Document interp_doc(path, line_number, insides, context);
        Node interp_node(interp_doc.parse_list());
        schema << interp_node;
      }
      else if (lex< identifier >()) {
        schema << Node(Node::identifier, line_number, lexed);
      }
      else if (lex< percentage >()) {
        schema << Node(Node::textual_percentage, line_number, lexed);
      }
      else if (lex< dimension >()) {
        schema << Node(Node::textual_dimension, line_number, lexed);
      }
      else if (lex< number >()) {
        schema << Node(Node::textual_number, line_number, lexed);
      }
      else if (lex< hex >()) {
        schema << Node(Node::textual_hex, line_number, lexed);
      }
      else if (lex< string_constant >()) {
        schema << Node(Node::string_constant, line_number, lexed);
      }
      else if (lex< variable >()) {
        schema << Node(Node::variable, line_number, lexed);
      }
      else {
        syntax_error("error parsing interpolated value");
      }
    }
    schema.eval_me = true;
    return schema;
  }
  
  Node Document::parse_function_call()
  {
    lex< identifier >();
    Node name(Node::identifier, line_number, lexed);
    Node args(parse_arguments());
    Node call(Node::function_call, context.registry, line_number, 2);
    call << name << args;
    call.eval_me = true;
    return call;
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