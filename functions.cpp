#ifndef SASS_PRELEXER
#include "prelexer.hpp"
#endif

#include "node_factory.hpp"
#include "functions.hpp"
#include "context.hpp"
#include "document.hpp"
#include "eval_apply.hpp"
#include "error.hpp"

#include <iostream>
#include <cmath>
#include <algorithm>

using std::cerr; using std::endl;

namespace Sass {

  // this constructor needs context.hpp, so it can't be defined in functions.hpp
  Function::Function(char* signature, Primitive ip, Context& ctx)
  : definition(Node()),
    primitive(ip),
    overloaded(false)
  {
    Document sig_doc(Document::make_from_source_chars(ctx, signature));
    sig_doc.lex<Prelexer::identifier>();
    name = sig_doc.lexed.to_string();
    parameters = sig_doc.parse_parameters();
    parameter_names = ctx.new_Node(Node::parameters, "[PRIMITIVE FUNCTIONS]", 0, parameters.size());
    for (size_t i = 0, S = parameters.size(); i < S; ++i) {
      Node param(parameters[i]);
      if (param.type() == Node::variable) {
        parameter_names << param;
      }
      else {
        parameter_names << param[0];
        // assume it's safe to evaluate default args just once at initialization
        param[1] = eval(param[1], Node(), ctx.global_env, ctx.function_env, ctx.new_Node, ctx);
      }
    }
  }


  namespace Functions {

    static void throw_eval_error(string message, string path, size_t line)
    {
      if (!path.empty() && Prelexer::string_constant(path.c_str()))
        path = path.substr(1, path.length() - 1);

      throw Error(Error::evaluation, path, line, message);
    }

    extern Signature foo_sig = "foo($x, $y, $z: \"hey\")";
    Node foo(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node arg1(bindings[parameter_names[0].token()]);
      Node arg2(bindings[parameter_names[1].token()]);
      Node arg3(bindings[parameter_names[2].token()]);

      Node cat(new_Node(Node::concatenation, arg1.path(), arg1.line(), 3));
      cat << arg3 << arg2 << arg1;
      return cat;
    }

    // RGB Functions ///////////////////////////////////////////////////////

    extern Signature rgb_sig = "rgb($red, $green, $blue)";
    Node rgb(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node r(bindings[parameter_names[0].token()]);
      Node g(bindings[parameter_names[1].token()]);
      Node b(bindings[parameter_names[2].token()]);
      if (!(r.type() == Node::number && g.type() == Node::number && b.type() == Node::number)) {
        throw_eval_error("arguments for rgb must be numbers", r.path(), r.line());
      }
      return new_Node(r.path(), r.line(), r.numeric_value(), g.numeric_value(), b.numeric_value(), 1.0);
    }

    // TODO: SOMETHING SPECIAL FOR OVERLOADED FUNCTIONS
    extern Signature rgba_4_sig = "rgba($red, $green, $blue, $alpha)";
    Node rgba_4(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node r(bindings[parameter_names[0].token()]);
      Node g(bindings[parameter_names[1].token()]);
      Node b(bindings[parameter_names[2].token()]);
      Node a(bindings[parameter_names[3].token()]);
      if (!(r.type() == Node::number && g.type() == Node::number && b.type() == Node::number && a.type() == Node::number)) {
        throw_eval_error("arguments for rgba must be numbers", r.path(), r.line());
      }
      return new_Node(r.path(), r.line(), r.numeric_value(), g.numeric_value(), b.numeric_value(), a.numeric_value());
    }
    
    extern Signature rgba_2_sig = "rgba($color, $alpha)";
    Node rgba_2(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      Node r(color[0]);
      Node g(color[1]);
      Node b(color[2]);
      Node a(bindings[parameter_names[1].token()]);
      if (color.type() != Node::numeric_color || a.type() != Node::number) throw_eval_error("arguments to rgba must be a color and a number", color.path(), color.line());
      return new_Node(color.path(), color.line(), r.numeric_value(), g.numeric_value(), b.numeric_value(), a.numeric_value());
    }
    
    extern Signature red_sig = "red($color)";
    Node red(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to red must be a color", color.path(), color.line());
      return color[0];
    }
    
    extern Signature green_sig = "green($color)";
    Node green(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to green must be a color", color.path(), color.line());
      return color[1];
    }
    
    extern Signature blue_sig = "blue($color)";
    Node blue(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to blue must be a color", color.path(), color.line());
      return color[2];
    }

