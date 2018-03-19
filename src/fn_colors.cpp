#include <cctype>
#include <iomanip>
#include "ast.hpp"
#include "fn_utils.hpp"
#include "fn_colors.hpp"

namespace Sass {

  namespace Functions {

    bool special_number(String_Constant_Ptr s) {
      if (s) {
        std::string calc("calc(");
        std::string var("var(");
        std::string ss(s->value());
        return std::equal(calc.begin(), calc.end(), ss.begin()) ||
               std::equal(var.begin(), var.end(), ss.begin());
      }
      return false;
    }

    Signature rgb_sig = "rgb($red, $green, $blue)";
    BUILT_IN(rgb)
    {
      if (
        special_number(Cast<String_Constant>(env["$red"])) ||
        special_number(Cast<String_Constant>(env["$green"])) ||
        special_number(Cast<String_Constant>(env["$blue"]))
      ) {
        return SASS_MEMORY_NEW(String_Constant, pstate, "rgb("
                                                        + env["$red"]->to_string()
                                                        + ", "
                                                        + env["$green"]->to_string()
                                                        + ", "
                                                        + env["$blue"]->to_string()
                                                        + ")"
        );
      }

      return SASS_MEMORY_NEW(Color,
                             pstate,
                             COLOR_NUM("$red"),
                             COLOR_NUM("$green"),
                             COLOR_NUM("$blue"));
    }

    Signature rgba_4_sig = "rgba($red, $green, $blue, $alpha)";
    BUILT_IN(rgba_4)
    {
      if (
        special_number(Cast<String_Constant>(env["$red"])) ||
        special_number(Cast<String_Constant>(env["$green"])) ||
        special_number(Cast<String_Constant>(env["$blue"])) ||
        special_number(Cast<String_Constant>(env["$alpha"]))
      ) {
        return SASS_MEMORY_NEW(String_Constant, pstate, "rgba("
                                                        + env["$red"]->to_string()
                                                        + ", "
                                                        + env["$green"]->to_string()
                                                        + ", "
                                                        + env["$blue"]->to_string()
                                                        + ", "
                                                        + env["$alpha"]->to_string()
                                                        + ")"
        );
      }

      return SASS_MEMORY_NEW(Color,
                             pstate,
                             COLOR_NUM("$red"),
                             COLOR_NUM("$green"),
                             COLOR_NUM("$blue"),
                             ALPHA_NUM("$alpha"));
    }

    Signature rgba_2_sig = "rgba($color, $alpha)";
    BUILT_IN(rgba_2)
    {
      if (
        special_number(Cast<String_Constant>(env["$color"]))
      ) {
        return SASS_MEMORY_NEW(String_Constant, pstate, "rgba("
                                                        + env["$color"]->to_string()
                                                        + ", "
                                                        + env["$alpha"]->to_string()
                                                        + ")"
        );
      }

      Color_Ptr c_arg = ARG("$color", Color);

      if (
        special_number(Cast<String_Constant>(env["$alpha"]))
      ) {
        std::stringstream strm;
        strm << "rgba("
                 << (int)c_arg->r() << ", "
                 << (int)c_arg->g() << ", "
                 << (int)c_arg->b() << ", "
                 << env["$alpha"]->to_string()
             << ")";
        return SASS_MEMORY_NEW(String_Constant, pstate, strm.str());
      }

      Color_Ptr new_c = SASS_MEMORY_COPY(c_arg);
      new_c->a(ALPHA_NUM("$alpha"));
      new_c->disp("");
      return new_c;
    }

    ////////////////
    // RGB FUNCTIONS
    ////////////////

    Signature red_sig = "red($color)";
    BUILT_IN(red)
    { return SASS_MEMORY_NEW(Number, pstate, ARG("$color", Color)->r()); }

    Signature green_sig = "green($color)";
    BUILT_IN(green)
    { return SASS_MEMORY_NEW(Number, pstate, ARG("$color", Color)->g()); }

    Signature blue_sig = "blue($color)";
    BUILT_IN(blue)
    { return SASS_MEMORY_NEW(Number, pstate, ARG("$color", Color)->b()); }

    Color* colormix(Context& ctx, ParserState& pstate, Color* color1, Color* color2, double weight) {
      double p = weight/100;
      double w = 2*p - 1;
      double a = color1->a() - color2->a();

      double w1 = (((w * a == -1) ? w : (w + a)/(1 + w*a)) + 1)/2.0;
      double w2 = 1 - w1;

      return SASS_MEMORY_NEW(Color,
                             pstate,
                             Sass::round(w1*color1->r() + w2*color2->r(), ctx.c_options.precision),
                             Sass::round(w1*color1->g() + w2*color2->g(), ctx.c_options.precision),
                             Sass::round(w1*color1->b() + w2*color2->b(), ctx.c_options.precision),
                             color1->a()*p + color2->a()*(1-p));
    }

