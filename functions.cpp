#ifndef SASS_PRELEXER_INCLUDED
#include "prelexer.hpp"
#endif
#include "node_factory.hpp"
#include "functions.hpp"
#include "error.hpp"
#include <iostream>
#include <cmath>
using std::cerr; using std::endl;

namespace Sass {
  namespace Functions {

    static void throw_eval_error(string message, string path, size_t line)
    {
      if (!path.empty() && Prelexer::string_constant(path.c_str()))
        path = path.substr(1, path.length() - 1);

      throw Error(Error::evaluation, path, line, message);
    }

    // RGB Functions ///////////////////////////////////////////////////////

    Function_Descriptor rgb_descriptor = 
    { "rgb", "$red", "$green", "$blue", 0 };
    Node rgb(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node r(bindings[parameters[0].token()]);
      Node g(bindings[parameters[1].token()]);
      Node b(bindings[parameters[2].token()]);
      if (!(r.type() == Node::number && g.type() == Node::number && b.type() == Node::number)) {
        throw_eval_error("arguments for rgb must be numbers", r.path(), r.line());
      }
      return new_Node(r.path(), r.line(), r.numeric_value(), g.numeric_value(), b.numeric_value(), 1.0);
    }

    Function_Descriptor rgba_4_descriptor = 
    { "rgba 4", "$red", "$green", "$blue", "$alpha", 0 };
    Node rgba_4(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node r(bindings[parameters[0].token()]);
      Node g(bindings[parameters[1].token()]);
      Node b(bindings[parameters[2].token()]);
      Node a(bindings[parameters[3].token()]);
      if (!(r.type() == Node::number && g.type() == Node::number && b.type() == Node::number && a.type() == Node::number)) {
        throw_eval_error("arguments for rgba must be numbers", r.path(), r.line());
      }
      return new_Node(r.path(), r.line(), r.numeric_value(), g.numeric_value(), b.numeric_value(), a.numeric_value());
    }
    