    extern Signature mix_sig = "mix($color-1, $color-2, $weight: 50%)";
    Node mix(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color1(bindings[parameter_names[0].token()]);
      Node color2(bindings[parameter_names[1].token()]);
      Node weight(bindings[parameter_names[2].token()]);

      if (color1.type() != Node::numeric_color) throw_eval_error("first argument to mix must be a color", color1.path(), color1.line());
      if (color2.type() != Node::numeric_color) throw_eval_error("second argument to mix must be a color", color2.path(), color2.line());
      if (!weight.is_numeric())                 throw_eval_error("third argument to mix must be numeric", weight.path(), weight.line());

      double p = weight.numeric_value()/100;
      double w = 2*p - 1;
      double a = color1[3].numeric_value() - color2[3].numeric_value();

      double w1 = (((w * a == -1) ? w : (w + a)/(1 + w*a)) + 1)/2.0;
      double w2 = 1 - w1;

      Node mixed(new_Node(Node::numeric_color, color1.path(), color1.line(), 4));
      for (int i = 0; i < 3; ++i) {
        mixed << new_Node(mixed.path(), mixed.line(), w1*color1[i].numeric_value() + w2*color2[i].numeric_value());
      }
      double alpha = color1[3].numeric_value()*p + color2[3].numeric_value()*(1-p);
      mixed << new_Node(mixed.path(), mixed.line(), alpha);
      return mixed;
    }
 
    // HSL Functions ///////////////////////////////////////////////////////

    // Utility rgb to hsl function so we can do hsl operations
    Node rgb_to_hsl(double r, double g, double b, Node_Factory& new_Node) {
      cerr << "rgb to hsl: " << r << " " << g << " " << b << endl;
      r /= 255.0; g /= 255.0; b /= 255.0;

      double max = std::max(r, std::max(g, b));
      double min = std::min(r, std::min(g, b));
      double del = max - min;

      double h = 0, s = 0, l = (max + min)/2;

      if (max == min) {
        h = s = 0; // achromatic
      }
      else {
        /*
        double delta = max - min;
        s = (l > 0.5) ? (2 - max - min) : (delta / (max + min));
        if (max == r)      h = (g - b) / delta + (g < b ? 6 : 0);
        else if (max == g) h = (b - r) / delta + 2;
        else if (max == b) h = (r - g) / delta + 4;
        h /= 6;
        */

        if (l < 0.5) s = del / (max + min);
        else         s = del / (2.0 - max - min);

        double dr = (((max - r)/6.0) + (del/2.0))/del;
        double dg = (((max - g)/6.0) + (del/2.0))/del;
        double db = (((max - b)/6.0) + (del/2.0))/del;

        if      (r == max) h = db - dg;
        else if (g == max) h = (1.0/3.0) + dr - db;
        else if (b == max) h = (2.0/3.0) + dg - dr;

        if      (h < 0) h += 1;
        else if (h > 1) h -= 1;
      }
      return new_Node("", 0, static_cast<int>(h*360)%360, s*100, l*100);
    }

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
      s = (s < 0) ? 0 : (s / 100.0);
      l = (l < 0) ? 0 : (l / 100.0);

      double m2;
      if (l <= 0.5) m2 = l*(s+1.0);
      else m2 = l+s-l*s;
      double m1 = l*2-m2;
      double r = h_to_rgb(m1, m2, h+1.0/3.0) * 255.0;
      double g = h_to_rgb(m1, m2, h) * 255.0;
      double b = h_to_rgb(m1, m2, h-1.0/3.0) * 255.0;

      return new_Node("", 0, r, g, b, a);
    }

    extern Signature hsla_sig = "hsla($hue, $saturation, $lightness, $alpha)";
    Node hsla(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      if (!(bindings[parameter_names[0].token()].is_numeric() &&
            bindings[parameter_names[1].token()].is_numeric() &&
            bindings[parameter_names[2].token()].is_numeric() &&
            bindings[parameter_names[3].token()].is_numeric())) {
        throw_eval_error("arguments to hsla must be numeric", bindings[parameter_names[0].token()].path(), bindings[parameter_names[0].token()].line());
      }  
      double h = bindings[parameter_names[0].token()].numeric_value();
      double s = bindings[parameter_names[1].token()].numeric_value();
      double l = bindings[parameter_names[2].token()].numeric_value();
      double a = bindings[parameter_names[3].token()].numeric_value();
      Node color(hsla_impl(h, s, l, a, new_Node));
      return color;
    }
    
