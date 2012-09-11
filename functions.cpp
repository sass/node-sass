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
#include <sstream>
#include <cmath>
#include <algorithm>

using std::cerr; using std::endl; using std::stringstream;

namespace Sass {

  // this constructor needs context.hpp, so it can't be defined in functions.hpp
  // because including context.hpp in functions.hpp would be circular
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

    extern const char true_str[]  = "true";
    extern const char false_str[] = "false";

    extern const char empty_str[]   = "";
    extern const char percent_str[] = "%";
    extern const char deg_str[]     = "deg";

    extern const char numeric_name[] = "numeric value";
    extern const char number_name[] = "number";
    extern const char string_name[] = "string";
    extern const char bool_name[]   = "bool";
    extern const char color_name[]  = "color";
    extern const char list_name[]   = "list";

    static void throw_eval_error(string message, string& path, size_t line)
    {
      if (!path.empty() && Prelexer::string_constant(path.c_str()))
        path = path.substr(1, path.length() - 1);

      throw Error(Error::evaluation, path, line, message);
    }

    static const char* nameof(Node::Type t) {
      switch (t)
      {
        case Node::numeric: {
          return numeric_name;
        } break;

        case Node::number:
        case Node::numeric_percentage:
        case Node::numeric_dimension: {
          return number_name;
        } break;

        case Node::identifier:
        case Node::value_schema:
        case Node::identifier_schema:
        case Node::string_constant:
        case Node::string_schema:
        case Node::concatenation: {
          return string_name;
        } break;

        case Node::boolean: {
          return bool_name;
        } break;

        case Node::numeric_color: {
          return color_name;
        } break;

        case Node::space_list:
        case Node::comma_list: {
          return list_name;
        } break;

        default: {
          return empty_str;
        } break;
      }
      // unreachable statement
      return empty_str;
    }

    // Functions for fetching and checking arguments.
    static Node arg(Signature sig, string& path, size_t line, const Node parameter_names, Environment& bindings, size_t param_num, Node::Type param_type) {
      Node the_arg(bindings[parameter_names[param_num].token()]);
      Node::Type arg_type = the_arg.type();
      switch (param_type)
      {
        case Node::any: {
          return the_arg;
        } break;

        case Node::numeric: {
          if (the_arg.is_numeric()) return the_arg;
        } break;

        case Node::list: {
          if (arg_type == Node::space_list || arg_type == Node::comma_list) return the_arg;
        } break;

        default: {
          if (arg_type == param_type) return the_arg;
        } break;
      }
      stringstream msg;
      msg << nameof(param_type) << " required for argument " << param_num+1 << " in call to '" << sig << "'";
      throw_eval_error(msg.str(), path, line);
      // unreachable statement
      return Node();
    }

    static Node arg(Signature sig, string& path, size_t line, const Node parameter_names, Environment& bindings, size_t param_num, double low, double high) {
      Node the_arg(arg(sig, path, line, parameter_names, bindings, param_num, Node::numeric));
      double val = the_arg.numeric_value();
      if (val < low || high < val) {
        stringstream msg;
        msg << "argument " << param_num+1 << " must be between " << low << " and " << high << " in call to '" << sig << "'";
        throw_eval_error(msg.str(), path, line);
      }
      return the_arg;
    }

    ////////////////////////////////////////////////////////////////////////
    // RGB Functions ///////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////

    extern Signature rgb_sig = "rgb($red, $green, $blue)";
    Node rgb(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      double r = arg(rgb_sig, path, line, parameter_names, bindings, 0, 0, 255).numeric_value();
      double g = arg(rgb_sig, path, line, parameter_names, bindings, 1, 0, 255).numeric_value();
      double b = arg(rgb_sig, path, line, parameter_names, bindings, 2, 0, 255).numeric_value();
      return new_Node(path, line, std::floor(r), std::floor(g), std::floor(b), 1.0);
    }

    extern Signature rgba_4_sig = "rgba($red, $green, $blue, $alpha)";
    Node rgba_4(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      double r = arg(rgba_4_sig, path, line, parameter_names, bindings, 0, 0, 255).numeric_value();
      double g = arg(rgba_4_sig, path, line, parameter_names, bindings, 1, 0, 255).numeric_value();
      double b = arg(rgba_4_sig, path, line, parameter_names, bindings, 2, 0, 255).numeric_value();
      double a = arg(rgba_4_sig, path, line, parameter_names, bindings, 3, 0, 1).numeric_value();
      return new_Node(path, line, std::floor(r), std::floor(g), std::floor(b), a);
    }