    Function_Descriptor rgba_2_descriptor = 
    { "rgba 2", "$color", "$alpha", 0 };
    Node rgba_2(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameters[0].token()]);
      Node r(color[0]);
      Node g(color[1]);
      Node b(color[2]);
      Node a(bindings[parameters[1].token()]);
      if (color.type() != Node::numeric_color || a.type() != Node::number) throw_eval_error("arguments to rgba must be a color and a number", color.path(), color.line());
      return new_Node(color.path(), color.line(), r.numeric_value(), g.numeric_value(), b.numeric_value(), a.numeric_value());
    }
    
    Function_Descriptor red_descriptor =
    { "red", "$color", 0 };
    Node red(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameters[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to red must be a color", color.path(), color.line());
      return color[0];
    }
    
    Function_Descriptor green_descriptor =
    { "green", "$color", 0 };
    Node green(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameters[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to green must be a color", color.path(), color.line());
      return color[1];
    }
    
    Function_Descriptor blue_descriptor =
    { "blue", "$color", 0 };
    Node blue(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameters[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to blue must be a color", color.path(), color.line());
      return color[2];
    }
    
    Node mix_impl(Node color1, Node color2, double weight, Node_Factory& new_Node) {
      if (!(color1.type() == Node::numeric_color && color2.type() == Node::numeric_color)) {
        throw_eval_error("first two arguments to mix must be colors", color1.path(), color1.line());
      }
      double p = weight/100;
      double w = 2*p - 1;
      double a = color1[3].numeric_value() - color2[3].numeric_value();
      
      double w1 = (((w * a == -1) ? w : (w + a)/(1 + w*a)) + 1)/2.0;
      double w2 = 1 - w1;
      
      Node mixed(new_Node(Node::numeric_color, color1.path(), color1.line(), 4));
      for (int i = 0; i < 3; ++i) {
        mixed << new_Node(mixed.path(), mixed.line(),
                          w1*color1[i].numeric_value() + w2*color2[i].numeric_value());
      }
      double alpha = color1[3].numeric_value()*p + color2[3].numeric_value()*(1-p);
      mixed << new_Node(mixed.path(), mixed.line(), alpha);
      return mixed;
    }
    
    Function_Descriptor mix_2_descriptor =
    { "mix 2", "$color1", "$color2", 0 };
    Node mix_2(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      return mix_impl(bindings[parameters[0].token()], bindings[parameters[1].token()], 50, new_Node);
    }
    
    Function_Descriptor mix_3_descriptor =
    { "mix 3", "$color1", "$color2", "$weight", 0 };
    Node mix_3(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node percentage(bindings[parameters[2].token()]);
      if (!(percentage.type() == Node::number || percentage.type() == Node::numeric_percentage || percentage.type() == Node::numeric_dimension)) {
        throw_eval_error("third argument to mix must be numeric", percentage.path(), percentage.line());
      }
      return mix_impl(bindings[parameters[0].token()],
                      bindings[parameters[1].token()],
                      percentage.numeric_value(),
                      new_Node);
    }
    
    // HSL Functions ///////////////////////////////////////////////////////
    
    double h_to_rgb(double m1, double m2, double h) {
      if (h < 0) ++h;
      if (h > 1) --h;
      if (h*6.0 < 1) return m1 + (m2 - m1)*h*6;
      if (h*2.0 < 1) return m2;
      if (h*3.0 < 2) return m1 + (m2 - m1) * (2.0/3.0 - h)*6;
      return m1;
    }

    Node hsla_impl(double h, double s, double l, double a, Node_Factory& new_Node) {
      h = static_cast<double>(((static_cast<int>(h) % 360) + 360) % 360) / 360.0;
      s = s / 100.0;
      l = l / 100.0;

      double m2;
      if (l <= 0.5) m2 = l*(s+1.0);
      else m2 = l+s-l*s;
      double m1 = l*2-m2;
      double r = h_to_rgb(m1, m2, h+1.0/3.0) * 255.0;
      double g = h_to_rgb(m1, m2, h) * 255.0;
      double b = h_to_rgb(m1, m2, h-1.0/3.0) * 255.0;
      
      return new_Node("", 0, r, g, b, a);
    }

    Function_Descriptor hsla_descriptor =
    { "hsla", "$hue", "$saturation", "$lightness", "$alpha", 0 };
    Node hsla(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      if (!(bindings[parameters[0].token()].is_numeric() &&
            bindings[parameters[1].token()].is_numeric() &&
            bindings[parameters[2].token()].is_numeric() &&
            bindings[parameters[3].token()].is_numeric())) {
        throw_eval_error("arguments to hsla must be numeric", bindings[parameters[0].token()].path(), bindings[parameters[0].token()].line());
      }  
      double h = bindings[parameters[0].token()].numeric_value();
      double s = bindings[parameters[1].token()].numeric_value();
      double l = bindings[parameters[2].token()].numeric_value();
      double a = bindings[parameters[3].token()].numeric_value();
      Node color(hsla_impl(h, s, l, a, new_Node));
      // color.line() = bindings[parameters[0].token()].line();
      return color;
    }
    
    Function_Descriptor hsl_descriptor =
    { "hsl", "$hue", "$saturation", "$lightness", 0 };
    Node hsl(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      if (!(bindings[parameters[0].token()].is_numeric() &&
            bindings[parameters[1].token()].is_numeric() &&
            bindings[parameters[2].token()].is_numeric())) {
        throw_eval_error("arguments to hsl must be numeric", bindings[parameters[0].token()].path(), bindings[parameters[0].token()].line());
      }  
      double h = bindings[parameters[0].token()].numeric_value();
      double s = bindings[parameters[1].token()].numeric_value();
      double l = bindings[parameters[2].token()].numeric_value();
      Node color(hsla_impl(h, s, l, 1, new_Node));
      // color.line() = bindings[parameters[0].token()].line();
      return color;
    }
    
    Function_Descriptor invert_descriptor =
    { "invert", "$color", 0 };
    Node invert(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameters[0].token()]);
      if (orig.type() != Node::numeric_color) throw_eval_error("argument to invert must be a color", orig.path(), orig.line());
      return new_Node(orig.path(), orig.line(),
                      255 - orig[0].numeric_value(),
                      255 - orig[1].numeric_value(),
                      255 - orig[2].numeric_value(),
                      orig[3].numeric_value());
    }
    
    // Opacity Functions ///////////////////////////////////////////////////
    
    Function_Descriptor alpha_descriptor =
    { "alpha", "$color", 0 };
    Function_Descriptor opacity_descriptor =
    { "opacity", "$color", 0 };
    Node alpha(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameters[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to alpha must be a color", color.path(), color.line());
      return color[3];
    }
    
    Function_Descriptor opacify_descriptor =
    { "opacify", "$color", "$amount", 0 };
    Function_Descriptor fade_in_descriptor =
    { "fade_in", "$color", "$amount", 0 };
    Node opacify(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameters[0].token()]);
      Node delta(bindings[parameters[1].token()]);
      if (color.type() != Node::numeric_color || !delta.is_numeric()) {
        throw_eval_error("arguments to opacify/fade_in must be a color and a numeric value", color.path(), color.line());
      }
      if (delta.numeric_value() < 0 || delta.numeric_value() > 1) {
        throw_eval_error("amount must be between 0 and 1 for opacify/fade-in", delta.path(), delta.line());
      }
      double alpha = color[3].numeric_value() + delta.numeric_value();
      if (alpha > 1) alpha = 1;
      else if (alpha < 0) alpha = 0;
      return new_Node(color.path(), color.line(),
                      color[0].numeric_value(), color[1].numeric_value(), color[2].numeric_value(), alpha);
    }
    
    Function_Descriptor transparentize_descriptor =
    { "transparentize", "$color", "$amount", 0 };
    Function_Descriptor fade_out_descriptor =
    { "fade_out", "$color", "$amount", 0 };
    Node transparentize(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameters[0].token()]);
      Node delta(bindings[parameters[1].token()]);
      if (color.type() != Node::numeric_color || !delta.is_numeric()) {
        throw_eval_error("arguments to transparentize/fade_out must be a color and a numeric value", color.path(), color.line());
      }
      if (delta.numeric_value() < 0 || delta.numeric_value() > 1) {
        throw_eval_error("amount must be between 0 and 1 for transparentize/fade-out", delta.path(), delta.line());
      }
      double alpha = color[3].numeric_value() - delta.numeric_value();
      if (alpha > 1) alpha = 1;
      else if (alpha < 0) alpha = 0;
      return new_Node(color.path(), color.line(),
                      color[0].numeric_value(), color[1].numeric_value(), color[2].numeric_value(), alpha);
    }
      
    // String Functions ////////////////////////////////////////////////////
    
    Function_Descriptor unquote_descriptor =
    { "unquote", "$string", 0 };
    Node unquote(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node cpy(new_Node(bindings[parameters[0].token()]));
      // if (cpy.type() != Node::string_constant /* && cpy.type() != Node::concatenation */) {
      //   throw_eval_error("argument to unquote must be a string", cpy.path(), cpy.line());
      // }
      cpy.is_unquoted() = true;
      cpy.is_quoted() = false;
      return cpy;
    }
    
    Function_Descriptor quote_descriptor =
    { "quote", "$string", 0 };
    Node quote(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameters[0].token()]);
      switch (orig.type())
      {
        default: {
          throw_eval_error("argument to quote must be a string or identifier", orig.path(), orig.line());
        } break;

        case Node::string_constant:
        case Node::string_schema:
        case Node::identifier:
        case Node::identifier_schema:
        case Node::concatenation: {
          Node cpy(new_Node(orig));
          cpy.is_unquoted() = false;
          cpy.is_quoted() = true;
          return cpy;
        } break;
      }
      return orig;
    }
    
    // Number Functions ////////////////////////////////////////////////////
    
    Function_Descriptor percentage_descriptor =
    { "percentage", "$value", 0 };
    Node percentage(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameters[0].token()]);
      if (orig.type() != Node::number) {
        throw_eval_error("argument to percentage must be a unitless number", orig.path(), orig.line());
      }
      return new_Node(orig.path(), orig.line(), orig.numeric_value() * 100, Node::numeric_percentage);
    }

    Function_Descriptor round_descriptor =
    { "round", "$value", 0 };
    Node round(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameters[0].token()]);
      switch (orig.type())
      {
        case Node::numeric_dimension: {
          return new_Node(orig.path(), orig.line(),
                          std::floor(orig.numeric_value() + 0.5), orig.unit());
        } break;

        case Node::number: {
          return new_Node(orig.path(), orig.line(),
                          std::floor(orig.numeric_value() + 0.5));
        } break;

        case Node::numeric_percentage: {
          return new_Node(orig.path(), orig.line(),
                          std::floor(orig.numeric_value() + 0.5),
                          Node::numeric_percentage);
        } break;

        default: {
          throw_eval_error("argument to round must be numeric", orig.path(), orig.line());
        } break;
      }
      // unreachable statement
      return Node();
    }

    Function_Descriptor ceil_descriptor =
    { "ceil", "$value", 0 };
    Node ceil(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameters[0].token()]);
      switch (orig.type())
      {
        case Node::numeric_dimension: {
          return new_Node(orig.path(), orig.line(),
                          std::ceil(orig.numeric_value()), orig.unit());
        } break;

        case Node::number: {
          return new_Node(orig.path(), orig.line(),
                          std::ceil(orig.numeric_value()));
        } break;

        case Node::numeric_percentage: {
          return new_Node(orig.path(), orig.line(),
                          std::ceil(orig.numeric_value()),
                          Node::numeric_percentage);
        } break;

        default: {
          throw_eval_error("argument to ceil must be numeric", orig.path(), orig.line());
        } break;
      }
      // unreachable statement
      return Node();
    }

    Function_Descriptor floor_descriptor =
    { "floor", "$value", 0 };
    Node floor(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameters[0].token()]);
      switch (orig.type())
      {
        case Node::numeric_dimension: {
          return new_Node(orig.path(), orig.line(),
                          std::floor(orig.numeric_value()), orig.unit());
        } break;

        case Node::number: {
          return new_Node(orig.path(), orig.line(),
                          std::floor(orig.numeric_value()));
        } break;

        case Node::numeric_percentage: {
          return new_Node(orig.path(), orig.line(),
                          std::floor(orig.numeric_value()),
                          Node::numeric_percentage);
        } break;

        default: {
          throw_eval_error("argument to floor must be numeric", orig.path(), orig.line());
        } break;
      }
      // unreachable statement
      return Node();
    }

    Function_Descriptor abs_descriptor =
    { "abs", "$value", 0 };
    Node abs(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameters[0].token()]);
      switch (orig.type())
      {
        case Node::numeric_dimension: {
          return new_Node(orig.path(), orig.line(),
                          std::abs(orig.numeric_value()), orig.unit());
        } break;

        case Node::number: {
          return new_Node(orig.path(), orig.line(),
                          std::abs(orig.numeric_value()));
        } break;

        case Node::numeric_percentage: {
          return new_Node(orig.path(), orig.line(),
                          std::abs(orig.numeric_value()),
                          Node::numeric_percentage);
        } break;

        default: {
          throw_eval_error("argument to abs must be numeric", orig.path(), orig.line());
        } break;
      }
      // unreachable statement
      return Node();
    }
    
    // List Functions //////////////////////////////////////////////////////

    Function_Descriptor length_descriptor =
    { "length", "$list", 0 };
    Node length(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node arg(bindings[parameters[0].token()]);
      switch (arg.type())
      {
        case Node::space_list:
        case Node::comma_list: {
          return new_Node(arg.path(), arg.line(), arg.size());
        } break;

        case Node::nil: {
          return new_Node(arg.path(), arg.line(), 0);
        } break;

        default: {
          // single objects should be reported as lists of length 1
          return new_Node(arg.path(), arg.line(), 1);
        } break;
      }
      // unreachable statement
      return Node();
    }
    
    Function_Descriptor nth_descriptor =
    { "nth", "$list", "$n", 0 };
    Node nth(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node l(bindings[parameters[0].token()]);
      Node n(bindings[parameters[1].token()]);
      if (n.type() != Node::number) {
        throw_eval_error("second argument to nth must be a number", n.path(), n.line());
      }
      if (l.type() == Node::nil) {
        throw_eval_error("cannot index into an empty list", l.path(), l.line());
      }
      // wrap the first arg if it isn't a list
      if (l.type() != Node::space_list && l.type() != Node::comma_list) {
        l = new_Node(Node::space_list, l.path(), l.line(), 1) << l;
      }
      double n_prim = n.numeric_value();
      if (n_prim < 1 || n_prim > l.size()) {
        throw_eval_error("out of range index for nth", n.path(), n.line());
      }
      return l[n_prim - 1];
    }
    
    extern const char separator_kwd[] = "$separator";
    Node join_impl(const Node parameters, map<Token, Node>& bindings, bool has_sep, Node_Factory& new_Node) {
      // if the args aren't lists, turn them into singleton lists
      Node l1(bindings[parameters[0].token()]);
      if (l1.type() != Node::space_list && l1.type() != Node::comma_list && l1.type() != Node::nil) {
        l1 = new_Node(Node::space_list, l1.path(), l1.line(), 1) << l1;
      }
      Node l2(bindings[parameters[1].token()]);
      if (l2.type() != Node::space_list && l2.type() != Node::comma_list && l2.type() != Node::nil) {
        l2 = new_Node(Node::space_list, l2.path(), l2.line(), 1) << l2;
      }
      // nil + nil = nil
      if (l1.type() == Node::nil && l2.type() == Node::nil) {
        return new_Node(Node::nil, l1.path(), l1.line(), 0);
      }
      // figure out the combined size in advance
      size_t size = 0;
      if (l1.type() != Node::nil) size += l1.size();
      if (l2.type() != Node::nil) size += l2.size();
      // figure out the result type in advance
      Node::Type rtype = Node::space_list;
      if (has_sep) {
        string sep(bindings[parameters[2].token()].token().unquote());
        if (sep == "comma")      rtype = Node::comma_list;
        else if (sep == "space") rtype = Node::space_list;
        else if (sep == "auto")  rtype = l1.type();
        else {
          throw_eval_error("third argument to join must be 'space', 'comma', or 'auto'", l2.path(), l2.line());
        }
      }
      else if (l1.type() != Node::nil) rtype = l1.type();
      else if (l2.type() != Node::nil) rtype = l2.type();
      // accumulate the result
      Node lr(new_Node(rtype, l1.path(), l1.line(), size));
      if (l1.type() != Node::nil) lr += l1;
      if (l2.type() != Node::nil) lr += l2;
      return lr;
    }
    
    Function_Descriptor join_2_descriptor =
    { "join 2", "$list1", "$list2", 0 };
    Node join_2(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      return join_impl(parameters, bindings, false, new_Node);
    }
    
    Function_Descriptor join_3_descriptor =
    { "join 3", "$list1", "$list2", "$separator", 0 };
    Node join_3(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      return join_impl(parameters, bindings, true, new_Node);
    }

    Node append_impl(const Node parameters, map<Token, Node>& bindings, bool has_sep, Node_Factory& new_Node) {
      Node list(bindings[parameters[0].token()]);
      switch (list.type())
      {
        case Node::space_list:
        case Node::comma_list:
        case Node::nil: {
          // do nothing
        } break;
        // if the first arg isn't a list, wrap it in a singleton
        default: {
          list = (new_Node(Node::space_list, list.path(), list.line(), 1) << list);
        } break;
      }
      Node::Type sep_type = list.type();
      if (has_sep) {
        string sep_string = bindings[parameters[2].token()].token().unquote();
        if (sep_string == "comma")      sep_type = Node::comma_list;
        else if (sep_string == "space") sep_type = Node::space_list;
        else if (sep_string == "auto")  sep_type = list.type();
        else throw_eval_error("third argument to append must be 'space', 'comma', or 'auto'", list.path(), list.line());
      }
      Node new_list(new_Node(sep_type, list.path(), list.line(), list.size() + 1));
      new_list += list;
      new_list << bindings[parameters[1].token()];
      return new_list;
    }

    Function_Descriptor append_2_descriptor =
    { "append 2", "$list", "$val", 0 };
    Node append_2(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      return append_impl(parameters, bindings, false, new_Node);
    }

    Function_Descriptor append_3_descriptor =
    { "append 3", "$list", "$val", "$separator", 0 };
    Node append_3(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      return append_impl(parameters, bindings, true, new_Node);
    }

    Node compact(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      size_t num_args     = bindings.size();
      Node::Type sep_type = Node::comma_list;
      Node list;
      Node arg1(bindings[parameters[0].token()]);
      if (num_args == 1 && (arg1.type() == Node::space_list ||
                            arg1.type() == Node::comma_list ||
                            arg1.type() == Node::nil)) {
        list = new_Node(arg1.type(), arg1.path(), arg1.line(), arg1.size());
        list += arg1;
      }
      else {
        list = new_Node(sep_type, arg1.path(), arg1.line(), num_args);
        for (size_t i = 0; i < num_args; ++i) {
          list << bindings[parameters[i].token()];
        }
      }
      Node new_list(new_Node(list.type(), list.path(), list.line(), 0));
      for (size_t i = 0, S = list.size(); i < S; ++i) {
        if ((list[i].type() != Node::boolean) || list[i].boolean_value()) {
          new_list << list[i];
        }
      }
      return new_list.size() ? new_list : new_Node(Node::nil, list.path(), list.line(), 0);
    }

    Function_Descriptor compact_1_descriptor =
    { "compact 1", "$arg1", 0 };
    Function_Descriptor compact_2_descriptor =
    { "compact 2", "$arg1", "$arg2", 0 };
    Function_Descriptor compact_3_descriptor =
    { "compact 3", "$arg1", "$arg2", "$arg3", 0 };
    Function_Descriptor compact_4_descriptor =
    { "compact 4", "$arg1", "$arg2", "$arg3", "$arg4", 0 };
    Function_Descriptor compact_5_descriptor =
    { "compact 5", "$arg1", "$arg2", "$arg3", "$arg4", "$arg5", 0 };
    Function_Descriptor compact_6_descriptor =
    { "compact 6", "$arg1", "$arg2", "$arg3", "$arg4", "$arg5", 
                   "$arg6", 0 };
    Function_Descriptor compact_7_descriptor =
    { "compact 7", "$arg1", "$arg2", "$arg3", "$arg4", "$arg5", 
                   "$arg6", "$arg7", 0 };
    Function_Descriptor compact_8_descriptor =
    { "compact 8", "$arg1", "$arg2", "$arg3", "$arg4", "$arg5", 
                   "$arg6", "$arg7", "$arg8", 0 };
    Function_Descriptor compact_9_descriptor =
    { "compact 9", "$arg1", "$arg2", "$arg3", "$arg4", "$arg5", 
                   "$arg6", "$arg7", "$arg8", "$arg9", 0 };
    Function_Descriptor compact_10_descriptor =
    { "compact 10", "$arg1", "$arg2", "$arg3", "$arg4", "$arg5", 
                    "$arg6", "$arg7", "$arg8", "$arg9", "$arg10", 0 };
    Function_Descriptor compact_11_descriptor =
    { "compact 11", "$arg1", "$arg2", "$arg3", "$arg4", "$arg5", "$arg6",
                    "$arg7", "$arg8", "$arg9", "$arg10", "$arg11", 0 };
    Function_Descriptor compact_12_descriptor =
    { "compact 12", "$arg1", "$arg2", "$arg3", "$arg4", "$arg5", "arg6", 
                    "$arg7", "$arg8", "$arg9", "$arg10", "$arg11", "$arg12", 0 };


    
    // Introspection Functions /////////////////////////////////////////////
    
    extern const char number_name[] = "number";
    extern const char string_name[] = "string";
    extern const char bool_name[]   = "bool";
    extern const char color_name[]  = "color";
    extern const char list_name[]   = "list";
    
    Function_Descriptor type_of_descriptor =
    { "type-of", "$value", 0 };
    Node type_of(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node val(bindings[parameters[0].token()]);
      Token type_name;
      switch (val.type())
      {
        case Node::number:
        case Node::numeric_dimension:
        case Node::numeric_percentage: {
          type_name = Token::make(number_name);
        } break;
        case Node::boolean: {
          type_name = Token::make(bool_name);
        } break;
        case Node::string_constant:
        case Node::value_schema: {
          type_name = Token::make(string_name);
        } break;
        case Node::numeric_color: {
          type_name = Token::make(color_name);
        } break;
        case Node::comma_list:
        case Node::space_list:
        case Node::nil: {
          type_name = Token::make(list_name);
        } break;
        default: {
          type_name = Token::make(string_name);
        } break;
      }
      Node type(new_Node(Node::string_constant, val.path(), val.line(), type_name));
      type.is_unquoted() = true;
      return type;
    }
    
    extern const char empty_str[] = "";
    extern const char percent_str[] = "%";
    
    Function_Descriptor unit_descriptor =
    { "unit", "$number", 0 };
    Node unit(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node val(bindings[parameters[0].token()]);
      switch (val.type())
      {
        case Node::number: {
          return new_Node(Node::string_constant, val.path(), val.line(), Token::make(empty_str));
        } break;

        case Node::numeric_dimension:
        case Node::numeric_percentage: {
          return new_Node(Node::string_constant, val.path(), val.line(), val.unit());
        } break;

        default: {
          throw_eval_error("argument to unit must be numeric", val.path(), val.line());
        } break;
      }
      // unreachable statement
      return Node();
    }

    extern const char true_str[]  = "true";
    extern const char false_str[] = "false";

    Function_Descriptor unitless_descriptor =
    { "unitless", "$number", 0 };
    Node unitless(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node val(bindings[parameters[0].token()]);
      switch (val.type())
      {
        case Node::number: {
          return new_Node(Node::boolean, val.path(), val.line(), true);
        } break;

        case Node::numeric_percentage:
        case Node::numeric_dimension: {
          return new_Node(Node::boolean, val.path(), val.line(), false);
        } break;

        default: {
          throw_eval_error("argument to unitless must be numeric", val.path(), val.line());
        } break;
      }
      // unreachable statement
      return Node();
    }
    
    Function_Descriptor comparable_descriptor =
    { "comparable", "$number_1", "$number_2", 0 };
    Node comparable(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node n1(bindings[parameters[0].token()]);
      Node n2(bindings[parameters[1].token()]);
      Node::Type t1 = n1.type();
      Node::Type t2 = n2.type();
      if ((t1 == Node::number && n2.is_numeric()) ||
          (n1.is_numeric() && t2 == Node::number)) {
        return new_Node(Node::boolean, n1.path(), n1.line(), true);
      }
      else if (t1 == Node::numeric_percentage && t2 == Node::numeric_percentage) {
        return new_Node(Node::boolean, n1.path(), n1.line(), true);
      }
      else if (t1 == Node::numeric_dimension && t2 == Node::numeric_dimension) {
        string u1(n1.unit().to_string());
        string u2(n2.unit().to_string());
        if ((u1 == "ex" && u2 == "ex") ||
            (u1 == "em" && u2 == "em") ||
            ((u1 == "in" || u1 == "cm" || u1 == "mm" || u1 == "pt" || u1 == "pc") &&
             (u2 == "in" || u2 == "cm" || u2 == "mm" || u2 == "pt" || u2 == "pc"))) {
          return new_Node(Node::boolean, n1.path(), n1.line(), true);
        }
        else {
          return new_Node(Node::boolean, n1.path(), n1.line(), false);
        }
      }
      else if (!n1.is_numeric() && !n2.is_numeric()) {
        throw_eval_error("arguments to comparable must be numeric", n1.path(), n1.line());
      }
      // default to false if we missed anything
      return new_Node(Node::boolean, n1.path(), n1.line(), false);
    }
    
    // Boolean Functions ///////////////////////////////////////////////////
    Function_Descriptor not_descriptor =
    { "not", "$value", 0 };
    Node not_impl(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node val(bindings[parameters[0].token()]);
      if (val.type() == Node::boolean && val.boolean_value() == false) {
        return new_Node(Node::boolean, val.path(), val.line(), true);
      }
      else {
        return new_Node(Node::boolean, val.path(), val.line(), false);
      }
    }

    Function_Descriptor if_descriptor =
    { "if", "$predicate", "$consequent", "$alternative", 0 };
    Node if_impl(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node) {
      Node predicate(bindings[parameters[0].token()]);
      Node consequent(bindings[parameters[1].token()]);
      Node alternative(bindings[parameters[2].token()]);

      if (predicate.type() == Node::boolean && predicate.boolean_value() == false) return alternative;
      return consequent;
    }

  }
}
