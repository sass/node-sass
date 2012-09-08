#include "prelexer.hpp"
#include "eval_apply.hpp"
#include "document.hpp"
#include "error.hpp"
#include <cctype>
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace Sass {
  using std::cerr; using std::endl;

  static void throw_eval_error(string message, string path, size_t line)
  {
    if (!path.empty() && Prelexer::string_constant(path.c_str()))
      path = path.substr(1, path.size() - 1);

    throw Error(Error::evaluation, path, line, message);
  }

  // Evaluate the parse tree in-place (mostly). Most nodes will be left alone.

  Node eval(Node expr, Node prefix, Environment& env, map<string, Function>& f_env, Node_Factory& new_Node, Context& ctx, bool function_name)
  {
    switch (expr.type())
    {
      case Node::mixin: {
        env[expr[0].token()] = expr;
        return expr;
      } break;

      case Node::function: {
        f_env[expr[0].to_string()] = Function(expr);
        return expr;
      } break;
      
      case Node::expansion: {
        Token name(expr[0].token());
        Node args(expr[1]);
        if (!env.query(name)) throw_eval_error("mixin " + name.to_string() + " is undefined", expr.path(), expr.line());
        Node mixin(env[name]);
        Node expansion(apply_mixin(mixin, args, prefix, env, f_env, new_Node, ctx));
        expr.pop_back();
        expr.pop_back();
        expr += expansion;
        return expr;
      } break;
      
      case Node::propset: {
        eval(expr[1], prefix, env, f_env, new_Node, ctx);
        return expr;
      } break;

      case Node::ruleset: {
        // if the selector contains interpolants, eval it and re-parse
        if (expr[0].type() == Node::selector_schema) {
          expr[0] = eval(expr[0], prefix, env, f_env, new_Node, ctx);
        }

        // expand the selector with the prefix and save it in expr[2]
        expr << expand_selector(expr[0], prefix, new_Node);

        // gather selector extensions into a pending queue
        if (ctx.has_extensions) {
          // check single selector
          if (expr.back().type() != Node::selector_group) {
            Node sel(selector_base(expr.back()));
            if (ctx.extensions.count(sel)) {
              for (multimap<Node, Node>::iterator i = ctx.extensions.lower_bound(sel); i != ctx.extensions.upper_bound(sel); ++i) {
                ctx.pending_extensions.push_back(pair<Node, Node>(expr, i->second));
              }
            }
          }
          // individually check each selector in a group
          else {
            Node group(expr.back());
            for (size_t i = 0, S = group.size(); i < S; ++i) {
              Node sel(selector_base(group[i]));
              if (ctx.extensions.count(sel)) {
                for (multimap<Node, Node>::iterator j = ctx.extensions.lower_bound(sel); j != ctx.extensions.upper_bound(sel); ++j) {
                  ctx.pending_extensions.push_back(pair<Node, Node>(expr, j->second));
                }
              }
            }
          }
        }

        // eval the body with the current selector as the prefix
        eval(expr[1], expr.back(), env, f_env, new_Node, ctx);
        return expr;
      } break;

      case Node::media_query: {
        Node block(expr[1]);
        Node new_ruleset(new_Node(Node::ruleset, expr.path(), expr.line(), 3));
        new_ruleset << prefix << block << prefix;
        expr[1] = eval(new_ruleset, new_Node(Node::none, expr.path(), expr.line(), 0), env, f_env, new_Node, ctx);
        return expr;
      } break;

      case Node::selector_schema: {
        string expansion;
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          expr[i] = eval(expr[i], prefix, env, f_env, new_Node, ctx);
          if (expr[i].type() == Node::string_constant) {
            expansion += expr[i].token().unquote();
          }
          else {
            expansion += expr[i].to_string();
          }
        }
        expansion += " {"; // the parser looks for an lbrace to end a selector
        char* expn_src = new char[expansion.size() + 1];
        strcpy(expn_src, expansion.c_str());
        Document needs_reparsing(Document::make_from_source_chars(ctx, expn_src, expr.path(), true));
        needs_reparsing.line = expr.line(); // set the line number to the original node's line
        Node sel(needs_reparsing.parse_selector_group());
        return sel;
      } break;
      
      case Node::root: {
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          expr[i] = eval(expr[i], prefix, env, f_env, new_Node, ctx);
        }
        return expr;
      } break;
      
      case Node::block: {
        Environment new_frame;
        new_frame.link(env);
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          expr[i] = eval(expr[i], prefix, new_frame, f_env, new_Node, ctx);
        }
        return expr;
      } break;
      
      case Node::assignment: {
        Node val(expr[1]);
        if (val.type() == Node::comma_list || val.type() == Node::space_list) {
          for (size_t i = 0, S = val.size(); i < S; ++i) {
            if (val[i].should_eval()) val[i] = eval(val[i], prefix, env, f_env, new_Node, ctx);
          }
        }
        else {
          val = eval(val, prefix, env, f_env, new_Node, ctx);
        }
        Node var(expr[0]);
        if (expr.is_guarded() && env.query(var.token())) return expr;
        // If a binding exists (possible upframe), then update it.
        // Otherwise, make a new on in the current frame.
        if (env.query(var.token())) {
          env[var.token()] = val;
        }
        else {
          env.current_frame[var.token()] = val;
        }
        return expr;
      } break;

      case Node::rule: {
        Node lhs(expr[0]);
        if (lhs.should_eval()) eval(lhs, prefix, env, f_env, new_Node, ctx);
        Node rhs(expr[1]);
        if (rhs.type() == Node::comma_list || rhs.type() == Node::space_list) {
          for (size_t i = 0, S = rhs.size(); i < S; ++i) {
            if (rhs[i].should_eval()) rhs[i] = eval(rhs[i], prefix, env, f_env, new_Node, ctx);
          }
        }
        else if (rhs.type() == Node::value_schema || rhs.type() == Node::string_schema) {
          eval(rhs, prefix, env, f_env, new_Node, ctx);
        }
        else {
          if (rhs.should_eval()) expr[1] = eval(rhs, prefix, env, f_env, new_Node, ctx);
        }
        return expr;
      } break;

      case Node::comma_list:
      case Node::space_list: {
        if (expr.should_eval()) expr[0] = eval(expr[0], prefix, env, f_env, new_Node, ctx);
        return expr;
      } break;
      
      case Node::disjunction: {
        Node result;
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          result = eval(expr[i], prefix, env, f_env, new_Node, ctx);
          if (result.type() == Node::boolean && result.boolean_value() == false) continue;
          else return result;
        }
        return result;
      } break;
      
      case Node::conjunction: {
        Node result;
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          result = eval(expr[i], prefix, env, f_env, new_Node, ctx);
          if (result.type() == Node::boolean && result.boolean_value() == false) return result;
        }
        return result;
      } break;
      
      case Node::relation: {
        
        Node lhs(eval(expr[0], prefix, env, f_env, new_Node, ctx));
        Node op(expr[1]);
        Node rhs(eval(expr[2], prefix, env, f_env, new_Node, ctx));
        // TO DO: don't allocate both T and F
        Node T(new_Node(Node::boolean, lhs.path(), lhs.line(), true));
        Node F(new_Node(Node::boolean, lhs.path(), lhs.line(), false));
        
        switch (op.type())
        {
          case Node::eq:  return (lhs == rhs) ? T : F;
          case Node::neq: return (lhs != rhs) ? T : F;
          case Node::gt:  return (lhs > rhs)  ? T : F;
          case Node::gte: return (lhs >= rhs) ? T : F;
          case Node::lt:  return (lhs < rhs)  ? T : F;
          case Node::lte: return (lhs <= rhs) ? T : F;
          default:
            throw_eval_error("unknown comparison operator " + expr.token().to_string(), expr.path(), expr.line());
            return Node();
        }
      } break;

      case Node::expression: {
        Node acc(new_Node(Node::expression, expr.path(), expr.line(), 1));
        acc << eval(expr[0], prefix, env, f_env, new_Node, ctx);
        Node rhs(eval(expr[2], prefix, env, f_env, new_Node, ctx));
        accumulate(expr[1].type(), acc, rhs, new_Node);
        for (size_t i = 3, S = expr.size(); i < S; i += 2) {
          Node rhs(eval(expr[i+1], prefix, env, f_env, new_Node, ctx));
          accumulate(expr[i].type(), acc, rhs, new_Node);
        }
        return acc.size() == 1 ? acc[0] : acc;
      } break;

      case Node::term: {
        if (expr.should_eval()) {
          Node acc(new_Node(Node::expression, expr.path(), expr.line(), 1));
          acc << eval(expr[0], prefix, env, f_env, new_Node, ctx);
          Node rhs(eval(expr[2], prefix, env, f_env, new_Node, ctx));
          accumulate(expr[1].type(), acc, rhs, new_Node);
          for (size_t i = 3, S = expr.size(); i < S; i += 2) {
            Node rhs(eval(expr[i+1], prefix, env, f_env, new_Node, ctx));
            accumulate(expr[i].type(), acc, rhs, new_Node);
          }
          return acc.size() == 1 ? acc[0] : acc;
        }
        else {
          return expr;
        }
      } break;

      case Node::textual_percentage: {
        return new_Node(expr.path(), expr.line(), std::atof(expr.token().begin), Node::numeric_percentage);
      } break;

      case Node::textual_dimension: {
        return new_Node(expr.path(), expr.line(),
                        std::atof(expr.token().begin),
                        Token::make(Prelexer::number(expr.token().begin),
                                    expr.token().end));
      } break;
      
      case Node::textual_number: {
        return new_Node(expr.path(), expr.line(), std::atof(expr.token().begin));
      } break;

      case Node::textual_hex: {        
        Node triple(new_Node(Node::numeric_color, expr.path(), expr.line(), 4));
        Token hext(Token::make(expr.token().begin+1, expr.token().end));
        if (hext.length() == 6) {
          for (int i = 0; i < 6; i += 2) {
            triple << new_Node(expr.path(), expr.line(), static_cast<double>(std::strtol(string(hext.begin+i, 2).c_str(), NULL, 16)));
          }
        }
        else {
          for (int i = 0; i < 3; ++i) {
            triple << new_Node(expr.path(), expr.line(), static_cast<double>(std::strtol(string(2, hext.begin[i]).c_str(), NULL, 16)));
          }
        }
        triple << new_Node(expr.path(), expr.line(), 1.0);
        return triple;
      } break;
      
      case Node::variable: {
        if (!env.query(expr.token())) throw_eval_error("reference to unbound variable " + expr.token().to_string(), expr.path(), expr.line());
        return env[expr.token()];
      } break;

      case Node::image_url: {
        Node base(eval(expr[0], prefix, env, f_env, new_Node, ctx));
        Node prefix(new_Node(Node::identifier, base.path(), base.line(), Token::make(ctx.image_path)));
        Node fullpath(new_Node(Node::concatenation, base.path(), base.line(), 2));
        Node url(new_Node(Node::uri, base.path(), base.line(), 1));
        fullpath << prefix << base;
        url << fullpath;
        return url;
      } break;
      
      case Node::function_call: {
        // TO DO: default-constructed Function should be a generic callback (maybe)

        // eval the function name in case it's interpolated
        expr[0] = eval(expr[0], prefix, env, f_env, new_Node, ctx, true);
        string name(expr[0].to_string());
        if (!f_env.count(name)) {
          // no definition available; just pass it through (with evaluated args)
          Node args(expr[1]);
          for (size_t i = 0, S = args.size(); i < S; ++i) {
            args[i] = eval(args[i], prefix, env, f_env, new_Node, ctx);
          }
          return expr;
        }
        else {
          // check to see if the function is primitive/built-in
          Function f(f_env[name]);
          if (f.overloaded) {
            stringstream s;
            s << name << " " << expr[1].size();
            string resolved_name(s.str());
            if (!f_env.count(resolved_name)) throw_eval_error("wrong number of arguments to " + name, expr.path(), expr.line());
            f = f_env[resolved_name];
          }
          return apply_function(f, expr[1], prefix, env, f_env, new_Node, ctx);
        }
      } break;
      
      case Node::unary_plus: {
        Node arg(eval(expr[0], prefix, env, f_env, new_Node, ctx));
        if (arg.is_numeric()) {
          return arg;
        }
        else {
          expr[0] = arg;
          return expr;
        }
      } break;
      
      case Node::unary_minus: {
        Node arg(eval(expr[0], prefix, env, f_env, new_Node, ctx));
        if (arg.is_numeric()) {
          return new_Node(expr.path(), expr.line(), -arg.numeric_value());
        }
        else {
          expr[0] = arg;
          return expr;
        }
      } break;

      case Node::identifier: {
        string id_str(expr.to_string());
        to_lowercase(id_str);
        if (!function_name && ctx.color_names_to_values.count(id_str)) {
          Node color_orig(ctx.color_names_to_values[id_str]);
          Node r(color_orig[0]);
          Node g(color_orig[1]);
          Node b(color_orig[2]);
          Node a(color_orig[3]);
          return new_Node(expr.path(), expr.line(),
                          r.numeric_value(),
                          g.numeric_value(),
                          b.numeric_value(),
                          a.numeric_value());
        }
        else {
          return expr;
        }
      } break;
      
      case Node::string_schema:
      case Node::value_schema:
      case Node::identifier_schema: {
        for (size_t i = 0, S = expr.size(); i < S; ++i) {
          expr[i] = eval(expr[i], prefix, env, f_env, new_Node, ctx);
        }
        return expr;
      } break;
      
      case Node::css_import: {
        expr[0] = eval(expr[0], prefix, env, f_env, new_Node, ctx);
        return expr;
      } break;

      case Node::if_directive: {
        for (size_t i = 0, S = expr.size(); i < S; i += 2) {
          if (expr[i].type() != Node::block) {
            // cerr << "EVALUATING PREDICATE " << (i/2+1) << endl;
            Node predicate_val(eval(expr[i], prefix, env, f_env, new_Node, ctx));
            if ((predicate_val.type() != Node::boolean) || predicate_val.boolean_value()) {
              // cerr << "EVALUATING CONSEQUENT " << (i/2+1) << endl;
              return eval(expr[i+1], prefix, env, f_env, new_Node, ctx);
            }
          }
          else {
            // cerr << "EVALUATING ALTERNATIVE" << endl;
            return eval(expr[i], prefix, env, f_env, new_Node, ctx);
          }
        }
      } break;

      case Node::for_through_directive:
      case Node::for_to_directive: {
        Node fake_mixin(new_Node(Node::mixin, expr.path(), expr.line(), 3));
        Node fake_param(new_Node(Node::parameters, expr.path(), expr.line(), 1));
        fake_mixin << new_Node(Node::none, "", 0, 0) << (fake_param << expr[0]) << expr[3];
        Node lower_bound(eval(expr[1], prefix, env, f_env, new_Node, ctx));
        Node upper_bound(eval(expr[2], prefix, env, f_env, new_Node, ctx));
        if (!(lower_bound.is_numeric() && upper_bound.is_numeric())) {
          throw_eval_error("bounds of @for directive must be numeric", expr.path(), expr.line());
        }
        expr.pop_back();
        expr.pop_back();
        expr.pop_back();
        expr.pop_back();
        for (double i = lower_bound.numeric_value(),
                    U = upper_bound.numeric_value() + ((expr.type() == Node::for_to_directive) ? 0 : 1);
             i < U;
             ++i) {
          Node i_node(new_Node(expr.path(), expr.line(), i));
          Node fake_arg(new_Node(Node::arguments, expr.path(), expr.line(), 1));
          fake_arg << i_node;
          expr += apply_mixin(fake_mixin, fake_arg, prefix, env, f_env, new_Node, ctx, true);
        }
      } break;

      case Node::each_directive: {
        Node fake_mixin(new_Node(Node::mixin, expr.path(), expr.line(), 3));
        Node fake_param(new_Node(Node::parameters, expr.path(), expr.line(), 1));
        fake_mixin << new_Node(Node::none, "", 0, 0) << (fake_param << expr[0]) << expr[2];
        Node list(eval(expr[1], prefix, env, f_env, new_Node, ctx));
        // If the list isn't really a list, make a singleton out of it.
        if (list.type() != Node::space_list && list.type() != Node::comma_list) {
          list = (new_Node(Node::space_list, list.path(), list.line(), 1) << list);
        }
        expr.pop_back();
        expr.pop_back();
        expr.pop_back();
        for (size_t i = 0, S = list.size(); i < S; ++i) {
          Node fake_arg(new_Node(Node::arguments, expr.path(), expr.line(), 1));
          fake_arg << eval(list[i], prefix, env, f_env, new_Node, ctx);
          expr += apply_mixin(fake_mixin, fake_arg, prefix, env, f_env, new_Node, ctx, true);
        }
      } break;

      case Node::while_directive: {
        Node fake_mixin(new_Node(Node::mixin, expr.path(), expr.line(), 3));
        Node fake_param(new_Node(Node::parameters, expr.path(), expr.line(), 0));
        Node fake_arg(new_Node(Node::arguments, expr.path(), expr.line(), 0));
        fake_mixin << new_Node(Node::none, "", 0, 0) << fake_param << expr[1];
        Node pred(expr[0]);
        expr.pop_back();
        expr.pop_back();
        Node ev_pred(eval(pred, prefix, env, f_env, new_Node, ctx));
        while ((ev_pred.type() != Node::boolean) || ev_pred.boolean_value()) {
          expr += apply_mixin(fake_mixin, fake_arg, prefix, env, f_env, new_Node, ctx, true);
          ev_pred = eval(pred, prefix, env, f_env, new_Node, ctx);
        }
      } break;

      case Node::block_directive: {
        // TO DO: eval the directive name for interpolants
        eval(expr[1], new_Node(Node::none, expr.path(), expr.line(), 0), env, f_env, new_Node, ctx);
        return expr;
      } break;

      case Node::warning: {
        expr[0] = eval(expr[0], prefix, env, f_env, new_Node, ctx);
        return expr;
      } break;

      default: {
        return expr;
      } break;
    }

    return expr;
  }

  // Accumulate arithmetic operations. It's done this way because arithmetic
  // expressions are stored as vectors of operands with operators interspersed,
  // rather than as the usual binary tree.

  Node accumulate(Node::Type op, Node acc, Node rhs, Node_Factory& new_Node)
  {
    Node lhs(acc.back());
    double lnum = lhs.numeric_value();
    double rnum = rhs.numeric_value();
    
    if (lhs.type() == Node::number && rhs.type() == Node::number) {
      Node result(new_Node(acc.path(), acc.line(), operate(op, lnum, rnum)));
      acc.pop_back();
      acc.push_back(result);
    }
    // TO DO: find a way to merge the following two clauses
    else if (lhs.type() == Node::number && rhs.type() == Node::numeric_dimension) {
      Node result(new_Node(acc.path(), acc.line(), operate(op, lnum, rnum), rhs.unit()));
      acc.pop_back();
      acc.push_back(result);
    }
    else if (lhs.type() == Node::numeric_dimension && rhs.type() == Node::number) {
      Node result(new_Node(acc.path(), acc.line(), operate(op, lnum, rnum), lhs.unit()));
      acc.pop_back();
      acc.push_back(result);
    }
    else if (lhs.type() == Node::numeric_dimension && rhs.type() == Node::numeric_dimension) {
      // TO DO: CHECK FOR MISMATCHED UNITS HERE
      Node result;
      if (op == Node::div)
      { result = new_Node(acc.path(), acc.line(), operate(op, lnum, rnum)); }
      else
      { result = new_Node(acc.path(), acc.line(), operate(op, lnum, rnum), lhs.unit()); }
      acc.pop_back();
      acc.push_back(result);
    }
    // TO DO: find a way to merge the following two clauses
    else if (lhs.type() == Node::number && rhs.type() == Node::numeric_color) {
      if (op != Node::sub && op != Node::div) {
        double r = operate(op, lhs.numeric_value(), rhs[0].numeric_value());
        double g = operate(op, lhs.numeric_value(), rhs[1].numeric_value());
        double b = operate(op, lhs.numeric_value(), rhs[2].numeric_value());
        double a = rhs[3].numeric_value();
        acc.pop_back();
        acc << new_Node(acc.path(), acc.line(), r, g, b, a);
      }
      // trying to handle weird edge cases ... not sure if it's worth it
      else if (op == Node::div) {
        acc << new_Node(Node::div, acc.path(), acc.line(), 0);
        acc << rhs;
      }
      else if (op == Node::sub) {
        acc << new_Node(Node::sub, acc.path(), acc.line(), 0);
        acc << rhs;
      }
      else {
        acc << rhs;
      }
    }
    else if (lhs.type() == Node::numeric_color && rhs.type() == Node::number) {
      double r = operate(op, lhs[0].numeric_value(), rhs.numeric_value());
      double g = operate(op, lhs[1].numeric_value(), rhs.numeric_value());
      double b = operate(op, lhs[2].numeric_value(), rhs.numeric_value());
      double a = lhs[3].numeric_value();
      acc.pop_back();
      acc << new_Node(acc.path(), acc.line(), r, g, b, a);
    }
    else if (lhs.type() == Node::numeric_color && rhs.type() == Node::numeric_color) {
      if (lhs[3].numeric_value() != rhs[3].numeric_value()) throw_eval_error("alpha channels must be equal for " + lhs.to_string() + " + " + rhs.to_string(), lhs.path(), lhs.line());
      double r = operate(op, lhs[0].numeric_value(), rhs[0].numeric_value());
      double g = operate(op, lhs[1].numeric_value(), rhs[1].numeric_value());
      double b = operate(op, lhs[2].numeric_value(), rhs[2].numeric_value());
      double a = lhs[3].numeric_value();
      acc.pop_back();
      acc << new_Node(acc.path(), acc.line(), r, g, b, a);
    }
    else if (lhs.type() == Node::concatenation && rhs.type() == Node::concatenation) {
      if (op == Node::add) {
        lhs += rhs;
      }
      else {
        acc << new_Node(op, acc.path(), acc.line(), Token::make());
        acc << rhs;
      }
    }
    else if (lhs.type() == Node::concatenation && rhs.type() == Node::string_constant) {
      if (op == Node::add) {
        lhs << rhs;
      }
      else {
        acc << new_Node(op, acc.path(), acc.line(), Token::make());
        acc << rhs;
      }
    }
    else if (lhs.type() == Node::string_constant && rhs.type() == Node::concatenation) {
      if (op == Node::add) {
        Node new_cat(new_Node(Node::concatenation, lhs.path(), lhs.line(), 1 + rhs.size()));
        new_cat << lhs;
        new_cat += rhs;
        acc.pop_back();
        acc << new_cat;
      }
      else {
        acc << new_Node(op, acc.path(), acc.line(), Token::make());
        acc << rhs;
      }
    }
    else if (lhs.type() == Node::string_constant && rhs.type() == Node::string_constant) {
      if (op == Node::add) {
        Node new_cat(new_Node(Node::concatenation, lhs.path(), lhs.line(), 2));
        new_cat << lhs << rhs;
        acc.pop_back();
        acc << new_cat;
      }
      else {
        acc << new_Node(op, acc.path(), acc.line(), Token::make());
        acc << rhs;
      }
    }
    else {
      // TO DO: disallow division and multiplication on lists
      if (op == Node::sub) acc << new_Node(Node::sub, acc.path(), acc.line(), Token::make());
      acc.push_back(rhs);
    }

    return acc;
  }

  // Helper for doing the actual arithmetic.

  double operate(Node::Type op, double lhs, double rhs)
  {
    switch (op)
    {
      case Node::add: return lhs + rhs; break;
      case Node::sub: return lhs - rhs; break;
      case Node::mul: return lhs * rhs; break;
      case Node::div: return lhs / rhs; break;
      default:        return 0;         break;
    }
  }

  // Helper function for binding arguments in function and mixin calls.
  // Needs the environment containing the bindings to be passed in by the
  // caller. Also expects the caller to have pre-evaluated the arguments.
  void bind_arguments(string callee_name, const Node params, const Node args, Node prefix, Environment& env, map<string, Function>& f_env, Node_Factory& new_Node, Context& ctx)
  {
    // populate the env with the names of the parameters so we can check for
    // correctness further down
    for (size_t i = 0, S = params.size(); i < S; ++i) {
      Node param(params[i]);
      env.current_frame[param.type() == Node::variable ? param.token() : param[0].token()] = Node();
    }

    // now do the actual binding
    size_t args_bound = 0, num_params = params.size();
    for (size_t i = 0, j = 0, S = args.size(); i < S; ++i) {
      if (j >= num_params) {
        stringstream msg;
        msg << callee_name << " only takes " << num_params << " arguments";
        throw_eval_error(msg.str(), args.path(), args.line());
      }
      Node arg(args[i]), param(params[j]);
      // ordinal argument; just bind and keep going
      if (arg.type() != Node::assignment) {
        env[param.type() == Node::variable ? param.token() : param[0].token()] = arg;
        ++j;
      }
      // keyword argument -- need to check for correctness
      else {
        Token arg_name(arg[0].token());
        Node arg_value(arg[1]);
        if (!env.query(arg_name)) {
          throw_eval_error(callee_name + " has no parameter named " + arg_name.to_string(), arg.path(), arg.line());
        }
        if (!env[arg_name].is_stub()) {
          throw_eval_error(callee_name + " was passed argument " + arg_name.to_string() + " both by position and by name", arg.path(), arg.line());
        }
        env[arg_name] = arg_value;
        ++args_bound;
      }
    }
    // now plug in the holes with default values, if any
    for (size_t i = 0, S = params.size(); i < S; ++i) {
      Node param(params[i]);
      Token param_name((param.type() == Node::assignment ? param[0] : param).token());
      if (env[param_name].is_stub()) {
        if (param.type() != Node::assignment) {
          throw_eval_error(callee_name + " is missing argument " + param_name.to_string(), args.path(), args.line());
        }
        // eval default values in an environment where the previous vals have already been evaluated
        env[param_name] = eval(param[1], prefix, env, f_env, new_Node, ctx);
      }
    }
  }

  // Apply a mixin -- bind the arguments in a new environment, link the new
  // environment to the current one, then copy the body and eval in the new
  // environment.
  Node apply_mixin(Node mixin, const Node args, Node prefix, Environment& env, map<string, Function>& f_env, Node_Factory& new_Node, Context& ctx, bool dynamic_scope)
  {
    Node params(mixin[1]);
    Node body(new_Node(mixin[2])); // clone the body

    // evaluate arguments in the current environment
    for (size_t i = 0, S = args.size(); i < S; ++i) {
      if (args[i].type() != Node::assignment) {
        args[i] = eval(args[i], prefix, env, f_env, new_Node, ctx);
      }
      else {
        args[i][1] = eval(args[i][1], prefix, env, f_env, new_Node, ctx);
      }
    }

    // Create a new environment for the mixin and link it to the appropriate parent
    Environment bindings;
    if (dynamic_scope) {
      bindings.link(env);
    }
    else {
      // C-style scope for now (current/global, nothing in between). May need
      // to implement full lexical scope someday.
      bindings.link(env.global ? *env.global : env);
    }
    // bind arguments in the extended environment
    bind_arguments("mixin " + mixin[0].to_string(), params, args, prefix, bindings, f_env, new_Node, ctx);
    // evaluate the mixin's body
    for (size_t i = 0, S = body.size(); i < S; ++i) {
      body[i] = eval(body[i], prefix, bindings, f_env, new_Node, ctx);
    }
    return body;
  }

  // Apply a function -- bind the arguments and pass them to the underlying
  // primitive function implementation, then return its value.
  Node apply_function(const Function& f, const Node args, Node prefix, Environment& env, map<string, Function>& f_env, Node_Factory& new_Node, Context& ctx, string& path, size_t line)
  {
    if (f.primitive) {
      // evaluate arguments in the current environment
      for (size_t i = 0, S = args.size(); i < S; ++i) {
        if (args[i].type() != Node::assignment) {
          args[i] = eval(args[i], prefix, env, f_env, new_Node, ctx);
        }
        else {
          args[i][1] = eval(args[i][1], prefix, env, f_env, new_Node, ctx);
        }
      }
      // bind arguments
      Environment bindings;
      bindings.link(env.global ? *env.global : env);
      bind_arguments("function " + f.name, f.parameters, args, prefix, bindings, f_env, new_Node, ctx);
      return f.primitive(f.parameter_names, bindings, new_Node, path, line);
    }
    else {
      Node params(f.definition[1]);
      Node body(new_Node(f.definition[2]));

      // evaluate arguments in the current environment
      for (size_t i = 0, S = args.size(); i < S; ++i) {
        if (args[i].type() != Node::assignment) {
          args[i] = eval(args[i], prefix, env, f_env, new_Node, ctx);
        }
        else {
          args[i][1] = eval(args[i][1], prefix, env, f_env, new_Node, ctx);
        }
      }

      // create a new environment for the function and link it to the global env
      // (C-style scope -- no full-blown lexical scope yet)
      Environment bindings;
      bindings.link(env.global ? *env.global : env);
      // bind the arguments
      bind_arguments("function " + f.name, params, args, prefix, bindings, f_env, new_Node, ctx);
      return function_eval(f.name, body, bindings, new_Node, ctx, true);
    }
  }

  // Special function for evaluating pure Sass functions. The evaluation
  // algorithm is different in this case because the body needs to be
  // executed and a single value needs to be returned directly, rather than
  // styles being expanded and spliced in place.

  Node function_eval(string name, Node body, Environment& bindings, Node_Factory& new_Node, Context& ctx, bool at_toplevel)
  {
    for (size_t i = 0, S = body.size(); i < S; ++i) {
      Node stm(body[i]);
      switch (stm.type())
      {
        case Node::assignment: {
          Node val(stm[1]);
          if (val.type() == Node::comma_list || val.type() == Node::space_list) {
            for (size_t i = 0, S = val.size(); i < S; ++i) {
              if (val[i].should_eval()) val[i] = eval(val[i], Node(), bindings, ctx.function_env, new_Node, ctx);
            }
          }
          else {
            val = eval(val, Node(), bindings, ctx.function_env, new_Node, ctx);
          }
          Node var(stm[0]);
          if (stm.is_guarded() && bindings.query(var.token())) continue;
          // If a binding exists (possibly upframe), then update it.
          // Otherwise, make a new one in the current frame.
          if (bindings.query(var.token())) {
            bindings[var.token()] = val;
          }
          else {
            bindings.current_frame[var.token()] = val;
          }
        } break;

        case Node::if_directive: {
          for (size_t j = 0, S = stm.size(); j < S; j += 2) {
            if (stm[j].type() != Node::block) {
              Node predicate_val(eval(stm[j], Node(), bindings, ctx.function_env, new_Node, ctx));
              if ((predicate_val.type() != Node::boolean) || predicate_val.boolean_value()) {
                Node v(function_eval(name, stm[j+1], bindings, new_Node, ctx));
                if (v.is_null_ptr()) break;
                else                 return v;
              }
            }
            else {
              Node v(function_eval(name, stm[j], bindings, new_Node, ctx));
              if (v.is_null_ptr()) break;
              else                 return v;
            }
          }
        } break;

        case Node::for_through_directive:
        case Node::for_to_directive: {
          Node::Type for_type = stm.type();
          Node iter_var(stm[0]);
          Node lower_bound(eval(stm[1], Node(), bindings, ctx.function_env, new_Node, ctx));
          Node upper_bound(eval(stm[2], Node(), bindings, ctx.function_env, new_Node, ctx));
          Node for_body(stm[3]);
          Environment for_env; // re-use this env for each iteration
          for_env.link(bindings);
          for (double j = lower_bound.numeric_value(), T = upper_bound.numeric_value() + ((for_type == Node::for_to_directive) ? 0 : 1);
               j < T;
               j += 1) {
            for_env.current_frame[iter_var.token()] = new_Node(lower_bound.path(), lower_bound.line(), j);
            Node v(function_eval(name, for_body, for_env, new_Node, ctx));
            if (v.is_null_ptr()) continue;
            else                 return v;
          }
        } break;

        case Node::each_directive: {
          Node iter_var(stm[0]);
          Node list(eval(stm[1], Node(), bindings, ctx.function_env, new_Node, ctx));
          if (list.type() != Node::comma_list && list.type() != Node::space_list) {
            list = (new_Node(Node::space_list, list.path(), list.line(), 1) << list);
          }
          Node each_body(stm[2]);
          Environment each_env; // re-use this env for each iteration
          each_env.link(bindings);
          for (size_t j = 0, T = list.size(); j < T; ++j) {
            each_env.current_frame[iter_var.token()] = eval(list[j], Node(), bindings, ctx.function_env, new_Node, ctx);
            Node v(function_eval(name, each_body, each_env, new_Node, ctx));
            if (v.is_null_ptr()) continue;
            else return v;
          }
        } break;

        case Node::while_directive: {
          Node pred_expr(stm[0]);
          Node while_body(stm[1]);
          Environment while_env; // re-use this env for each iteration
          while_env.link(bindings);
          Node pred_val(eval(pred_expr, Node(), bindings, ctx.function_env, new_Node, ctx));
          while ((pred_val.type() != Node::boolean) || pred_val.boolean_value()) {
            Node v(function_eval(name, while_body, while_env, new_Node, ctx));
            if (v.is_null_ptr()) {
              pred_val = eval(pred_expr, Node(), bindings, ctx.function_env, new_Node, ctx);
              continue;
            }
            else return v;
          }
        } break;

        case Node::return_directive: {
          Node retval(eval(stm[0], Node(), bindings, ctx.function_env, new_Node, ctx));
          if (retval.type() == Node::comma_list || retval.type() == Node::space_list) {
            for (size_t i = 0, S = retval.size(); i < S; ++i) {
              retval[i] = eval(retval[i], Node(), bindings, ctx.function_env, new_Node, ctx);
            }
          }
          return retval;
        } break;

        default: {

        } break;
      }
    }
    if (at_toplevel) throw_eval_error("function finished without @return", body.path(), body.line());
    return Node();
  }

  // Expand a selector with respect to its prefix/context. Two separate cases:
  // when the selector has backrefs, substitute the prefix for each occurrence
  // of a backref. When the selector doesn't have backrefs, just prepend the
  // prefix. This function needs multiple subsidiary cases in order to properly
  // combine the various kinds of selectors.

  Node expand_selector(Node sel, Node pre, Node_Factory& new_Node)
  {
    if (pre.type() == Node::none) return sel;

    if (sel.has_backref()) {
      if ((pre.type() == Node::selector_group) && (sel.type() == Node::selector_group)) {
        Node group(new_Node(Node::selector_group, sel.path(), sel.line(), pre.size() * sel.size()));
        for (size_t i = 0, S = pre.size(); i < S; ++i) {
          for (size_t j = 0, T = sel.size(); j < T; ++j) {
            group << expand_backref(new_Node(sel[j]), pre[i]);
          }
        }
        return group;
      }
      else if ((pre.type() == Node::selector_group) && (sel.type() != Node::selector_group)) {
        Node group(new_Node(Node::selector_group, sel.path(), sel.line(), pre.size()));
        for (size_t i = 0, S = pre.size(); i < S; ++i) {
          group << expand_backref(new_Node(sel), pre[i]);
        }
        return group;
      }
      else if ((pre.type() != Node::selector_group) && (sel.type() == Node::selector_group)) {
        Node group(new_Node(Node::selector_group, sel.path(), sel.line(), sel.size()));
        for (size_t i = 0, S = sel.size(); i < S; ++i) {
          group << expand_backref(new_Node(sel[i]), pre);
        }
        return group;
      }
      else {
        return expand_backref(new_Node(sel), pre);
      }
    }

    if ((pre.type() == Node::selector_group) && (sel.type() == Node::selector_group)) {
      Node group(new_Node(Node::selector_group, sel.path(), sel.line(), pre.size() * sel.size()));
      for (size_t i = 0, S = pre.size(); i < S; ++i) {
        for (size_t j = 0, T = sel.size(); j < T; ++j) {
          Node new_sel(new_Node(Node::selector, sel.path(), sel.line(), 2));
          if (pre[i].type() == Node::selector) new_sel += pre[i];
          else                                 new_sel << pre[i];
          if (sel[j].type() == Node::selector) new_sel += sel[j];
          else                                 new_sel << sel[j];
          group << new_sel;
        }
      }
      return group;
    }
    else if ((pre.type() == Node::selector_group) && (sel.type() != Node::selector_group)) {
      Node group(new_Node(Node::selector_group, sel.path(), sel.line(), pre.size()));
      for (size_t i = 0, S = pre.size(); i < S; ++i) {
        Node new_sel(new_Node(Node::selector, sel.path(), sel.line(), 2));
        if (pre[i].type() == Node::selector) new_sel += pre[i];
        else                                 new_sel << pre[i];
        if (sel.type() == Node::selector)    new_sel += sel;
        else                                 new_sel << sel;
        group << new_sel;
      }
      return group;
    }
    else if ((pre.type() != Node::selector_group) && (sel.type() == Node::selector_group)) {
      Node group(new_Node(Node::selector_group, sel.path(), sel.line(), sel.size()));
      for (size_t i = 0, S = sel.size(); i < S; ++i) {
        Node new_sel(new_Node(Node::selector, sel.path(), sel.line(), 2));
        if (pre.type() == Node::selector)    new_sel += pre;
        else                                 new_sel << pre;
        if (sel[i].type() == Node::selector) new_sel += sel[i];
        else                                 new_sel << sel[i];
        group << new_sel;
      }
      return group;
    }
    else {
      Node new_sel(new_Node(Node::selector, sel.path(), sel.line(), 2));
      if (pre.type() == Node::selector) new_sel += pre;
      else                              new_sel << pre;
      if (sel.type() == Node::selector) new_sel += sel;
      else                              new_sel << sel;
      return new_sel;
    }
    // unreachable statement
    return Node();
  }

  // Helper for expanding selectors with backrefs.

  Node expand_backref(Node sel, Node pre)
  {
    switch (sel.type())
    {
      case Node::backref: {
        return pre;
      } break;

      case Node::simple_selector_sequence:
      case Node::selector: {
        for (size_t i = 0, S = sel.size(); i < S; ++i) {
          sel[i] = expand_backref(sel[i], pre);
        }
        return sel;
      } break;

      default: {
        return sel;
      } break;
    }
    // unreachable statement
    return Node();
  }

  // Resolve selector extensions.

  void extend_selectors(vector<pair<Node, Node> >& pending, multimap<Node, Node>& extension_table, Node_Factory& new_Node)
  {
    for (size_t i = 0, S = pending.size(); i < S; ++i) {
      // Node extender(pending[i].second[2]);
      // Node original_extender(pending[i].second[0]);
      Node ruleset_to_extend(pending[i].first);
      Node extendee(ruleset_to_extend[2]);

      if (extendee.type() != Node::selector_group && !extendee.has_been_extended()) {
        Node extendee_base(selector_base(extendee));
        Node extender_group(new_Node(Node::selector_group, extendee.path(), extendee.line(), 1));
        for (multimap<Node, Node>::iterator i = extension_table.lower_bound(extendee_base), E = extension_table.upper_bound(extendee_base);
             i != E;
             ++i) {
          if (i->second.size() <= 2) continue; // TODO: UN-HACKIFY THIS
          if (i->second[2].type() == Node::selector_group)
            extender_group += i->second[2];
          else
            extender_group << i->second[2];
        }
        Node extended_group(new_Node(Node::selector_group, extendee.path(), extendee.line(), extender_group.size() + 1));
        extendee.has_been_extended() = true;
        extended_group << extendee;
        for (size_t i = 0, S = extender_group.size(); i < S; ++i) {
          extended_group << generate_extension(extendee, extender_group[i], new_Node);
        }
        ruleset_to_extend[2] = extended_group;
      }
      else {
        Node extended_group(new_Node(Node::selector_group, extendee.path(), extendee.line(), extendee.size() + 1));
        for (size_t i = 0, S = extendee.size(); i < S; ++i) {
          Node extendee_i(extendee[i]);
          Node extendee_i_base(selector_base(extendee_i));
          extended_group << extendee_i;
          if (!extendee_i.has_been_extended() && extension_table.count(extendee_i_base)) {
            Node extender_group(new_Node(Node::selector_group, extendee.path(), extendee.line(), 1));
            for (multimap<Node, Node>::iterator i = extension_table.lower_bound(extendee_i_base), E = extension_table.upper_bound(extendee_i_base);
                 i != E;
                 ++i) {
              if (i->second.size() <= 2) continue; // TODO: UN-HACKIFY THIS
              if (i->second[2].type() == Node::selector_group)
                extender_group += i->second[2];
              else
                extender_group << i->second[2];
            }
            for (size_t j = 0, S = extender_group.size(); j < S; ++j) {
              extended_group << generate_extension(extendee_i, extender_group[j], new_Node);
            }
            extendee_i.has_been_extended() = true;
          }
        }
        ruleset_to_extend[2] = extended_group;
      }

      // if (extendee.type() != Node::selector_group && extender.type() != Node::selector_group) {
      //   Node ext(generate_extension(extendee, extender, new_Node));
      //   ext.push_front(extendee);
      //   ruleset_to_extend[2] = ext;
      // }
      // else if (extendee.type() == Node::selector_group && extender.type() != Node::selector_group) {
      //   cerr << "extending a group with a singleton!" << endl;
      //   Node new_group(new_Node(Node::selector_group, extendee.path(), extendee.line(), extendee.size()));
      //   for (size_t i = 0, S = extendee.size(); i < S; ++i) {
      //     new_group << extendee[i];
      //     if (extension_table.count(extendee[i])) {
      //       new_group << generate_extension(extendee[i], extender, new_Node);
      //     }
      //   }
      //   ruleset_to_extend[2] = new_group;
      // }
      // else if (extendee.type() != Node::selector_group && extender.type() == Node::selector_group) {
      //   cerr << "extending a singleton with a group!" << endl;
      // }
      // else {
      //   cerr << "skipping this for now!" << endl;
      // }

      // if (selector_to_extend.type() != Node::selector_group) {
      //   Node ext(generate_extension(selector_to_extend, extender, new_Node));
      //   ext.push_front(selector_to_extend);
      //   ruleset_to_extend[2] = ext;
      // }
      // else {
      //   cerr << "possibly extending a selector in a group: " << selector_to_extend.to_string() << endl;
      //   Node new_group(new_Node(Node::selector_group,
      //                  selector_to_extend.path(),
      //                  selector_to_extend.line(),
      //                  selector_to_extend.size()));
      //   for (size_t i = 0, S = selector_to_extend.size(); i < S; ++i) {
      //     Node sel_i(selector_to_extend[i]);
      //     Node sel_ib(selector_base(sel_i));
      //     if (extension_table.count(sel_ib)) {
      //       for (multimap<Node, Node>::iterator i = extension_table.lower_bound(sel_ib); i != extension_table.upper_bound(sel_ib); ++i) {
      //         if (i->second.is(original_extender)) {
      //           new_group << sel_i;
      //           new_group += generate_extension(sel_i, extender, new_Node);
      //         }
      //         else {
      //           cerr << "not what you think is happening!" << endl;
      //         }
      //       }
      //     }
      //   }
      //   ruleset_to_extend[2] = new_group;
      // }

      // if (selector_to_extend.type() != Node::selector) {
      //   switch (extender.type())
      //   {
      //     case Node::simple_selector:
      //     case Node::attribute_selector:
      //     case Node::simple_selector_sequence:
      //     case Node::selector: {
      //       cerr << "EXTENDING " << selector_to_extend.to_string() << " WITH " << extender.to_string() << endl;
      //       if (selector_to_extend.type() == Node::selector_group) {
      //         selector_to_extend << extender;
      //       }
      //       else {
      //         Node new_group(new_Node(Node::selector_group, selector_to_extend.path(), selector_to_extend.line(), 2));
      //         new_group << selector_to_extend << extender;
      //         ruleset_to_extend[2] = new_group;
      //       }
      //     } break;
      //     default: {
      //     // handle the other cases later
      //     }
      //   }
      // }
      // else {
      //   switch (extender.type())
      //   {
      //     case Node::simple_selector:
      //     case Node::attribute_selector:
      //     case Node::simple_selector_sequence: {
      //       Node new_ext(new_Node(selector_to_extend));
      //       new_ext.back() = extender;
      //       if (selector_to_extend.type() == Node::selector_group) {
      //         selector_to_extend << new_ext;
      //       }
      //       else {
      //         Node new_group(new_Node(Node::selector_group, selector_to_extend.path(), selector_to_extend.line(), 2));
      //         new_group << selector_to_extend << new_ext;
      //         ruleset_to_extend[2] = new_group;
      //       }
      //     } break;

      //     case Node::selector: {
      //       Node new_ext1(new_Node(Node::selector, selector_to_extend.path(), selector_to_extend.line(), selector_to_extend.size() + extender.size() - 1));
      //       Node new_ext2(new_Node(Node::selector, selector_to_extend.path(), selector_to_extend.line(), selector_to_extend.size() + extender.size() - 1));
      //       new_ext1 += selector_prefix(selector_to_extend, new_Node);
      //       new_ext1 += extender;
      //       new_ext2 += selector_prefix(extender, new_Node);
      //       new_ext2 += selector_prefix(selector_to_extend, new_Node);
      //       new_ext2 << extender.back();
      //       if (selector_to_extend.type() == Node::selector_group) {
      //         selector_to_extend << new_ext1 << new_ext2;
      //       }
      //       else {
      //         Node new_group(new_Node(Node::selector_group, selector_to_extend.path(), selector_to_extend.line(), 2));
      //         new_group << selector_to_extend << new_ext1 << new_ext2;
      //         ruleset_to_extend[2] = new_group;
      //       }
      //     } break;

      //     default: {
      //       // something
      //     } break;
      //   }
      // }
    }
  }

  // Helper for generating selector extensions; called for each extendee and
  // extender in a pair of selector groups.

  Node generate_extension(Node extendee, Node extender, Node_Factory& new_Node)
  {
    Node new_group(new_Node(Node::selector_group, extendee.path(), extendee.line(), 1));
    if (extendee.type() != Node::selector) {
      switch (extender.type())
      {
        case Node::simple_selector:
        case Node::attribute_selector:
        case Node::simple_selector_sequence:
        case Node::selector: {
          new_group << extender;
          return new_group;
        } break;
        default: {
          // not sure why selectors sometimes get wrapped in a singleton group
          return generate_extension(extendee, extender[0], new_Node);
        } break;
      }
    }
    else {
      switch (extender.type())
      {
        case Node::simple_selector:
        case Node::attribute_selector:
        case Node::simple_selector_sequence: {
          Node new_ext(new_Node(Node::selector, extendee.path(), extendee.line(), extendee.size()));
          for (size_t i = 0, S = extendee.size() - 1; i < S; ++i) {
            new_ext << extendee[i];
          }
          new_ext << extender;
          new_group << new_ext;
          return new_group;
        } break;

        case Node::selector: {
          Node new_ext1(new_Node(Node::selector, extendee.path(), extendee.line(), extendee.size() + extender.size() - 1));
          Node new_ext2(new_Node(Node::selector, extendee.path(), extendee.line(), extendee.size() + extender.size() - 1));
          new_ext1 += selector_prefix(extendee, new_Node);
          new_ext1 += extender;
          new_ext2 += selector_prefix(extender, new_Node);
          new_ext2 += selector_prefix(extendee, new_Node);
          new_ext2 << extender.back();
          new_group << new_ext1 << new_ext2;
          return new_group;
        } break;

        default: {
          // something
        } break;
      }
    }
    return Node();
  }

  // Helpers for extracting subsets of selectors

  Node selector_prefix(Node sel, Node_Factory& new_Node)
  {
    switch (sel.type())
    {
      case Node::selector: {
        Node pre(new_Node(Node::selector, sel.path(), sel.line(), sel.size() - 1));
        for (size_t i = 0, S = sel.size() - 1; i < S; ++i) {
          pre << sel[i];
        }
        return pre;
      } break;

      default: {
        return new_Node(Node::selector, sel.path(), sel.line(), 0);
      } break;
    }
  }

  Node selector_base(Node sel)
  {
    switch (sel.type())
    {
      case Node::selector: {
        return sel.back();
      } break;

      default: {
        return sel;
      } break;
    }
  }

  static Node selector_but(Node sel, Node_Factory& new_Node, size_t start, size_t from_end)
  {
    switch (sel.type())
    {
      case Node::selector: {
        Node bf(new_Node(Node::selector, sel.path(), sel.line(), sel.size() - 1));
        for (size_t i = start, S = sel.size() - from_end; i < S; ++i) {
          bf << sel[i];
        }
        return bf;
      } break;

      default: {
        return new_Node(Node::selector, sel.path(), sel.line(), 0);
      } break;
    }
  }

  Node selector_butfirst(Node sel, Node_Factory& new_Node)
  { return selector_but(sel, new_Node, 1, 0); }

  Node selector_butlast(Node sel, Node_Factory& new_Node)
  { return selector_but(sel, new_Node, 0, 1); }

  void to_lowercase(string& s)
  { for (size_t i = 0, L = s.length(); i < L; ++i) s[i] = tolower(s[i]); }

}