    Signature mix_sig = "mix($color-1, $color-2, $weight: 50%)";
    BUILT_IN(mix)
    {
      Color_Obj  color1 = ARG("$color-1", Color);
      Color_Obj  color2 = ARG("$color-2", Color);
      double weight = DARG_U_PRCT("$weight");
      return colormix(ctx, pstate, color1, color2, weight);

    }

    ////////////////
    // HSL FUNCTIONS
    ////////////////

    // RGB to HSL helper function
    struct HSL { double h; double s; double l; };
    HSL rgb_to_hsl(double r, double g, double b)
    {

      // Algorithm from http://en.wikipedia.org/wiki/wHSL_and_HSV#Conversion_from_RGB_to_HSL_or_HSV
      r /= 255.0; g /= 255.0; b /= 255.0;

      double max = std::max(r, std::max(g, b));
      double min = std::min(r, std::min(g, b));
      double delta = max - min;

      double h = 0;
      double s;
      double l = (max + min) / 2.0;

      if (NEAR_EQUAL(max, min)) {
        h = s = 0; // achromatic
      }
      else {
        if (l < 0.5) s = delta / (max + min);
        else         s = delta / (2.0 - max - min);

        if      (r == max) h = (g - b) / delta + (g < b ? 6 : 0);
        else if (g == max) h = (b - r) / delta + 2;
        else if (b == max) h = (r - g) / delta + 4;
      }

      HSL hsl_struct;
      hsl_struct.h = h / 6 * 360;
      hsl_struct.s = s * 100;
      hsl_struct.l = l * 100;

      return hsl_struct;
    }

    // hue to RGB helper function
    double h_to_rgb(double m1, double m2, double h) {
      while (h < 0) h += 1;
      while (h > 1) h -= 1;
      if (h*6.0 < 1) return m1 + (m2 - m1)*h*6;
      if (h*2.0 < 1) return m2;
      if (h*3.0 < 2) return m1 + (m2 - m1) * (2.0/3.0 - h)*6;
      return m1;
    }

    Color_Ptr hsla_impl(double h, double s, double l, double a, Context& ctx, ParserState pstate)
    {
      h /= 360.0;
      s /= 100.0;
      l /= 100.0;

      if (l < 0) l = 0;
      if (s < 0) s = 0;
      if (l > 1) l = 1;
      if (s > 1) s = 1;
      while (h < 0) h += 1;
      while (h > 1) h -= 1;

      // if saturation is exacly zero, we loose
      // information for hue, since it will evaluate
      // to zero if converted back from rgb. Setting
      // saturation to a very tiny number solves this.
      if (s == 0) s = 1e-10;

      // Algorithm from the CSS3 spec: http://www.w3.org/TR/css3-color/#hsl-color.
      double m2;
      if (l <= 0.5) m2 = l*(s+1.0);
      else m2 = (l+s)-(l*s);
      double m1 = (l*2.0)-m2;
      // round the results -- consider moving this into the Color constructor
      double r = (h_to_rgb(m1, m2, h + 1.0/3.0) * 255.0);
      double g = (h_to_rgb(m1, m2, h) * 255.0);
      double b = (h_to_rgb(m1, m2, h - 1.0/3.0) * 255.0);

      return SASS_MEMORY_NEW(Color, pstate, r, g, b, a);
    }

    Signature hsl_sig = "hsl($hue, $saturation, $lightness)";
    BUILT_IN(hsl)
    {
      if (
        special_number(Cast<String_Constant>(env["$hue"])) ||
        special_number(Cast<String_Constant>(env["$saturation"])) ||
        special_number(Cast<String_Constant>(env["$lightness"]))
      ) {
        return SASS_MEMORY_NEW(String_Constant, pstate, "hsl("
                                                        + env["$hue"]->to_string()
                                                        + ", "
                                                        + env["$saturation"]->to_string()
                                                        + ", "
                                                        + env["$lightness"]->to_string()
                                                        + ")"
        );
      }

      return hsla_impl(ARGVAL("$hue"),
                       ARGVAL("$saturation"),
                       ARGVAL("$lightness"),
                       1.0,
                       ctx,
                       pstate);
    }