    extern Signature hsl_sig = "hsl($hue, $saturation, $lightness)";
    Node hsl(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      if (!(bindings[parameter_names[0].token()].is_numeric() &&
            bindings[parameter_names[1].token()].is_numeric() &&
            bindings[parameter_names[2].token()].is_numeric())) {
        throw_eval_error("arguments to hsl must be numeric", bindings[parameter_names[0].token()].path(), bindings[parameter_names[0].token()].line());
      }  
      double h = bindings[parameter_names[0].token()].numeric_value();
      double s = bindings[parameter_names[1].token()].numeric_value();
      double l = bindings[parameter_names[2].token()].numeric_value();
      Node color(hsla_impl(h, s, l, 1, new_Node));
      return color;
    }

    extern Signature adjust_hue_sig = "adjust-hue($color, $degrees)";
    Node adjust_hue(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node rgb_col(bindings[parameter_names[0].token()]);
      Node degrees(bindings[parameter_names[1].token()]);
      if (rgb_col.type() != Node::numeric_color) throw_eval_error("first argument to adjust-hue must be a color", rgb_col.path(), rgb_col.line());
      if (!degrees.is_numeric()) throw_eval_error("second argument to adjust-hue must be numeric", degrees.path(), degrees.line());
      Node hsl_col(rgb_to_hsl(rgb_col[0].numeric_value(),
                              rgb_col[1].numeric_value(),
                              rgb_col[2].numeric_value(),
                              new_Node));
      return hsla_impl(hsl_col[0].numeric_value() + degrees.numeric_value(),
                       hsl_col[1].numeric_value(),
                       hsl_col[2].numeric_value(),
                       rgb_col[3].numeric_value(),
                       new_Node);
    }

    extern Signature adjust_color_sig = "adjust-color($color, $red: false, $green: false, $blue: false, $hue: false, $saturation: false, $lightness: false, $alpha: false)";
    Node adjust_color(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      Node r(bindings[parameter_names[1].token()]);
      Node g(bindings[parameter_names[2].token()]);
      Node b(bindings[parameter_names[3].token()]);
      Node h(bindings[parameter_names[4].token()]);
      Node s(bindings[parameter_names[5].token()]);
      Node l(bindings[parameter_names[6].token()]);
      Node a(bindings[parameter_names[7].token()]);

      bool no_rgb = r.is_false() && g.is_false() && b.is_false();
      bool no_hsl = h.is_false() && s.is_false() && l.is_false();

      if (!no_rgb && !no_hsl) {
        throw_eval_error("cannot specify RGB and HSL values for a color at the same time for 'adjust-color'", r.path(), r.line());
      }
      else if (!no_rgb) {
        double new_r = color[0].numeric_value() + (r.is_false() ? 0 : r.numeric_value());
        double new_g = color[1].numeric_value() + (g.is_false() ? 0 : g.numeric_value());
        double new_b = color[2].numeric_value() + (b.is_false() ? 0 : b.numeric_value());
        double new_a = color[3].numeric_value() + (a.is_false() ? 0 : a.numeric_value());
        return new_Node("", 0, new_r, new_g, new_b, new_a);
      }
      else if (!no_hsl) {
        Node hsl_node(rgb_to_hsl(color[0].numeric_value(),
                                 color[1].numeric_value(),
                                 color[2].numeric_value(),
                                 new_Node));
        double new_h = (h.is_false() ? 0 : h.numeric_value()) + hsl_node[0].numeric_value();
        double new_s = (s.is_false() ? 0 : s.numeric_value()) + hsl_node[1].numeric_value();
        double new_l = (l.is_false() ? 0 : l.numeric_value()) + hsl_node[2].numeric_value();
        double new_a = (a.is_false() ? 0 : a.numeric_value()) + color[3].numeric_value();
        return hsla_impl(new_h, new_s, new_l, new_a, new_Node);
      }
      else if (!a.is_false()) {
        return new_Node("", 0,
                        color[0].numeric_value(),
                        color[1].numeric_value(),
                        color[2].numeric_value(),
                        color[3].numeric_value() + a.numeric_value());
      }
      else {
        throw_eval_error("not enough arguments for 'adjust-color'", color.path(), color.line());
      }
      // unreachable statement
      return Node();
    }

