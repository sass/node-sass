#include "document.hpp"
#include "error.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  void Document::parse_scss()
  {
    lex< optional_spaces >();
    Selector_Lookahead lookahead_result;
    while (position < end) {
      if (lex< block_comment >()) {
        root << context.new_Node(Node::comment, path, line, lexed);
      }
      else if (peek< import >()) {
        Node importee(parse_import());
        if (importee.type() == Node::css_import) root << importee;
        else                                     root += importee;
        if (!lex< exactly<';'> >()) throw_syntax_error("top-level @import directive must be terminated by ';'");
      }
      else if (peek< mixin >() || peek< exactly<'='> >()) {
        root << parse_mixin_definition();
      }
      else if (peek< variable >()) {
        root << parse_assignment();
        if (!lex< exactly<';'> >()) throw_syntax_error("top-level variable binding must be terminated by ';'");
      }
      else if (peek< sequence< identifier, optional_spaces, exactly<':'>, optional_spaces, exactly<'{'> > >(position)) {
        root << parse_propset();
      }
      else if ((lookahead_result = lookahead_for_selector(position)).found) {
        root << parse_ruleset();
      }
      else if (peek< include >() || peek< exactly<'+'> >()) {
        root << parse_mixin_call();
        if (!lex< exactly<';'> >()) throw_syntax_error("top-level @include directive must be terminated by ';'");
      }
      else {
        lex< spaces_and_comments >();
        throw_syntax_error("invalid top-level expression");
      }
      lex< optional_spaces >();
    }
  }

  Node Document::parse_import()
  {
    lex< import >();
    if (lex< uri_prefix >())
    {
      if (peek< string_constant >()) {
        Node schema(parse_string());
        Node importee(context.new_Node(Node::css_import, path, line, 1));
        importee << schema;
        if (!lex< exactly<')'> >()) throw_syntax_error("unterminated url in @import directive");
        return importee;
      }
      else {
        const char* beg = position;
        const char* end = find_first< exactly<')'> >(position);
        if (!end) throw_syntax_error("unterminated url in @import directive");
        Node path_node(context.new_Node(Node::identifier, path, line, Token::make(beg, end)));
        Node importee(context.new_Node(Node::css_import, path, line, 1));
        importee << path_node;
        position = end;
        lex< exactly<')'> >();
        return importee;
      }
    }
    if (!lex< string_constant >()) throw_syntax_error("@import directive requires a url or quoted path");
    // TO DO: BETTER PATH HANDLING
    string import_path(lexed.unquote());
    const char* curr_path_start = path.c_str();
    const char* curr_path_end   = folders(curr_path_start);
    string current_path(curr_path_start, curr_path_end - curr_path_start);
    try {
      Document importee(Document::make_from_file(context, current_path + import_path));
      importee.parse_scss();
      return importee.root;
    }
    catch (string& path) {
      throw_read_error("error reading file \"" + path + "\"");
    }
    // unreached statement
    return Node();
  }

  Node Document::parse_mixin_definition()
  {
    lex< mixin >() || lex< exactly<'='> >();
    if (!lex< identifier >()) throw_syntax_error("invalid name in @mixin directive");
    Node name(context.new_Node(Node::identifier, path, line, lexed));
    Node params(parse_mixin_parameters());
    if (!peek< exactly<'{'> >()) throw_syntax_error("body for mixin " + name.token().to_string() + " must begin with a '{'");
    Node body(parse_block(true));
    Node the_mixin(context.new_Node(Node::mixin, path, line, 3));
    the_mixin << name << params << body;
    return the_mixin;
  }

  Node Document::parse_mixin_parameters()
  {
    Node params(context.new_Node(Node::parameters, path, line, 0));
    Token name(lexed);
    if (lex< exactly<'('> >()) {
      if (peek< variable >()) {
        params << parse_parameter();
        while (lex< exactly<','> >()) {
          if (!peek< variable >()) throw_syntax_error("expected a variable name (e.g. $x) for the parameter list for " + name.to_string());
          params << parse_parameter();
        }
        if (!lex< exactly<')'> >()) throw_syntax_error("parameter list for " + name.to_string() + " requires a ')'");
      }
      else if (!lex< exactly<')'> >()) throw_syntax_error("expected a variable name (e.g. $x) or ')' for the parameter list for " + name.to_string());
    }
    return params;
  }

  Node Document::parse_parameter() {
    lex< variable >();
    Node var(context.new_Node(Node::variable, path, line, lexed));
    if (lex< exactly<':'> >()) { // default value
      Node val(parse_space_list());
      Node par_and_val(context.new_Node(Node::assignment, path, line, 2));
      par_and_val << var << val;
      return par_and_val;
    }
    else {
      return var;
    }
    // unreachable statement
    return Node();
  }

  Node Document::parse_mixin_call()
  {
    lex< include >() || lex< exactly<'+'> >();
    if (!lex< identifier >()) throw_syntax_error("invalid name in @include directive");
    Node name(context.new_Node(Node::identifier, path, line, lexed));
    Node args(parse_arguments());
    Node the_call(context.new_Node(Node::expansion, path, line, 2));
    the_call << name << args;
    return the_call;
  }
  
  Node Document::parse_arguments()
  {
    Token name(lexed);
    Node args(context.new_Node(Node::arguments, path, line, 0));
    if (lex< exactly<'('> >()) {
      if (!peek< exactly<')'> >(position)) {
        Node arg(parse_argument());
        arg.should_eval() = true;
        args << arg;
        while (lex< exactly<','> >()) {
          Node arg(parse_argument());
          arg.should_eval() = true;
          args << arg;
        }
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("improperly terminated argument list for " + name.to_string());
    }
    return args;
  }
  
  Node Document::parse_argument()
  {
    if (peek< sequence < variable, spaces_and_comments, exactly<':'> > >()) {
      lex< variable >();
      Node var(context.new_Node(Node::variable, path, line, lexed));
      lex< exactly<':'> >();
      Node val(parse_space_list());
      Node assn(context.new_Node(Node::assignment, path, line, 2));
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
    Node var(context.new_Node(Node::variable, path, line, lexed));
    if (!lex< exactly<':'> >()) throw_syntax_error("expected ':' after " + lexed.to_string() + " in assignment statement");
    Node val(parse_list());
    Node assn(context.new_Node(Node::assignment, path, line, 2));
    assn << var << val;
    return assn;
  }
  
  Node Document::parse_propset()
  {
    lex< identifier >();
    Node property_segment(context.new_Node(Node::identifier, path, line, lexed));
    lex< exactly<':'> >();
    lex< exactly<'{'> >();
    Node block(context.new_Node(Node::block, path, line, 1));
    while (!lex< exactly<'}'> >()) {
      if (peek< sequence< identifier, optional_spaces, exactly<':'>, optional_spaces, exactly<'{'> > >(position)) {
        block << parse_propset();
      }
      else {
        block << parse_rule();
        lex< exactly<';'> >();
      }
    }
    if (block.empty()) throw_syntax_error("namespaced property cannot be empty");
    Node propset(context.new_Node(Node::propset, path, line, 2));
    propset << property_segment;
    propset << block;
    return propset;
  }

  Node Document::parse_ruleset(bool definition)
  {
    Node ruleset(context.new_Node(Node::ruleset, path, line, 2));
    ruleset << parse_selector_group();
    if (!peek< exactly<'{'> >()) throw_syntax_error("expected a '{' after the selector");
    ruleset << parse_block(definition);
    return ruleset;
  }

  Node Document::parse_selector_group()
  {
    Node sel1(parse_selector());
    if (!peek< exactly<','> >()) return sel1;
    
    Node group(context.new_Node(Node::selector_group, path, line, 2));
    group << sel1;
    while (lex< exactly<','> >()) group << parse_selector();
    return group;
  }

  Node Document::parse_selector()
  {
    Node seq1(parse_simple_selector_sequence());
    if (peek< exactly<','> >() ||
        peek< exactly<')'> >() ||
        peek< exactly<'{'> >()) return seq1;
    
    Node selector(context.new_Node(Node::selector, path, line, 2));
    selector << seq1;

    while (!peek< exactly<'{'> >() && !peek< exactly<','> >()) {
      selector << parse_simple_selector_sequence();
    }
    return selector;
  }

  Node Document::parse_simple_selector_sequence()
  {
    // check for initial and trailing combinators
    if (lex< exactly<'+'> >() ||
        lex< exactly<'~'> >() ||
        lex< exactly<'>'> >())
    { return context.new_Node(Node::selector_combinator, path, line, lexed); }
    
    // check for backref or type selector, which are only allowed at the front
    Node simp1;
    if (lex< exactly<'&'> >()) {
      simp1 = context.new_Node(Node::backref, path, line, lexed);
    }
    else if (lex< alternatives< type_selector, universal > >()) {
      simp1 = context.new_Node(Node::simple_selector, path, line, lexed);
    }
    else {
      simp1 = parse_simple_selector();
    }
    
    // now we have one simple/atomic selector -- see if that's all
    if (peek< spaces >()       || peek< exactly<'>'> >() ||
        peek< exactly<'+'> >() || peek< exactly<'~'> >() ||
        peek< exactly<','> >() || peek< exactly<')'> >() ||
        peek< exactly<'{'> >())
    { return simp1; }

    // otherwise, we have a sequence of simple selectors
    Node seq(context.new_Node(Node::simple_selector_sequence, path, line, 2));
    seq << simp1;
    
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
  
  Node Document::parse_selector_combinator()
  {
    lex< exactly<'+'> >() || lex< exactly<'~'> >() ||
    lex< exactly<'>'> >() || lex< ancestor_of >();
    return context.new_Node(Node::selector_combinator, path, line, lexed);
  }
  
  Node Document::parse_simple_selector()
  {
    if (lex< id_name >() || lex< class_name >()) {
      return context.new_Node(Node::simple_selector, path, line, lexed);
    }
    else if (peek< exactly<':'> >(position)) {
      return parse_pseudo();
    }
    else if (peek< exactly<'['> >(position)) {
      return parse_attribute_selector();
    }
    else {
      throw_syntax_error("invalid selector after " + lexed.to_string());
    }
    // unreachable statement
    return Node();
  }
  
  Node Document::parse_pseudo() {
    if (lex< pseudo_not >()) {
      Node ps_not(context.new_Node(Node::pseudo_negation, path, line, 2));
      ps_not << context.new_Node(Node::value, path, line, lexed);
      ps_not << parse_selector_group();
      lex< exactly<')'> >();
      return ps_not;
    }
    else if (lex< sequence< pseudo_prefix, functional > >()) {
      Node pseudo(context.new_Node(Node::functional_pseudo, path, line, 2));
      Token name(lexed);
      pseudo << context.new_Node(Node::value, path, line, name);
      if (lex< alternatives< even, odd > >()) {
        pseudo << context.new_Node(Node::value, path, line, lexed);
      }
      else if (peek< binomial >(position)) {
        lex< coefficient >();
        pseudo << context.new_Node(Node::value, path, line, lexed);
        lex< exactly<'n'> >();
        pseudo << context.new_Node(Node::value, path, line, lexed);
        lex< sign >();
        pseudo << context.new_Node(Node::value, path, line, lexed);
        lex< digits >();
        pseudo << context.new_Node(Node::value, path, line, lexed);
      }
      else if (lex< sequence< optional<sign>,
                              optional<digits>,
                              exactly<'n'> > >()) {
        pseudo << context.new_Node(Node::value, path, line, lexed);
      }
      else if (lex< sequence< optional<sign>, digits > >()) {
        pseudo << context.new_Node(Node::value, path, line, lexed);
      }
      else if (lex< string_constant >()) {
        pseudo << context.new_Node(Node::string_constant, path, line, lexed);
      }
      else {
        throw_syntax_error("invalid argument to " + name.to_string() + "...)");
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("unterminated argument to " + name.to_string() + "...)");
      return pseudo;
    }
    else if (lex < sequence< pseudo_prefix, identifier > >()) {
      return context.new_Node(Node::pseudo, path, line, lexed);
    }
    else {
      throw_syntax_error("unrecognized pseudo-class or pseudo-element");
    }
    // unreachable statement
    return Node();
  }
  
  Node Document::parse_attribute_selector()
  {
    Node attr_sel(context.new_Node(Node::attribute_selector, path, line, 3));
    lex< exactly<'['> >();
    if (!lex< type_selector >()) throw_syntax_error("invalid attribute name in attribute selector");
    Token name(lexed);
    attr_sel << context.new_Node(Node::value, path, line, name);
    if (lex< exactly<']'> >()) return attr_sel;
    if (!lex< alternatives< exact_match, class_match, dash_match,
                            prefix_match, suffix_match, substring_match > >()) {
      throw_syntax_error("invalid operator in attribute selector for " + name.to_string());
    }
    attr_sel << context.new_Node(Node::value, path, line, lexed);
    if (!lex< string_constant >()) throw_syntax_error("expected a quoted string constant in attribute selector for " + name.to_string());
    attr_sel << context.new_Node(Node::value, path, line, lexed);
    if (!lex< exactly<']'> >()) throw_syntax_error("unterminated attribute selector for " + name.to_string());
    return attr_sel;
  }

  Node Document::parse_block(bool definition)
  {
    lex< exactly<'{'> >();
    bool semicolon = false;
    Selector_Lookahead lookahead_result;
    Node block(context.new_Node(Node::block, path, line, 0));
    while (!lex< exactly<'}'> >()) {
      if (semicolon) {
        if (!lex< exactly<';'> >()) throw_syntax_error("non-terminal statement or declaration must end with ';'");
        semicolon = false;
        while (lex< block_comment >()) {
          block << context.new_Node(Node::comment, path, line, lexed);
        }
        if (lex< exactly<'}'> >()) break;
      }
      if (lex< block_comment >()) {
        block << context.new_Node(Node::comment, path, line, lexed);
      }
      else if (peek< import >(position)) {
        if (definition) {
          lex< import >(); // to adjust the line number
          throw_syntax_error("@import directive not allowed inside mixin definition");
        }
        Node imported_tree(parse_import());
        if (imported_tree.type() == Node::css_import) {
          block << imported_tree;
        }
        else {
          for (size_t i = 0, S = imported_tree.size(); i < S; ++i) {
            block << imported_tree[i];
          }
          semicolon = true;
        }
      }
      else if (peek< include >(position)) {
        block << parse_mixin_call();
        semicolon = true;
      }
      else if (lex< variable >()) {
        block << parse_assignment();
        semicolon = true;
      }
      else if (peek< sequence< identifier, optional_spaces, exactly<':'>, optional_spaces, exactly<'{'> > >(position)) {
        block << parse_propset();
      }
      else if ((lookahead_result = lookahead_for_selector(position)).found) {
        block << parse_ruleset(definition);
      }
      else if (peek< exactly<'+'> >()) {
        block << parse_mixin_call();
        semicolon = true;
      }
      else if (!peek< exactly<';'> >()) {
        Node rule(parse_rule());
        // check for lbrace; if it's there, we have a namespace property with a value
        if (peek< exactly<'{'> >()) {
          Node inner(parse_block());
          Node propset(context.new_Node(Node::propset, path, line, 2));
          propset << rule[0];
          rule[0] = context.new_Node(Node::property, path, line, Token::make());
          inner.push_front(rule);
          propset << inner;
          block << propset;
        }
        else {
          block << rule;
          semicolon = true;
        }
      }
      else lex< exactly<';'> >();
      while (lex< block_comment >()) {
        block << context.new_Node(Node::comment, path, line, lexed);
      }
    }
    return block;
  }

  Node Document::parse_rule() {
    Node rule(context.new_Node(Node::rule, path, line, 2));
    if (!lex< sequence< optional< exactly<'*'> >, identifier > >()) {
      lex< spaces_and_comments >(); // get the line number right
      throw_syntax_error("invalid property name");
    }
    rule << context.new_Node(Node::property, path, line, lexed);
    if (!lex< exactly<':'> >()) throw_syntax_error("property \"" + lexed.to_string() + "\" must be followed by a ':'");
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
    { return context.new_Node(Node::nil, path, line, 0); }
    Node list1(parse_space_list());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< exactly<','> >(position)) return list1;
    
    Node comma_list(context.new_Node(Node::comma_list, path, line, 2));
    comma_list << list1;
    comma_list.should_eval() |= list1.should_eval();
    
    while (lex< exactly<','> >())
    {
      Node list(parse_space_list());
      comma_list << list;
      comma_list.should_eval() |= list.should_eval();
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
    
    Node space_list(context.new_Node(Node::space_list, path, line, 2));
    space_list << disj1;
    space_list.should_eval() |= disj1.should_eval();
    
    while (!(peek< exactly<';'> >(position) ||
             peek< exactly<'}'> >(position) ||
             peek< exactly<'{'> >(position) ||
             peek< exactly<')'> >(position) ||
             peek< exactly<','> >(position)))
    {
      Node disj(parse_disjunction());
      space_list << disj;
      space_list.should_eval() |= disj.should_eval();
    }
    
    return space_list;
  }
  
  Node Document::parse_disjunction()
  {
    Node conj1(parse_conjunction());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< sequence< or_kwd, negate< identifier > > >()) return conj1;
    
    Node disjunction(context.new_Node(Node::disjunction, path, line, 2));
    disjunction << conj1;
    while (lex< sequence< or_kwd, negate< identifier > > >()) disjunction << parse_conjunction();
    disjunction.should_eval() = true;
    
    return disjunction;
  }
  
  Node Document::parse_conjunction()
  {
    Node rel1(parse_relation());
    // if it's a singleton, return it directly; don't wrap it
    if (!peek< sequence< and_kwd, negate< identifier > > >()) return rel1;
    
    Node conjunction(context.new_Node(Node::conjunction, path, line, 2));
    conjunction << rel1;
    while (lex< sequence< and_kwd, negate< identifier > > >()) conjunction << parse_relation();
    conjunction.should_eval() = true;
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
    
    Node relation(context.new_Node(Node::relation, path, line, 3));
    expr1.should_eval() = true;
    relation << expr1;
        
    if (lex< eq_op >()) relation << context.new_Node(Node::eq, path, line, lexed);
    else if (lex< neq_op >()) relation << context.new_Node(Node::neq, path, line, lexed);
    else if (lex< gte_op >()) relation << context.new_Node(Node::gte, path, line, lexed);
    else if (lex< lte_op >()) relation << context.new_Node(Node::lte, path, line, lexed);
    else if (lex< gt_op >()) relation << context.new_Node(Node::gt, path, line, lexed);
    else if (lex< lt_op >()) relation << context.new_Node(Node::lt, path, line, lexed);
        
    Node expr2(parse_expression());
    expr2.should_eval() = true;
    relation << expr2;
    
    relation.should_eval() = true;
    return relation;
  }
  
  Node Document::parse_expression()
  {
    Node term1(parse_term());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'+'> >(position) ||
          peek< sequence< negate< number >, exactly<'-'> > >(position)))
    { return term1; }
    
    Node expression(context.new_Node(Node::expression, path, line, 3));
    term1.should_eval() = true;
    expression << term1;
    
    while (lex< exactly<'+'> >() || lex< sequence< negate< number >, exactly<'-'> > >()) {
      if (lexed.begin[0] == '+') {
        expression << context.new_Node(Node::add, path, line, lexed);
      }
      else {
        expression << context.new_Node(Node::sub, path, line, lexed);
      }
      Node term(parse_term());
      term.should_eval() = true;
      expression << term;
    }
    expression.should_eval() = true;

    return expression;
  }
  
  Node Document::parse_term()
  {
    Node fact1(parse_factor());
    // if it's a singleton, return it directly; don't wrap it
    if (!(peek< exactly<'*'> >(position) ||
          peek< exactly<'/'> >(position)))
    { return fact1; }

    Node term(context.new_Node(Node::term, path, line, 3));
    term << fact1;
    if (fact1.should_eval()) term.should_eval() = true;

    while (lex< exactly<'*'> >() || lex< exactly<'/'> >()) {
      if (lexed.begin[0] == '*') {
        term << context.new_Node(Node::mul, path, line, lexed);
        term.should_eval() = true;
      }
      else {
        term << context.new_Node(Node::div, path, line, lexed);
      }
      Node fact(parse_factor());
      term.should_eval() |= fact.should_eval();
      term << fact;
    }

    return term;
  }
  
  Node Document::parse_factor()
  {
    if (lex< exactly<'('> >()) {
      Node value(parse_comma_list());
      value.should_eval() = true;
      if (value.type() == Node::comma_list || value.type() == Node::space_list) {
        value[0].should_eval() = true;
      }
      if (!lex< exactly<')'> >()) throw_syntax_error("unclosed parenthesis");
      return value;
    }
    else if (lex< sequence< exactly<'+'>, negate< number > > >()) {
      Node plus(context.new_Node(Node::unary_plus, path, line, 1));
      plus << parse_factor();
      plus.should_eval() = true;
      return plus;
    }
    else if (lex< sequence< exactly<'-'>, negate< number> > >()) {
      Node minus(context.new_Node(Node::unary_minus, path, line, 1));
      minus << parse_factor();
      minus.should_eval() = true;
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
      const char* value = position;
      const char* rparen = find_first< exactly<')'> >(position);
      if (!rparen) throw_syntax_error("URI is missing ')'");
      Token contents(Token::make(value, rparen));
      // lex< string_constant >();
      Node result(context.new_Node(Node::uri, path, line, contents));
      position = rparen;
      lex< exactly<')'> >();
      return result;
    }
    
    if (lex< value_schema >())
    { return Document::make_from_token(context, lexed, path, line).parse_value_schema(); }
    
    if (lex< sequence< true_kwd, negate< identifier > > >())
    { return context.new_Node(Node::boolean, path, line, true); }
    
    if (lex< sequence< false_kwd, negate< identifier > > >())
    { return context.new_Node(Node::boolean, path, line, false); }
    
    if (peek< functional >())
    { return parse_function_call(); }
    
    if (lex< important >())
    { return context.new_Node(Node::important, path, line, lexed); }

    if (lex< identifier >())
    { return context.new_Node(Node::identifier, path, line, lexed); }

    if (lex< percentage >())
    { return context.new_Node(Node::textual_percentage, path, line, lexed); }

    if (lex< dimension >())
    { return context.new_Node(Node::textual_dimension, path, line, lexed); }

    if (lex< number >())
    { return context.new_Node(Node::textual_number, path, line, lexed); }

    if (lex< hex >())
    { return context.new_Node(Node::textual_hex, path, line, lexed); }

    if (peek< string_constant >())
    { return parse_string(); } 

    if (lex< variable >())
    {
      Node var(context.new_Node(Node::variable, path, line, lexed));
      var.should_eval() = true;
      return var;
    }
    
    throw_syntax_error("error reading values after " + lexed.to_string());

    // unreachable statement
    return Node();
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
      return context.new_Node(Node::string_constant, path, line, str);
    }
    
    Node schema(context.new_Node(Node::string_schema, path, line, 1));
    while (i < str.end) {
      p = find_first_in_interval< sequence< negate< exactly<'\\'> >, exactly<hash_lbrace> > >(i, str.end);
      if (p) {
        if (i < p) {
          schema << context.new_Node(Node::identifier, path, line, Token::make(i, p-2)); // accumulate the preceding segment if it's nonempty
        }
        const char* j = find_first_in_interval< exactly<rbrace> >(p, str.end); // find the closing brace
        if (j) {
          // parse the interpolant and accumulate it
          Node interp_node(Document::make_from_token(context, Token::make(p, j-1), path, line).parse_list());
          interp_node.should_eval() = true;
          schema << interp_node;
          i = j;
        }
        else {
          // throw an error if the interpolant is unterminated
          throw_syntax_error("unterminated interpolant inside string constant " + str.to_string());
        }
      }
      else { // no interpolants left; add the last segment if nonempty
        if (i < str.end) schema << context.new_Node(Node::identifier, path, line, Token::make(i, str.end));
        break;
      }
    }
    return schema;
  }
  
  Node Document::parse_value_schema()
  {    
    Node schema(context.new_Node(Node::value_schema, path, line, 1));
    
    while (position < end) {
      if (lex< interpolant >()) {
        Token insides(Token::make(lexed.begin + 2, lexed.end - 1));
        Node interp_node(Document::make_from_token(context, insides, path, line).parse_list());
        schema << interp_node;
      }
      else if (lex< identifier >()) {
        schema << context.new_Node(Node::identifier, path, line, lexed);
      }
      else if (lex< percentage >()) {
        schema << context.new_Node(Node::textual_percentage, path, line, lexed);
      }
      else if (lex< dimension >()) {
        schema << context.new_Node(Node::textual_dimension, path, line, lexed);
      }
      else if (lex< number >()) {
        schema << context.new_Node(Node::textual_number, path, line, lexed);
      }
      else if (lex< hex >()) {
        schema << context.new_Node(Node::textual_hex, path, line, lexed);
      }
      else if (lex< string_constant >()) {
        schema << context.new_Node(Node::string_constant, path, line, lexed);
      }
      else if (lex< variable >()) {
        schema << context.new_Node(Node::variable, path, line, lexed);
      }
      else {
        throw_syntax_error("error parsing interpolated value");
      }
    }
    schema.should_eval() = true;
    return schema;
  }
  
  Node Document::parse_function_call()
  {
    lex< identifier >();
    Node name(context.new_Node(Node::identifier, path, line, lexed));
    Node args(parse_arguments());
    Node call(context.new_Node(Node::function_call, path, line, 2));
    call << name << args;
    call.should_eval() = true;
    return call;
  }
  
  // const char* Document::lookahead_for_selector(const char* start)
  Selector_Lookahead Document::lookahead_for_selector(const char* start)
  {
    const char* p = start ? start : position;
    const char* q;
    bool saw_interpolant = false;

    while ((q = peek< identifier >(p))                            ||
           (q = peek< id_name >(p))                               ||
           (q = peek< class_name >(p))                            ||
           (q = peek< sequence< pseudo_prefix, identifier > >(p)) ||
           (q = peek< string_constant >(p))                       ||
           (q = peek< exactly<'*'> >(p))                          ||
           (q = peek< exactly<'('> >(p))                          ||
           (q = peek< exactly<')'> >(p))                          ||
           (q = peek< exactly<'['> >(p))                          ||
           (q = peek< exactly<']'> >(p))                          ||
           (q = peek< exactly<'+'> >(p))                          ||
           (q = peek< exactly<'~'> >(p))                          ||
           (q = peek< exactly<'>'> >(p))                          ||
           (q = peek< exactly<','> >(p))                          ||
           (q = peek< binomial >(p))                              ||
           (q = peek< sequence< optional<sign>,
                                optional<digits>,
                                exactly<'n'> > >(p))              ||
           (q = peek< sequence< optional<sign>,
                                digits > >(p))                    ||
           (q = peek< number >(p))                                ||
           (q = peek< exactly<'&'> >(p))                          ||
           (q = peek< alternatives<exact_match,
                                   class_match,
                                   dash_match,
                                   prefix_match,
                                   suffix_match,
                                   substring_match> >(p))         ||
           (q = peek< interpolant >(p))) {
      p = q;
      if (*(p - 1) == '}') saw_interpolant = true;
    }

    Selector_Lookahead result;
    result.found            = peek< exactly<'{'> >(p) ? p : 0;
    result.has_interpolants = saw_interpolant;

    return result;
  }
  
}