    extern Signature rgba_2_sig = "rgba($color, $alpha)";
    Node rgba_2(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color_arg(arg(rgba_2_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node alpha_arg(arg(rgba_2_sig, path, line, parameter_names, bindings, 1, 0, 1));
      double r = color_arg[0].numeric_value();
      double g = color_arg[1].numeric_value();
      double b = color_arg[2].numeric_value();
      double a = alpha_arg.numeric_value();
      return new_Node(path, line, r, g, b, a);
    }
    
    extern Signature red_sig = "red($color)";
    Node red(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(arg(red_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      return new_Node(path, line, color[0]);
    }
    
    extern Signature green_sig = "green($color)";
    Node green(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(arg(green_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      return new_Node(path, line, color[1]);
    }
    
    extern Signature blue_sig = "blue($color)";
    Node blue(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(arg(blue_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      return new_Node(path, line, color[2]);
    }

    extern Signature mix_sig = "mix($color-1, $color-2, $weight: 50%)";
    Node mix(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color1(arg(mix_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node color2(arg(mix_sig, path, line, parameter_names, bindings, 1, Node::numeric_color));
      Node weight(arg(mix_sig, path, line, parameter_names, bindings, 2, 0, 100));

      double p = weight.numeric_value()/100;
      double w = 2*p - 1;
      double a = color1[3].numeric_value() - color2[3].numeric_value();

      double w1 = (((w * a == -1) ? w : (w + a)/(1 + w*a)) + 1)/2.0;
      double w2 = 1 - w1;

      Node mixed(new_Node(Node::numeric_color, path, line, 4));
      for (int i = 0; i < 3; ++i) {
        mixed << new_Node(path, line, std::floor(w1*color1[i].numeric_value() + w2*color2[i].numeric_value()));
      }
      double alpha = color1[3].numeric_value()*p + color2[3].numeric_value()*(1-p);
      mixed << new_Node(path, line, alpha);
      return mixed;
    }
 
    ////////////////////////////////////////////////////////////////////////
    // HSL Functions ///////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////

    // RGB to HSL helper function so we can do hsl operations.
    // (taken from http://www.easyrgb.com)
    Node rgb_to_hsl(double r, double g, double b, Node_Factory& new_Node, string& path, size_t line) {
      r /= 255.0; g /= 255.0; b /= 255.0;

      double max = std::max(r, std::max(g, b));
      double min = std::min(r, std::min(g, b));
      double del = max - min;

      double h = 0, s = 0, l = (max + min)/2;

      if (max == min) {
        h = s = 0; // achromatic
      }
      else {
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
      return new_Node(path, line, static_cast<int>(h*360)%360, s*100, l*100);
    }

    // Hue to RGB helper function
    double h_to_rgb(double m1, double m2, double h) {
      if (h < 0) h += 1;
      if (h > 1) h -= 1;
      if (h*6.0 < 1) return m1 + (m2 - m1)*h*6;
      if (h*2.0 < 1) return m2;
      if (h*3.0 < 2) return m1 + (m2 - m1) * (2.0/3.0 - h)*6;
      return m1;
    }

    Node hsla_impl(double h, double s, double l, double a, Node_Factory& new_Node, string& path, size_t line) {
      h = static_cast<double>(((static_cast<int>(h) % 360) + 360) % 360) / 360.0;
      s = (s < 0)   ? 0 :
          (s > 100) ? 100 :
          s;
      l = (l < 0)   ? 0 :
          (l > 100) ? 100 :
          l;
      s /= 100.0;
      l /= 100.0;

      double m2;
      if (l <= 0.5) m2 = l*(s+1.0);
      else m2 = l+s-l*s;
      double m1 = l*2-m2;
      // round the results -- consider moving this into the Node constructors
      double r = std::floor(h_to_rgb(m1, m2, h+1.0/3.0) * 255.0 + 0.5);
      double g = std::floor(h_to_rgb(m1, m2, h) * 255.0 + 0.5);
      double b = std::floor(h_to_rgb(m1, m2, h-1.0/3.0) * 255.0 + 0.5);

      return new_Node(path, line, r, g, b, a);
    }

    extern Signature hsl_sig = "hsl($hue, $saturation, $lightness)";
    Node hsl(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      double h = arg(hsl_sig, path, line, parameter_names, bindings, 0, Node::numeric).numeric_value();
      double s = arg(hsl_sig, path, line, parameter_names, bindings, 1, 0, 100).numeric_value();
      double l = arg(hsl_sig, path, line, parameter_names, bindings, 2, 0, 100).numeric_value();
      return hsla_impl(h, s, l, 1.0, new_Node, path, line);
    }

    extern Signature hsla_sig = "hsla($hue, $saturation, $lightness, $alpha)";
    Node hsla(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      double h = arg(hsla_sig, path, line, parameter_names, bindings, 0, Node::numeric).numeric_value();
      double s = arg(hsla_sig, path, line, parameter_names, bindings, 1, 0, 100).numeric_value();
      double l = arg(hsla_sig, path, line, parameter_names, bindings, 2, 0, 100).numeric_value();
      double a = arg(hsla_sig, path, line, parameter_names, bindings, 3, 0, 1).numeric_value();
      return hsla_impl(h, s, l, a, new_Node, path, line);
    }
    
    extern Signature hue_sig = "hue($color)";
    Node hue(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node rgb_color(arg(hue_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node hsl_color(rgb_to_hsl(rgb_color[0].numeric_value(),
                                rgb_color[1].numeric_value(),
                                rgb_color[2].numeric_value(),
                                new_Node, path, line));
      return new_Node(path, line, hsl_color[0].numeric_value(), Token::make(deg_str));
    }

    extern Signature saturation_sig = "saturation($color)";
    Node saturation(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node rgb_color(arg(saturation_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node hsl_color(rgb_to_hsl(rgb_color[0].numeric_value(),
                                rgb_color[1].numeric_value(),
                                rgb_color[2].numeric_value(),
                                new_Node, path, line));
      return new_Node(path, line, hsl_color[1].numeric_value(), Token::make(percent_str));
    }

    extern Signature lightness_sig = "lightness($color)";
    Node lightness(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node rgb_color(arg(lightness_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node hsl_color(rgb_to_hsl(rgb_color[0].numeric_value(),
                                rgb_color[1].numeric_value(),
                                rgb_color[2].numeric_value(),
                                new_Node, path, line));
      return new_Node(path, line, hsl_color[2].numeric_value(), Token::make(percent_str));
    }

    extern Signature adjust_hue_sig = "adjust-hue($color, $degrees)";
    Node adjust_hue(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node rgb_col(arg(adjust_hue_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node degrees(arg(adjust_hue_sig, path, line, parameter_names, bindings, 1, Node::numeric));
      Node hsl_col(rgb_to_hsl(rgb_col[0].numeric_value(),
                              rgb_col[1].numeric_value(),
                              rgb_col[2].numeric_value(),
                              new_Node, path, line));
      return hsla_impl(hsl_col[0].numeric_value() + degrees.numeric_value(),
                       hsl_col[1].numeric_value(),
                       hsl_col[2].numeric_value(),
                       rgb_col[3].numeric_value(),
                       new_Node, path, line);
    }

    extern Signature lighten_sig = "lighten($color, $amount)";
    Node lighten(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node rgb_col(arg(lighten_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node amount(arg(lighten_sig, path, line, parameter_names, bindings, 1, 0, 100));
      Node hsl_col(rgb_to_hsl(rgb_col[0].numeric_value(),
                              rgb_col[1].numeric_value(),
                              rgb_col[2].numeric_value(),
                              new_Node, path, line));
      return hsla_impl(hsl_col[0].numeric_value(),
                       hsl_col[1].numeric_value(),
                       hsl_col[2].numeric_value() + amount.numeric_value(),
                       rgb_col[3].numeric_value(),
                       new_Node, path, line);
    }

    extern Signature darken_sig = "darken($color, $amount)";
    Node darken(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node rgb_col(arg(darken_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node amount(arg(darken_sig, path, line, parameter_names, bindings, 1, 0, 100));
      Node hsl_col(rgb_to_hsl(rgb_col[0].numeric_value(),
                              rgb_col[1].numeric_value(),
                              rgb_col[2].numeric_value(),
                              new_Node, path, line));
      return hsla_impl(hsl_col[0].numeric_value(),
                       hsl_col[1].numeric_value(),
                       hsl_col[2].numeric_value() - amount.numeric_value(),
                       rgb_col[3].numeric_value(),
                       new_Node, path, line);
    }

    extern Signature saturate_sig = "saturate($color, $amount)";
    Node saturate(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node rgb_col(arg(saturate_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node amount(arg(saturate_sig, path, line, parameter_names, bindings, 1, 0, 100));
      Node hsl_col(rgb_to_hsl(rgb_col[0].numeric_value(),
                              rgb_col[1].numeric_value(),
                              rgb_col[2].numeric_value(),
                              new_Node, path, line));
      return hsla_impl(hsl_col[0].numeric_value(),
                       hsl_col[1].numeric_value() + amount.numeric_value(),
                       hsl_col[2].numeric_value(),
                       rgb_col[3].numeric_value(),
                       new_Node, path, line);
    }

    extern Signature desaturate_sig = "desaturate($color, $amount)";
    Node desaturate(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node rgb_col(arg(desaturate_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node amount(arg(desaturate_sig, path, line, parameter_names, bindings, 1, 0, 100));
      Node hsl_col(rgb_to_hsl(rgb_col[0].numeric_value(),
                              rgb_col[1].numeric_value(),
                              rgb_col[2].numeric_value(),
                              new_Node, path, line));
      return hsla_impl(hsl_col[0].numeric_value(),
                       hsl_col[1].numeric_value() - amount.numeric_value(),
                       hsl_col[2].numeric_value(),
                       rgb_col[3].numeric_value(),
                       new_Node, path, line);
    }

    extern Signature grayscale_sig = "grayscale($color)";
    Node grayscale(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(arg(grayscale_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node hsl_color(rgb_to_hsl(color[0].numeric_value(),
                                color[1].numeric_value(),
                                color[2].numeric_value(),
                                new_Node, path, line));
      return hsla_impl(hsl_color[0].numeric_value(),
                       0.0, // desaturate completely
                       hsl_color[2].numeric_value(),
                       color[3].numeric_value(),
                       new_Node, path, line);
    }

    extern Signature complement_sig = "complement($color)";
    Node complement(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(arg(complement_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      Node hsl_color(rgb_to_hsl(color[0].numeric_value(),
                                color[1].numeric_value(),
                                color[2].numeric_value(),
                                new_Node, path, line));
      return hsla_impl(hsl_color[0].numeric_value() - 180, // other side of the color wheel
                       hsl_color[1].numeric_value(),
                       hsl_color[2].numeric_value(),
                       color[3].numeric_value(),
                       new_Node, path, line);
    }

    extern Signature invert_sig = "invert($color)";
    Node invert(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(arg(invert_sig, path, line, parameter_names, bindings, 0, Node::numeric_color));
      return new_Node(path, line,
                      255 - color[0].numeric_value(),
                      255 - color[1].numeric_value(),
                      255 - color[2].numeric_value(),
                      color[3].numeric_value());
    }
    
    ////////////////////////////////////////////////////////////////////////
    // Opacity Functions ///////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    
    extern Signature alpha_sig = "alpha($color)";
    Node alpha(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(bindings[parameter_names[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to 'alpha' must be a color", color.path(), color.line());
      return color[3];
    }

    extern Signature opacity_sig = "opacity($color)";
    Node opacity(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(bindings[parameter_names[0].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("argument to 'opacity' must be a color", color.path(), color.line());
      return color[3];
    }
    
    extern Signature opacify_sig = "opacify($color, $amount)";
    Node opacify(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(bindings[parameter_names[0].token()]);
      Node delta(bindings[parameter_names[1].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("first argument to 'opacify' must be a color", color.path(), color.line());
      if (!delta.is_numeric()) throw_eval_error("second argument to 'opacify' must be numeric", delta.path(), delta.line());
      double dd = delta.numeric_value();
      if (dd < 0 || 1 < dd) throw_eval_error("amount must be between 0 and 1 for 'opacify'", delta.path(), delta.line());
      dd += color[3].numeric_value();
      if (dd > 1) dd = 1;
      return new_Node(path, line,
                      color[0].numeric_value(),
                      color[1].numeric_value(),
                      color[2].numeric_value(),
                      dd);
    }

    extern Signature fade_in_sig = "fade-in($color, $amount)";
    Node fade_in(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(bindings[parameter_names[0].token()]);
      Node delta(bindings[parameter_names[1].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("first argument to 'fade-in' must be a color", color.path(), color.line());
      if (!delta.is_numeric()) throw_eval_error("second argument to 'fade-in' must be numeric", delta.path(), delta.line());
      double dd = delta.numeric_value();
      if (dd < 0 || 1 < dd) throw_eval_error("amount must be between 0 and 1 for 'fade-in'", delta.path(), delta.line());
      dd += color[3].numeric_value();
      if (dd > 1) dd = 1;
      return new_Node(path, line,
                      color[0].numeric_value(),
                      color[1].numeric_value(),
                      color[2].numeric_value(),
                      dd);
    }
    
    extern Signature transparentize_sig = "transparentize($color, $amount)";
    Node transparentize(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(bindings[parameter_names[0].token()]);
      Node delta(bindings[parameter_names[1].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("first argument to 'transparentize' must be a color", color.path(), color.line());
      if (!delta.is_numeric()) throw_eval_error("second argument to 'transparentize' must be numeric", delta.path(), delta.line());
      double dd = delta.numeric_value();
      if (dd < 0 || 1 < dd) throw_eval_error("amount must be between 0 and 1 for 'transparentize'", delta.path(), delta.line());
      double alpha = color[3].numeric_value() - dd;
      if (alpha < 0) alpha = 0;
      return new_Node(path, line,
                      color[0].numeric_value(),
                      color[1].numeric_value(),
                      color[2].numeric_value(),
                      alpha);
    }

    extern Signature fade_out_sig = "fade-out($color, $amount)";
    Node fade_out(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node color(bindings[parameter_names[0].token()]);
      Node delta(bindings[parameter_names[1].token()]);
      if (color.type() != Node::numeric_color) throw_eval_error("first argument to 'fade-out' must be a color", color.path(), color.line());
      if (!delta.is_numeric()) throw_eval_error("second argument to 'fade-out' must be numeric", delta.path(), delta.line());
      double dd = delta.numeric_value();
      if (dd < 0 || 1 < dd) throw_eval_error("amount must be between 0 and 1 for 'fade-out'", delta.path(), delta.line());
      double alpha = color[3].numeric_value() - dd;
      if (alpha < 0) alpha = 0;
      return new_Node(path, line,
                      color[0].numeric_value(),
                      color[1].numeric_value(),
                      color[2].numeric_value(),
                      alpha);
    }

    ////////////////////////////////////////////////////////////////////////
    // Other Color Functions ///////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
      
    extern Signature adjust_color_sig = "adjust-color($color, $red: false, $green: false, $blue: false, $hue: false, $saturation: false, $lightness: false, $alpha: false)";
    Node adjust_color(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
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

      if (color.type() != Node::numeric_color) {
        throw_eval_error("first argument to 'adjust-color' must be a color", color.path(), color.line());
      }
      else if (!no_rgb && !no_hsl) {
        throw_eval_error("cannot specify RGB and HSL values for a color at the same time for 'adjust-color'", r.path(), r.line());
      }
      else if (!no_rgb) {
        if (!r.is_false() && !r.is_numeric()) throw_eval_error("argument $red of 'adjust-color' must be numeric", r.path(), r.line());
        if (!g.is_false() && !g.is_numeric()) throw_eval_error("argument $green of 'adjust-color' must be numeric", g.path(), g.line());
        if (!b.is_false() && !b.is_numeric()) throw_eval_error("argument $blue of 'adjust-color' must be numeric", b.path(), b.line());
        if (!a.is_false() && !a.is_numeric()) throw_eval_error("argument $alpha of 'adjust-color' must be numeric", a.path(), a.line());
        double new_r = color[0].numeric_value() + (r.is_false() ? 0 : r.numeric_value());
        double new_g = color[1].numeric_value() + (g.is_false() ? 0 : g.numeric_value());
        double new_b = color[2].numeric_value() + (b.is_false() ? 0 : b.numeric_value());
        double new_a = color[3].numeric_value() + (a.is_false() ? 0 : a.numeric_value());
        return new_Node(path, line, new_r, new_g, new_b, new_a);
      }
      else if (!no_hsl) {
        Node hsl_node(rgb_to_hsl(color[0].numeric_value(),
                                 color[1].numeric_value(),
                                 color[2].numeric_value(),
                                 new_Node, path, line));
        if (!h.is_false() && !h.is_numeric()) throw_eval_error("argument $hue of 'adjust-color' must be numeric", h.path(), h.line());
        if (!s.is_false() && !s.is_numeric()) throw_eval_error("argument $saturation of 'adjust-color' must be numeric", s.path(), s.line());
        if (!l.is_false() && !l.is_numeric()) throw_eval_error("argument $lightness of 'adjust-color' must be numeric", l.path(), l.line());
        if (!a.is_false() && !a.is_numeric()) throw_eval_error("argument $alpha of 'adjust-color' must be numeric", a.path(), a.line());
        double new_h = (h.is_false() ? 0 : h.numeric_value()) + hsl_node[0].numeric_value();
        double new_s = (s.is_false() ? 0 : s.numeric_value()) + hsl_node[1].numeric_value();
        double new_l = (l.is_false() ? 0 : l.numeric_value()) + hsl_node[2].numeric_value();
        double new_a = (a.is_false() ? 0 : a.numeric_value()) + color[3].numeric_value();
        return hsla_impl(new_h, new_s, new_l, new_a, new_Node, path, line);
      }
      else if (!a.is_false()) {
        if (!a.is_numeric()) throw_eval_error("argument $alpha of 'adjust-color' must be numeric", a.path(), a.line());
        return new_Node(path, line,
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
    Node change_color(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
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

      if (color.type() != Node::numeric_color) {
        throw_eval_error("first argument to 'change-color' must be a color", color.path(), color.line());
      }
      if (!no_rgb && !no_hsl) {
        throw_eval_error("cannot specify RGB and HSL values for a color at the same time for 'change-color'", r.path(), r.line());
      }
      else if (!no_rgb) {
        if (!r.is_false() && !r.is_numeric()) throw_eval_error("argument $red of 'change-color' must be numeric", r.path(), r.line());
        if (!g.is_false() && !g.is_numeric()) throw_eval_error("argument $green of 'change-color' must be numeric", g.path(), g.line());
        if (!b.is_false() && !b.is_numeric()) throw_eval_error("argument $blue of 'change-color' must be numeric", b.path(), b.line());
        if (!a.is_false() && !a.is_numeric()) throw_eval_error("argument $alpha of 'change-color' must be numeric", a.path(), a.line());
        double new_r = (r.is_false() ? color[0] : r).numeric_value();
        double new_g = (g.is_false() ? color[1] : g).numeric_value();
        double new_b = (b.is_false() ? color[2] : b).numeric_value();
        double new_a = (a.is_false() ? color[3] : a).numeric_value();
        return new_Node(path, line, new_r, new_g, new_b, new_a);
      }
      else if (!no_hsl) {
        Node hsl_node(rgb_to_hsl(color[0].numeric_value(),
                                 color[1].numeric_value(),
                                 color[2].numeric_value(),
                                 new_Node, path, line));
        if (!h.is_false() && !h.is_numeric()) throw_eval_error("argument $hue of 'change-color' must be numeric", h.path(), h.line());
        if (!s.is_false() && !s.is_numeric()) throw_eval_error("argument $saturation of 'change-color' must be numeric", s.path(), s.line());
        if (!l.is_false() && !l.is_numeric()) throw_eval_error("argument $lightness of 'change-color' must be numeric", l.path(), l.line());
        if (!a.is_false() && !a.is_numeric()) throw_eval_error("argument $alpha of 'change-color' must be numeric", a.path(), a.line());
        double new_h = (h.is_false() ? hsl_node[0].numeric_value() : h.numeric_value());
        double new_s = (s.is_false() ? hsl_node[1].numeric_value() : s.numeric_value());
        double new_l = (l.is_false() ? hsl_node[2].numeric_value() : l.numeric_value());
        double new_a = (a.is_false() ? color[3].numeric_value() : a.numeric_value());
        return hsla_impl(new_h, new_s, new_l, new_a, new_Node, path, line);
      }
      else if (!a.is_false()) {
        if (!a.is_numeric()) throw_eval_error("argument $alpha of 'change-color' must be numeric", a.path(), a.line());
        return new_Node(path, line,
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

    ////////////////////////////////////////////////////////////////////////
    // String Functions ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    
    extern Signature unquote_sig = "unquote($string)";
    Node unquote(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node cpy(new_Node(bindings[parameter_names[0].token()]));
      // if (cpy.type() != Node::string_constant /* && cpy.type() != Node::concatenation */) {
      //   throw_eval_error("argument to unquote must be a string", cpy.path(), cpy.line());
      // }
      cpy.is_unquoted() = true;
      cpy.is_quoted() = false;
      return cpy;
    }
    
    extern Signature quote_sig = "quote($string)";
    Node quote(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
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
    
    ////////////////////////////////////////////////////////////////////////
    // Number Functions ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    
    extern Signature percentage_sig = "percentage($value)";
    Node percentage(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node orig(bindings[parameter_names[0].token()]);
      if (orig.type() != Node::number) {
        throw_eval_error("argument to percentage must be a unitless number", orig.path(), orig.line());
      }
      return new_Node(path, line, orig.numeric_value() * 100, Node::numeric_percentage);
    }

    extern Signature round_sig = "round($value)";
    Node round(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node orig(bindings[parameter_names[0].token()]);
      switch (orig.type())
      {
        case Node::numeric_dimension: {
          return new_Node(path, line,
                          std::floor(orig.numeric_value() + 0.5), orig.unit());
        } break;

        case Node::number: {
          return new_Node(path, line,
                          std::floor(orig.numeric_value() + 0.5));
        } break;

        case Node::numeric_percentage: {
          return new_Node(path, line,
                          std::floor(orig.numeric_value() + 0.5),
                          Node::numeric_percentage);
        } break;

        default: {
          throw_eval_error("argument to round must be numeric", path, line);
        } break;
      }
      // unreachable statement
      return Node();
    }

    extern Signature ceil_sig = "ceil($value)";
    Node ceil(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node orig(bindings[parameter_names[0].token()]);
      switch (orig.type())
      {
        case Node::numeric_dimension: {
          return new_Node(path, line,
                          std::ceil(orig.numeric_value()), orig.unit());
        } break;

        case Node::number: {
          return new_Node(path, line,
                          std::ceil(orig.numeric_value()));
        } break;

        case Node::numeric_percentage: {
          return new_Node(path, line,
                          std::ceil(orig.numeric_value()),
                          Node::numeric_percentage);
        } break;

        default: {
          throw_eval_error("argument to ceil must be numeric", path, line);
        } break;
      }
      // unreachable statement
      return Node();
    }

    extern Signature floor_sig = "floor($value)";
    Node floor(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node orig(bindings[parameter_names[0].token()]);
      switch (orig.type())
      {
        case Node::numeric_dimension: {
          return new_Node(path, line,
                          std::floor(orig.numeric_value()), orig.unit());
        } break;

        case Node::number: {
          return new_Node(path, line,
                          std::floor(orig.numeric_value()));
        } break;

        case Node::numeric_percentage: {
          return new_Node(path, line,
                          std::floor(orig.numeric_value()),
                          Node::numeric_percentage);
        } break;

        default: {
          throw_eval_error("argument to floor must be numeric", path, line);
        } break;
      }
      // unreachable statement
      return Node();
    }

    extern Signature abs_sig = "abs($value)";
    Node abs(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node orig(bindings[parameter_names[0].token()]);
      switch (orig.type())
      {
        case Node::numeric_dimension: {
          return new_Node(path, line,
                          std::abs(orig.numeric_value()), orig.unit());
        } break;

        case Node::number: {
          return new_Node(path, line,
                          std::abs(orig.numeric_value()));
        } break;

        case Node::numeric_percentage: {
          return new_Node(path, line,
                          std::abs(orig.numeric_value()),
                          Node::numeric_percentage);
        } break;

        default: {
          throw_eval_error("argument to abs must be numeric", path, line);
        } break;
      }
      // unreachable statement
      return Node();
    }
    
    ////////////////////////////////////////////////////////////////////////
    // List Functions //////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////

    extern Signature length_sig = "length($list)";
    Node length(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node arg(bindings[parameter_names[0].token()]);
      switch (arg.type())
      {
        case Node::space_list:
        case Node::comma_list: {
          return new_Node(path, line, arg.size());
        } break;

        case Node::nil: {
          return new_Node(path, line, 0);
        } break;

        default: {
          // single objects should be reported as lists of length 1
          return new_Node(path, line, 1);
        } break;
      }
      // unreachable statement
      return Node();
    }
    
    extern Signature nth_sig = "nth($list, $n)";
    Node nth(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node l(bindings[parameter_names[0].token()]);
      Node n(bindings[parameter_names[1].token()]);
      if (n.type() != Node::number) {
        throw_eval_error("second argument to nth must be a number", path, line);
      }
      if (l.type() == Node::nil) {
        throw_eval_error("cannot index into an empty list", path, line);
      }
      // wrap the first arg if it isn't a list
      if (l.type() != Node::space_list && l.type() != Node::comma_list) {
        l = new_Node(Node::space_list, path, line, 1) << l;
      }
      double n_prim = n.numeric_value();
      if (n_prim < 1 || n_prim > l.size()) {
        throw_eval_error("out of range index for nth", path, line);
      }
      return l[n_prim - 1];
    }

    extern Signature join_sig = "join($list1, $list2, $separator: auto)";
    Node join(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      // if the args aren't lists, turn them into singleton lists
      Node l1(bindings[parameter_names[0].token()]);
      if (l1.type() != Node::space_list && l1.type() != Node::comma_list && l1.type() != Node::nil) {
        l1 = new_Node(Node::space_list, path, line, 1) << l1;
      }
      Node l2(bindings[parameter_names[1].token()]);
      if (l2.type() != Node::space_list && l2.type() != Node::comma_list && l2.type() != Node::nil) {
        l2 = new_Node(Node::space_list, l2.path(), l2.line(), 1) << l2;
      }
      // nil + nil = nil
      if (l1.type() == Node::nil && l2.type() == Node::nil) {
        return new_Node(Node::nil, path, line, 0);
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
        throw_eval_error("third argument to join must be 'space', 'comma', or 'auto'", path, line);
      }
      if (rtype == Node::nil) rtype = l2.type();
      // accumulate the result
      Node lr(new_Node(rtype, path, line, size));
      if (l1.type() != Node::nil) lr += l1;
      if (l2.type() != Node::nil) lr += l2;
      return lr;
    }

    extern Signature append_sig = "append($list1, $list2, $separator: auto)";
    Node append(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
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
          list = (new_Node(Node::space_list, path, line, 1) << list);
        } break;
      }

      Node::Type sep_type;
      string sep_string = bindings[parameter_names[2].token()].token().unquote();
      if (sep_string == "comma")      sep_type = Node::comma_list;
      else if (sep_string == "space") sep_type = Node::space_list;
      else if (sep_string == "auto")  sep_type = list.type();
      else throw_eval_error("third argument to append must be 'space', 'comma', or 'auto'", path, line);

      Node new_list(new_Node(sep_type, path, line, list.size() + 1));
      new_list += list;
      new_list << bindings[parameter_names[1].token()];
      return new_list;
    }

    extern Signature compact_sig = "compact($arg1: false, $arg2: false, $arg3: false, $arg4: false, $arg5: false, $arg6: false, $arg7: false, $arg8: false, $arg9: false, $arg10: false, $arg11: false, $arg12: false)";
    Node compact(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      size_t num_args     = bindings.current_frame.size();
      Node::Type sep_type = Node::comma_list;
      Node list;
      Node arg1(bindings[parameter_names[0].token()]);
      if (num_args == 1 && (arg1.type() == Node::space_list ||
                            arg1.type() == Node::comma_list ||
                            arg1.type() == Node::nil)) {
        list = new_Node(arg1.type(), path, line, arg1.size());
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
      return new_list.size() ? new_list : new_Node(Node::nil, path, line, 0);
    }

    ////////////////////////////////////////////////////////////////////////
    // Introspection Functions /////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    
    extern Signature type_of_sig = "type-of($value)";
    Node type_of(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
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
      Node type(new_Node(Node::string_constant, path, line, type_name));
      type.is_unquoted() = true;
      return type;
    }

    extern Signature unit_sig = "unit($number)";
    Node unit(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node val(bindings[parameter_names[0].token()]);
      switch (val.type())
      {
        case Node::number: {
          return new_Node(Node::string_constant, path, line, Token::make(empty_str));
        } break;

        case Node::numeric_dimension:
        case Node::numeric_percentage: {
          return new_Node(Node::string_constant, path, line, val.unit());
        } break;

        default: {
          throw_eval_error("argument to unit must be numeric", path, line);
        } break;
      }
      // unreachable statement
      return Node();
    }

    extern Signature unitless_sig = "unitless($number)";
    Node unitless(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node val(bindings[parameter_names[0].token()]);
      switch (val.type())
      {
        case Node::number: {
          return new_Node(Node::boolean, path, line, true);
        } break;

        case Node::numeric_percentage:
        case Node::numeric_dimension: {
          return new_Node(Node::boolean, path, line, false);
        } break;

        default: {
          throw_eval_error("argument to unitless must be numeric", path, line);
        } break;
      }
      // unreachable statement
      return Node();
    }
    
    extern Signature comparable_sig = "comparable($number-1, $number-2)";
    Node comparable(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node n1(bindings[parameter_names[0].token()]);
      Node n2(bindings[parameter_names[1].token()]);
      Node::Type t1 = n1.type();
      Node::Type t2 = n2.type();
      if ((t1 == Node::number && n2.is_numeric()) ||
          (n1.is_numeric() && t2 == Node::number)) {
        return new_Node(Node::boolean, path, line, true);
      }
      else if (t1 == Node::numeric_percentage && t2 == Node::numeric_percentage) {
        return new_Node(Node::boolean, path, line, true);
      }
      else if (t1 == Node::numeric_dimension && t2 == Node::numeric_dimension) {
        string u1(n1.unit().to_string());
        string u2(n2.unit().to_string());
        if ((u1 == "ex" && u2 == "ex") ||
            (u1 == "em" && u2 == "em") ||
            ((u1 == "in" || u1 == "cm" || u1 == "mm" || u1 == "pt" || u1 == "pc") &&
             (u2 == "in" || u2 == "cm" || u2 == "mm" || u2 == "pt" || u2 == "pc"))) {
          return new_Node(Node::boolean, path, line, true);
        }
        else {
          return new_Node(Node::boolean, path, line, false);
        }
      }
      else if (!n1.is_numeric() && !n2.is_numeric()) {
        throw_eval_error("arguments to comparable must be numeric", path, line);
      }
      // default to false if we missed anything
      return new_Node(Node::boolean, path, line, false);
    }
    
    ////////////////////////////////////////////////////////////////////////
    // Boolean Functions ///////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////

    extern Signature not_sig = "not($value)";
    Node not_impl(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node val(bindings[parameter_names[0].token()]);
      if (val.type() == Node::boolean && val.boolean_value() == false) {
        return new_Node(Node::boolean, path, line, true);
      }
      else {
        return new_Node(Node::boolean, path, line, false);
      }
    }

    extern Signature if_sig = "if($condition, $if-true, $if-false)";
    Node if_impl(const Node parameter_names, Environment& bindings, Node_Factory& new_Node, string& path, size_t line) {
      Node predicate(bindings[parameter_names[0].token()]);
      Node consequent(bindings[parameter_names[1].token()]);
      Node alternative(bindings[parameter_names[2].token()]);

      if (predicate.type() == Node::boolean && predicate.boolean_value() == false) return alternative;
      return consequent;
    }

  }
}