    extern Signature change_color_sig = "change-color($color, $red: false, $green: false, $blue: false, $hue: false, $saturation: false, $lightness: false, $alpha: false)";
    Node change_color(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      Node r(bindings[parameter_names[1].token()]);
      Node g(bindings[parameter_names[2].token()]);
      Node b(bindings[parameter_names[3].token()]);
      Node h(bindings[parameter_names[4].token()]);
      Node s(bindings[parameter_names[5].token()]);
      Node l(bindings[parameter_names[6].token()]);
      Node a(bindings[parameter_names[7].token()]);

      bool no_rgb = r.is_false() && g.is_false() && b.is_false();
      bool no_hsl = h.is_false() && s.is_false() && l.is_false();

      if (!no_rgb && !no_hsl) {
        throw_eval_error("cannot specify RGB and HSL values for a color at the same time for 'change-color'", r.path(), r.line());
      }
      else if (!no_rgb) {
        double new_r = (r.is_false() ? color[0] : r).numeric_value();
        double new_g = (g.is_false() ? color[1] : g).numeric_value();
        double new_b = (b.is_false() ? color[2] : b).numeric_value();
        double new_a = (a.is_false() ? color[3] : a).numeric_value();
        return new_Node("", 0, new_r, new_g, new_b, new_a);
      }
      else if (!no_hsl) {
        cerr << color.to_string() << endl;
        Node hsl_node(rgb_to_hsl(color[0].numeric_value(),
                                 color[1].numeric_value(),
                                 color[2].numeric_value(),
                                 new_Node));
        cerr << hsl_node.to_string() << endl;
        double new_h = (h.is_false() ? hsl_node[0].numeric_value() : h.numeric_value());
        double new_s = (s.is_false() ? hsl_node[1].numeric_value() : s.numeric_value());
        double new_l = (l.is_false() ? hsl_node[2].numeric_value() : l.numeric_value());
        double new_a = (a.is_false() ? color[3].numeric_value() : a.numeric_value());
        return hsla_impl(new_h, new_s, new_l, new_a, new_Node);
      }
      else if (!a.is_false()) {
        return new_Node("", 0,
                        color[0].numeric_value(),
                        color[1].numeric_value(),
                        color[2].numeric_value(),
                        a.numeric_value());
      }
      else {
        throw_eval_error("not enough arguments for 'change-color'", color.path(), color.line());
      }
      // unreachable statement
      return Node();
    }

