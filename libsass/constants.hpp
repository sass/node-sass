#define SASS_CONSTANTS

namespace Sass {
  namespace Constants {

    // hidden variable name for the image path (for the image-url built-in)
    extern const char image_path_var[];

    // sass keywords
    extern const char import_kwd[];
    extern const char mixin_kwd[];
    extern const char function_kwd[];
    extern const char return_kwd[];
    extern const char include_kwd[];
    extern const char extend_kwd[];
    extern const char if_kwd[];
    extern const char else_kwd[];
    extern const char if_after_else_kwd[];
    extern const char for_kwd[];
    extern const char from_kwd[];
    extern const char to_kwd[];
    extern const char through_kwd[];
    extern const char each_kwd[];
    extern const char in_kwd[];
    extern const char while_kwd[];
    extern const char warn_kwd[];
    extern const char default_kwd[];

    // css standard units
    extern const char em_kwd[];
    extern const char ex_kwd[];
    extern const char px_kwd[];
    extern const char cm_kwd[];
    extern const char mm_kwd[];
    extern const char pt_kwd[];
    extern const char pc_kwd[];
    extern const char deg_kwd[];
    extern const char rad_kwd[];
    extern const char grad_kwd[];
    extern const char ms_kwd[];
    extern const char s_kwd[];
    extern const char Hz_kwd[];
    extern const char kHz_kwd[];

    // css functions and keywords
    extern const char media_kwd[];
    extern const char only_kwd[];
    extern const char rgb_kwd[];
    extern const char url_kwd[];
    extern const char image_url_kwd[];
    extern const char important_kwd[];
    extern const char pseudo_not_kwd[];
    extern const char even_kwd[];
    extern const char odd_kwd[];

    // css attribute-matching operators
    extern const char tilde_equal[];
    extern const char pipe_equal[];
    extern const char caret_equal[];
    extern const char dollar_equal[];
    extern const char star_equal[];

    // relational & logical operators and constants
    extern const char and_kwd[];
    extern const char or_kwd[];
    extern const char not_kwd[];
    extern const char gt[];
    extern const char gte[];
    extern const char lt[];
    extern const char lte[];
    extern const char eq[];
    extern const char neq[];
    extern const char true_kwd[];
    extern const char false_kwd[];

    // miscellaneous punctuation and delimiters
    extern const char percent_str[];
    extern const char empty_str[];
    extern const char slash_slash[];
    extern const char slash_star[];
    extern const char star_slash[];
    extern const char hash_lbrace[];
    extern const char rbrace[];
    extern const char rparen[];
    extern const char sign_chars[];

    // type names
    extern const char numeric_name[];
    extern const char number_name[];
    extern const char percentage_name[];
    extern const char dimension_name[];
    extern const char string_name[];
    extern const char bool_name[];
    extern const char color_name[];
    extern const char list_name[];
  }
}