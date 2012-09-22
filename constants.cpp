#include "constants.hpp"

namespace Sass {
  namespace Constants {

    // hidden variable name for the image path (for the image-url built-in)
    extern const char image_path_var[] = "$[image path]";

    // sass keywords
    extern const char import_kwd[]        = "@import";
    extern const char mixin_kwd[]         = "@mixin";
    extern const char function_kwd[]      = "@function";
    extern const char return_kwd[]        = "@return";
    extern const char include_kwd[]       = "@include";
    extern const char extend_kwd[]        = "@extend";
    extern const char if_kwd[]            = "@if";
    extern const char else_kwd[]          = "@else";
    extern const char if_after_else_kwd[] = "if";
    extern const char for_kwd[]           = "@for";
    extern const char from_kwd[]          = "from";
    extern const char to_kwd[]            = "to";
    extern const char through_kwd[]       = "through";
    extern const char each_kwd[]          = "@each";
    extern const char in_kwd[]            = "in";
    extern const char while_kwd[]         = "@while";
    extern const char warn_kwd[]          = "@warn";
    extern const char default_kwd[]       = "default";

    // css standard units
    extern const char em_kwd[]   = "em";
    extern const char ex_kwd[]   = "ex";
    extern const char px_kwd[]   = "px";
    extern const char cm_kwd[]   = "cm";
    extern const char mm_kwd[]   = "mm";
    extern const char pt_kwd[]   = "pt";
    extern const char pc_kwd[]   = "pc";
    extern const char deg_kwd[]  = "deg";
    extern const char rad_kwd[]  = "rad";
    extern const char grad_kwd[] = "grad";
    extern const char ms_kwd[]   = "ms";
    extern const char s_kwd[]    = "s";
    extern const char Hz_kwd[]   = "Hz";
    extern const char kHz_kwd[]  = "kHz";

    // css functions and keywords
    extern const char media_kwd[]      = "@media";
    extern const char only_kwd[]       = "only";
    extern const char rgb_kwd[]        = "rgb(";
    extern const char url_kwd[]        = "url(";
    extern const char image_url_kwd[]  = "image-url(";
    extern const char important_kwd[]  = "important";
    extern const char pseudo_not_kwd[] = ":not(";
    extern const char even_kwd[]       = "even";
    extern const char odd_kwd[]        = "odd";

    // css attribute-matching operators
    extern const char tilde_equal[]  = "~=";
    extern const char pipe_equal[]   = "|=";
    extern const char caret_equal[]  = "^=";
    extern const char dollar_equal[] = "$=";
    extern const char star_equal[]   = "*=";

    // relational & logical operators and constants
    extern const char and_kwd[]   = "and";
    extern const char or_kwd[]    = "or";
    extern const char not_kwd[]   = "not";
    extern const char gt[]        = ">";
    extern const char gte[]       = ">=";
    extern const char lt[]        = "<";
    extern const char lte[]       = "<=";
    extern const char eq[]        = "==";
    extern const char neq[]       = "!=";
    extern const char true_kwd[]  = "true";
    extern const char false_kwd[] = "false";

    // miscellaneous punctuation and delimiters
    extern const char percent_str[] = "%";
    extern const char empty_str[]   = "";
    extern const char slash_slash[] = "//";
    extern const char slash_star[]  = "/*";
    extern const char star_slash[]  = "*/";
    extern const char hash_lbrace[] = "#{";
    extern const char rbrace[]      = "}";
    extern const char rparen[]      = ")";
    extern const char sign_chars[]  = "-+";

    // type names
    extern const char numeric_name[]    = "numeric value";
    extern const char number_name[]     = "number";
    extern const char percentage_name[] = "percentage";
    extern const char dimension_name[]  = "numeric dimension";
    extern const char string_name[]     = "string";
    extern const char bool_name[]       = "bool";
    extern const char color_name[]      = "color";
    extern const char list_name[]       = "list";

  }
}