    extern Signature invert_sig = "invert($color)";
    Node invert(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameter_names[0].token()]);
      if (orig.type() != Node::numeric_color) throw_eval_error("argument to invert must be a color", orig.path(), orig.line());
      return new_Node(orig.path(), orig.line(),
                      255 - orig[0].numeric_value(),
                      255 - orig[1].numeric_value(),
                      255 - orig[2].numeric_value(),
                      orig[3].numeric_value());
    }
    
    // Opacity Functions ///////////////////////////////////////////////////
    
    extern Signature alpha_sig   = "alpha($color)";
    extern Signature opacity_sig = "opacity($color)";
    Node alpha(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to alpha/opacity must be a color", color.path(), color.line());
      return color[3];
    }
    
    extern Signature opacify_sig = "opacify($color, $amount)";
    extern Signature fade_in_sig = "fade-in($color, $amount)";
    Node opacify(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      Node delta(bindings[parameter_names[1].token()]);
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
    
    extern Signature transparentize_sig = "transparentize($color, $amount)";
    extern Signature fade_out_sig       = "fade-out($color, $amount)";
    Node transparentize(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node color(bindings[parameter_names[0].token()]);
      Node delta(bindings[parameter_names[1].token()]);
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
    
    extern Signature unquote_sig = "unquote($string)";
    Node unquote(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node cpy(new_Node(bindings[parameter_names[0].token()]));
      // if (cpy.type() != Node::string_constant /* && cpy.type() != Node::concatenation */) {
      //   throw_eval_error("argument to unquote must be a string", cpy.path(), cpy.line());
      // }
      cpy.is_unquoted() = true;
      cpy.is_quoted() = false;
      return cpy;
    }
    
    extern Signature quote_sig = "quote($string)";
    Node quote(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameter_names[0].token()]);
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
    
    extern Signature percentage_sig = "percentage($value)";
    Node percentage(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameter_names[0].token()]);
      if (orig.type() != Node::number) {
        throw_eval_error("argument to percentage must be a unitless number", orig.path(), orig.line());
      }
      return new_Node(orig.path(), orig.line(), orig.numeric_value() * 100, Node::numeric_percentage);
    }

    extern Signature round_sig = "round($value)";
    Node round(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameter_names[0].token()]);
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

    extern Signature ceil_sig = "ceil($value)";
    Node ceil(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameter_names[0].token()]);
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

    extern Signature floor_sig = "floor($value)";
    Node floor(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameter_names[0].token()]);
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

    extern Signature abs_sig = "abs($value)";
    Node abs(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node orig(bindings[parameter_names[0].token()]);
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

    extern Signature length_sig = "length($list)";
    Node length(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node arg(bindings[parameter_names[0].token()]);
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
    
    extern Signature nth_sig = "nth($list, $n)";
    Node nth(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node l(bindings[parameter_names[0].token()]);
      Node n(bindings[parameter_names[1].token()]);
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

    extern Signature join_sig = "join($list1, $list2, $separator: auto)";
    Node join(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      // if the args aren't lists, turn them into singleton lists
      Node l1(bindings[parameter_names[0].token()]);
      if (l1.type() != Node::space_list && l1.type() != Node::comma_list && l1.type() != Node::nil) {
        l1 = new_Node(Node::space_list, l1.path(), l1.line(), 1) << l1;
      }
      Node l2(bindings[parameter_names[1].token()]);
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

      string sep(bindings[parameter_names[2].token()].token().unquote());
      if (sep == "comma")      rtype = Node::comma_list;
      else if (sep == "space") rtype = Node::space_list;
      else if (sep == "auto")  rtype = l1.type();
      else {
        throw_eval_error("third argument to join must be 'space', 'comma', or 'auto'", l2.path(), l2.line());
      }
      if (rtype == Node::nil) rtype = l2.type();
      // accumulate the result
      Node lr(new_Node(rtype, l1.path(), l1.line(), size));
      if (l1.type() != Node::nil) lr += l1;
      if (l2.type() != Node::nil) lr += l2;
      return lr;
    }

    extern Signature append_sig = "append($list1, $list2, $separator: auto)";
    Node append(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node list(bindings[parameter_names[0].token()]);
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

      Node::Type sep_type;
      string sep_string = bindings[parameter_names[2].token()].token().unquote();
      if (sep_string == "comma")      sep_type = Node::comma_list;
      else if (sep_string == "space") sep_type = Node::space_list;
      else if (sep_string == "auto")  sep_type = list.type();
      else throw_eval_error("third argument to append must be 'space', 'comma', or 'auto'", list.path(), list.line());

      Node new_list(new_Node(sep_type, list.path(), list.line(), list.size() + 1));
      new_list += list;
      new_list << bindings[parameter_names[1].token()];
      return new_list;
    }

    extern Signature compact_sig = "compact($arg1: false, $arg2: false, $arg3: false, $arg4: false, $arg5: false, $arg6: false, $arg7: false, $arg8: false, $arg9: false, $arg10: false, $arg11: false, $arg12: false)";
    Node compact(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      size_t num_args     = bindings.current_frame.size();
      Node::Type sep_type = Node::comma_list;
      Node list;
      Node arg1(bindings[parameter_names[0].token()]);
      if (num_args == 1 && (arg1.type() == Node::space_list ||
                            arg1.type() == Node::comma_list ||
                            arg1.type() == Node::nil)) {
        list = new_Node(arg1.type(), arg1.path(), arg1.line(), arg1.size());
        list += arg1;
      }
      else {
        list = new_Node(sep_type, arg1.path(), arg1.line(), num_args);
        for (size_t i = 0; i < num_args; ++i) {
          list << bindings[parameter_names[i].token()];
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

    // Introspection Functions /////////////////////////////////////////////
    
    extern const char number_name[] = "number";
    extern const char string_name[] = "string";
    extern const char bool_name[]   = "bool";
    extern const char color_name[]  = "color";
    extern const char list_name[]   = "list";
    
    extern Signature type_of_sig = "type-of($value)";
    Node type_of(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node val(bindings[parameter_names[0].token()]);
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
    
    extern Signature unit_sig = "unit($number)";
    Node unit(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node val(bindings[parameter_names[0].token()]);
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

    extern Signature unitless_sig = "unitless($number)";
    Node unitless(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node val(bindings[parameter_names[0].token()]);
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
    
    extern Signature comparable_sig = "comparable($number-1, $number-2)";
    Node comparable(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node n1(bindings[parameter_names[0].token()]);
      Node n2(bindings[parameter_names[1].token()]);
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
    extern Signature not_sig = "not($value)";
    Node not_impl(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node val(bindings[parameter_names[0].token()]);
      if (val.type() == Node::boolean && val.boolean_value() == false) {
        return new_Node(Node::boolean, val.path(), val.line(), true);
      }
      else {
        return new_Node(Node::boolean, val.path(), val.line(), false);
      }
    }

    extern Signature if_sig = "if($condition, $if-true, $if-false)";
    Node if_impl(const Node parameter_names, Environment& bindings, Node_Factory& new_Node) {
      Node predicate(bindings[parameter_names[0].token()]);
      Node consequent(bindings[parameter_names[1].token()]);
      Node alternative(bindings[parameter_names[2].token()]);

      if (predicate.type() == Node::boolean && predicate.boolean_value() == false) return alternative;
      return consequent;
    }

  }
}