    Signature hsla_sig = "hsla($hue, $saturation, $lightness, $alpha)";
    BUILT_IN(hsla)
    {
      if (
        special_number(Cast<String_Constant>(env["$hue"])) ||
        special_number(Cast<String_Constant>(env["$saturation"])) ||
        special_number(Cast<String_Constant>(env["$lightness"])) ||
        special_number(Cast<String_Constant>(env["$alpha"]))
      ) {
        return SASS_MEMORY_NEW(String_Constant, pstate, "hsla("
                                                        + env["$hue"]->to_string()
                                                        + ", "
                                                        + env["$saturation"]->to_string()
                                                        + ", "
                                                        + env["$lightness"]->to_string()
                                                        + ", "
                                                        + env["$alpha"]->to_string()
                                                        + ")"
        );
      }

      return hsla_impl(ARGVAL("$hue"),
                       ARGVAL("$saturation"),
                       ARGVAL("$lightness"),
                       ARGVAL("$alpha"),
                       ctx,
                       pstate);
    }

    Signature hue_sig = "hue($color)";
    BUILT_IN(hue)
    {
      Color_Ptr rgb_color = ARG("$color", Color);
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());
      return SASS_MEMORY_NEW(Number, pstate, hsl_color.h, "deg");
    }

    Signature saturation_sig = "saturation($color)";
    BUILT_IN(saturation)
    {
      Color_Ptr rgb_color = ARG("$color", Color);
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());
      return SASS_MEMORY_NEW(Number, pstate, hsl_color.s, "%");
    }

    Signature lightness_sig = "lightness($color)";
    BUILT_IN(lightness)
    {
      Color_Ptr rgb_color = ARG("$color", Color);
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());
      return SASS_MEMORY_NEW(Number, pstate, hsl_color.l, "%");
    }

    Signature adjust_hue_sig = "adjust-hue($color, $degrees)";
    BUILT_IN(adjust_hue)
    {
      Color_Ptr rgb_color = ARG("$color", Color);
      double degrees = ARGVAL("$degrees");
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());
      return hsla_impl(hsl_color.h + degrees,
                       hsl_color.s,
                       hsl_color.l,
                       rgb_color->a(),
                       ctx,
                       pstate);
    }

    Signature lighten_sig = "lighten($color, $amount)";
    BUILT_IN(lighten)
    {
      Color_Ptr rgb_color = ARG("$color", Color);
      double amount = DARG_U_PRCT("$amount");
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());
      //Check lightness is not negative before lighten it
      double hslcolorL = hsl_color.l;
      if (hslcolorL < 0) {
        hslcolorL = 0;
      }

      return hsla_impl(hsl_color.h,
                       hsl_color.s,
                       hslcolorL + amount,
                       rgb_color->a(),
                       ctx,
                       pstate);
    }

    Signature darken_sig = "darken($color, $amount)";
    BUILT_IN(darken)
    {
      Color_Ptr rgb_color = ARG("$color", Color);
      double amount = DARG_U_PRCT("$amount");
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());

      //Check lightness if not over 100, before darken it
      double hslcolorL = hsl_color.l;
      if (hslcolorL > 100) {
        hslcolorL = 100;
      }

      return hsla_impl(hsl_color.h,
                       hsl_color.s,
                       hslcolorL - amount,
                       rgb_color->a(),
                       ctx,
                       pstate);
    }

    Signature saturate_sig = "saturate($color, $amount: false)";
    BUILT_IN(saturate)
    {
      // CSS3 filter function overload: pass literal through directly
      if (!Cast<Number>(env["$amount"])) {
        return SASS_MEMORY_NEW(String_Quoted, pstate, "saturate(" + env["$color"]->to_string(ctx.c_options) + ")");
      }

      double amount = DARG_U_PRCT("$amount");
      Color_Ptr rgb_color = ARG("$color", Color);
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());

      double hslcolorS = hsl_color.s + amount;

      // Saturation cannot be below 0 or above 100
      if (hslcolorS < 0) {
        hslcolorS = 0;
      }
      if (hslcolorS > 100) {
        hslcolorS = 100;
      }

      return hsla_impl(hsl_color.h,
                       hslcolorS,
                       hsl_color.l,
                       rgb_color->a(),
                       ctx,
                       pstate);
    }

    Signature desaturate_sig = "desaturate($color, $amount)";
    BUILT_IN(desaturate)
    {
      Color_Ptr rgb_color = ARG("$color", Color);
      double amount = DARG_U_PRCT("$amount");
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());

      double hslcolorS = hsl_color.s - amount;

      // Saturation cannot be below 0 or above 100
      if (hslcolorS <= 0) {
        hslcolorS = 0;
      }
      if (hslcolorS > 100) {
        hslcolorS = 100;
      }

      return hsla_impl(hsl_color.h,
                       hslcolorS,
                       hsl_color.l,
                       rgb_color->a(),
                       ctx,
                       pstate);
    }

    Signature grayscale_sig = "grayscale($color)";
    BUILT_IN(grayscale)
    {
      // CSS3 filter function overload: pass literal through directly
      Number_Ptr amount = Cast<Number>(env["$color"]);
      if (amount) {
        return SASS_MEMORY_NEW(String_Quoted, pstate, "grayscale(" + amount->to_string(ctx.c_options) + ")");
      }

      Color_Ptr rgb_color = ARG("$color", Color);
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());
      return hsla_impl(hsl_color.h,
                       0.0,
                       hsl_color.l,
                       rgb_color->a(),
                       ctx,
                       pstate);
    }

    Signature complement_sig = "complement($color)";
    BUILT_IN(complement)
    {
      Color_Ptr rgb_color = ARG("$color", Color);
      HSL hsl_color = rgb_to_hsl(rgb_color->r(),
                                 rgb_color->g(),
                                 rgb_color->b());
      return hsla_impl(hsl_color.h - 180.0,
                       hsl_color.s,
                       hsl_color.l,
                       rgb_color->a(),
                       ctx,
                       pstate);
    }

    Signature invert_sig = "invert($color, $weight: 100%)";
    BUILT_IN(invert)
    {
      // CSS3 filter function overload: pass literal through directly
      Number_Ptr amount = Cast<Number>(env["$color"]);
      if (amount) {
        return SASS_MEMORY_NEW(String_Quoted, pstate, "invert(" + amount->to_string(ctx.c_options) + ")");
      }

      double weight = DARG_U_PRCT("$weight");
      Color_Ptr rgb_color = ARG("$color", Color);
      Color_Obj inv = SASS_MEMORY_NEW(Color,
                             pstate,
                             255 - rgb_color->r(),
                             255 - rgb_color->g(),
                             255 - rgb_color->b(),
                             rgb_color->a());
      return colormix(ctx, pstate, inv, rgb_color, weight);
    }

    ////////////////////
    // OPACITY FUNCTIONS
    ////////////////////
    Signature alpha_sig = "alpha($color)";
    Signature opacity_sig = "opacity($color)";
    BUILT_IN(alpha)
    {
      String_Constant_Ptr ie_kwd = Cast<String_Constant>(env["$color"]);
      if (ie_kwd) {
        return SASS_MEMORY_NEW(String_Quoted, pstate, "alpha(" + ie_kwd->value() + ")");
      }

      // CSS3 filter function overload: pass literal through directly
      Number_Ptr amount = Cast<Number>(env["$color"]);
      if (amount) {
        return SASS_MEMORY_NEW(String_Quoted, pstate, "opacity(" + amount->to_string(ctx.c_options) + ")");
      }

      return SASS_MEMORY_NEW(Number, pstate, ARG("$color", Color)->a());
    }

    Signature opacify_sig = "opacify($color, $amount)";
    Signature fade_in_sig = "fade-in($color, $amount)";
    BUILT_IN(opacify)
    {
      Color_Ptr color = ARG("$color", Color);
      double amount = DARG_U_FACT("$amount");
      double alpha = std::min(color->a() + amount, 1.0);
      return SASS_MEMORY_NEW(Color,
                             pstate,
                             color->r(),
                             color->g(),
                             color->b(),
                             alpha);
    }

    Signature transparentize_sig = "transparentize($color, $amount)";
    Signature fade_out_sig = "fade-out($color, $amount)";
    BUILT_IN(transparentize)
    {
      Color_Ptr color = ARG("$color", Color);
      double amount = DARG_U_FACT("$amount");
      double alpha = std::max(color->a() - amount, 0.0);
      return SASS_MEMORY_NEW(Color,
                             pstate,
                             color->r(),
                             color->g(),
                             color->b(),
                             alpha);
    }

    ////////////////////////
    // OTHER COLOR FUNCTIONS
    ////////////////////////

    Signature adjust_color_sig = "adjust-color($color, $red: false, $green: false, $blue: false, $hue: false, $saturation: false, $lightness: false, $alpha: false)";
    BUILT_IN(adjust_color)
    {
      Color_Ptr color = ARG("$color", Color);
      Number_Ptr r = Cast<Number>(env["$red"]);
      Number_Ptr g = Cast<Number>(env["$green"]);
      Number_Ptr b = Cast<Number>(env["$blue"]);
      Number_Ptr h = Cast<Number>(env["$hue"]);
      Number_Ptr s = Cast<Number>(env["$saturation"]);
      Number_Ptr l = Cast<Number>(env["$lightness"]);
      Number_Ptr a = Cast<Number>(env["$alpha"]);

      bool rgb = r || g || b;
      bool hsl = h || s || l;

      if (rgb && hsl) {
        error("Cannot specify HSL and RGB values for a color at the same time for `adjust-color'", pstate, traces);
      }
      if (rgb) {
        double rr = r ? DARG_R_BYTE("$red") : 0;
        double gg = g ? DARG_R_BYTE("$green") : 0;
        double bb = b ? DARG_R_BYTE("$blue") : 0;
        double aa = a ? DARG_R_FACT("$alpha") : 0;
        return SASS_MEMORY_NEW(Color,
                               pstate,
                               color->r() + rr,
                               color->g() + gg,
                               color->b() + bb,
                               color->a() + aa);
      }
      if (hsl) {
        HSL hsl_struct = rgb_to_hsl(color->r(), color->g(), color->b());
        double ss = s ? DARG_R_PRCT("$saturation") : 0;
        double ll = l ? DARG_R_PRCT("$lightness") : 0;
        double aa = a ? DARG_R_FACT("$alpha") : 0;
        return hsla_impl(hsl_struct.h + (h ? h->value() : 0),
                         hsl_struct.s + ss,
                         hsl_struct.l + ll,
                         color->a() + aa,
                         ctx,
                         pstate);
      }
      if (a) {
        return SASS_MEMORY_NEW(Color,
                               pstate,
                               color->r(),
                               color->g(),
                               color->b(),
                               color->a() + (a ? a->value() : 0));
      }
      error("not enough arguments for `adjust-color'", pstate, traces);
      // unreachable
      return color;
    }

    Signature scale_color_sig = "scale-color($color, $red: false, $green: false, $blue: false, $hue: false, $saturation: false, $lightness: false, $alpha: false)";
    BUILT_IN(scale_color)
    {
      Color_Ptr color = ARG("$color", Color);
      Number_Ptr r = Cast<Number>(env["$red"]);
      Number_Ptr g = Cast<Number>(env["$green"]);
      Number_Ptr b = Cast<Number>(env["$blue"]);
      Number_Ptr h = Cast<Number>(env["$hue"]);
      Number_Ptr s = Cast<Number>(env["$saturation"]);
      Number_Ptr l = Cast<Number>(env["$lightness"]);
      Number_Ptr a = Cast<Number>(env["$alpha"]);

      bool rgb = r || g || b;
      bool hsl = h || s || l;

      if (rgb && hsl) {
        error("Cannot specify HSL and RGB values for a color at the same time for `scale-color'", pstate, traces);
      }
      if (rgb) {
        double rscale = (r ? DARG_R_PRCT("$red") : 0.0) / 100.0;
        double gscale = (g ? DARG_R_PRCT("$green") : 0.0) / 100.0;
        double bscale = (b ? DARG_R_PRCT("$blue") : 0.0) / 100.0;
        double ascale = (a ? DARG_R_PRCT("$alpha") : 0.0) / 100.0;
        return SASS_MEMORY_NEW(Color,
                               pstate,
                               color->r() + rscale * (rscale > 0.0 ? 255 - color->r() : color->r()),
                               color->g() + gscale * (gscale > 0.0 ? 255 - color->g() : color->g()),
                               color->b() + bscale * (bscale > 0.0 ? 255 - color->b() : color->b()),
                               color->a() + ascale * (ascale > 0.0 ? 1.0 - color->a() : color->a()));
      }
      if (hsl) {
        double hscale = (h ? DARG_R_PRCT("$hue") : 0.0) / 100.0;
        double sscale = (s ? DARG_R_PRCT("$saturation") : 0.0) / 100.0;
        double lscale = (l ? DARG_R_PRCT("$lightness") : 0.0) / 100.0;
        double ascale = (a ? DARG_R_PRCT("$alpha") : 0.0) / 100.0;
        HSL hsl_struct = rgb_to_hsl(color->r(), color->g(), color->b());
        hsl_struct.h += hscale * (hscale > 0.0 ? 360.0 - hsl_struct.h : hsl_struct.h);
        hsl_struct.s += sscale * (sscale > 0.0 ? 100.0 - hsl_struct.s : hsl_struct.s);
        hsl_struct.l += lscale * (lscale > 0.0 ? 100.0 - hsl_struct.l : hsl_struct.l);
        double alpha = color->a() + ascale * (ascale > 0.0 ? 1.0 - color->a() : color->a());
        return hsla_impl(hsl_struct.h, hsl_struct.s, hsl_struct.l, alpha, ctx, pstate);
      }
      if (a) {
        double ascale = (DARG_R_PRCT("$alpha")) / 100.0;
        return SASS_MEMORY_NEW(Color,
                               pstate,
                               color->r(),
                               color->g(),
                               color->b(),
                               color->a() + ascale * (ascale > 0.0 ? 1.0 - color->a() : color->a()));
      }
      error("not enough arguments for `scale-color'", pstate, traces);
      // unreachable
      return color;
    }

    Signature change_color_sig = "change-color($color, $red: false, $green: false, $blue: false, $hue: false, $saturation: false, $lightness: false, $alpha: false)";
    BUILT_IN(change_color)
    {
      Color_Ptr color = ARG("$color", Color);
      Number_Ptr r = Cast<Number>(env["$red"]);
      Number_Ptr g = Cast<Number>(env["$green"]);
      Number_Ptr b = Cast<Number>(env["$blue"]);
      Number_Ptr h = Cast<Number>(env["$hue"]);
      Number_Ptr s = Cast<Number>(env["$saturation"]);
      Number_Ptr l = Cast<Number>(env["$lightness"]);
      Number_Ptr a = Cast<Number>(env["$alpha"]);

      bool rgb = r || g || b;
      bool hsl = h || s || l;

      if (rgb && hsl) {
        error("Cannot specify HSL and RGB values for a color at the same time for `change-color'", pstate, traces);
      }
      if (rgb) {
        return SASS_MEMORY_NEW(Color,
                               pstate,
                               r ? DARG_U_BYTE("$red") : color->r(),
                               g ? DARG_U_BYTE("$green") : color->g(),
                               b ? DARG_U_BYTE("$blue") : color->b(),
                               a ? DARG_U_BYTE("$alpha") : color->a());
      }
      if (hsl) {
        HSL hsl_struct = rgb_to_hsl(color->r(), color->g(), color->b());
        if (h) hsl_struct.h = std::fmod(h->value(), 360.0);
        if (s) hsl_struct.s = DARG_U_PRCT("$saturation");
        if (l) hsl_struct.l = DARG_U_PRCT("$lightness");
        double alpha = a ? DARG_U_FACT("$alpha") : color->a();
        return hsla_impl(hsl_struct.h, hsl_struct.s, hsl_struct.l, alpha, ctx, pstate);
      }
      if (a) {
        double alpha = DARG_U_FACT("$alpha");
        return SASS_MEMORY_NEW(Color,
                               pstate,
                               color->r(),
                               color->g(),
                               color->b(),
                               alpha);
      }
      error("not enough arguments for `change-color'", pstate, traces);
      // unreachable
      return color;
    }

    template <size_t range>
    static double cap_channel(double c) {
      if      (c > range) return range;
      else if (c < 0)     return 0;
      else                return c;
    }

    Signature ie_hex_str_sig = "ie-hex-str($color)";
    BUILT_IN(ie_hex_str)
    {
      Color_Ptr c = ARG("$color", Color);
      double r = cap_channel<0xff>(c->r());
      double g = cap_channel<0xff>(c->g());
      double b = cap_channel<0xff>(c->b());
      double a = cap_channel<1>   (c->a()) * 255;

      std::stringstream ss;
      ss << '#' << std::setw(2) << std::setfill('0');
      ss << std::hex << std::setw(2) << static_cast<unsigned long>(Sass::round(a, ctx.c_options.precision));
      ss << std::hex << std::setw(2) << static_cast<unsigned long>(Sass::round(r, ctx.c_options.precision));
      ss << std::hex << std::setw(2) << static_cast<unsigned long>(Sass::round(g, ctx.c_options.precision));
      ss << std::hex << std::setw(2) << static_cast<unsigned long>(Sass::round(b, ctx.c_options.precision));

      std::string result(ss.str());
      for (size_t i = 0, L = result.length(); i < L; ++i) {
        result[i] = std::toupper(result[i]);
      }
      return SASS_MEMORY_NEW(String_Quoted, pstate, result);
    }

  }

}
