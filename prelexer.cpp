#include <cctype>
#include "prelexer.hpp"

namespace Sass {
  namespace Prelexer {
    
    // Matches zero characters (always succeeds without consuming input).
    const char* epsilon(char *src) {
      return src;
    }
    // Matches the empty string.
    const char* empty(char *src) {
      return *src ? 0 : src;
    }
    
    // Match any single character.
    const char* any_char(const char* src) { return *src ? src++ : src; }
    
    // Match a single character satisfying the ctype predicates.
    const char* space(const char* src) { return std::isspace(*src) ? src+1 : 0; }
    const char* alpha(const char* src) { return std::isalpha(*src) ? src+1 : 0; }
    const char* digit(const char* src) { return std::isdigit(*src) ? src+1 : 0; }
    const char* xdigit(const char* src) { return std::isxdigit(*src) ? src+1 : 0; }
    const char* alnum(const char* src) { return std::isalnum(*src) ? src+1 : 0; }
    const char* punct(const char* src) { return std::ispunct(*src) ? src+1 : 0; }
    // Match multiple ctype characters.
    const char* spaces(const char* src) { return one_plus<space>(src); }
    const char* alphas(const char* src) { return one_plus<alpha>(src); }
    const char* digits(const char* src) { return one_plus<digit>(src); }
    const char* xdigits(const char* src) { return one_plus<xdigit>(src); }
    const char* alnums(const char* src) { return one_plus<alnum>(src); }
    const char* puncts(const char* src) { return one_plus<punct>(src); }
        
    // Match a line comment.
    extern const char slash_slash[] = "//";
    const char* line_comment(const char* src) { return to_endl<slash_slash>(src); }
    // Match a block comment.
    extern const char slash_star[] = "/*";
    extern const char star_slash[] = "*/";
    const char* block_comment(const char* src) {
      return sequence< optional_spaces, delimited_by<slash_star, star_slash, false> >(src);
    }
    // Match either comment.
    const char* comment(const char* src) {
      return alternatives<block_comment, line_comment>(src);
    }
    // Match double- and single-quoted strings.
    const char* double_quoted_string(const char* src) {
      return delimited_by<'"', '"', true>(src);
    }
    const char* single_quoted_string(const char* src) {
      return delimited_by<'\'', '\'', true>(src);
    }
    const char* string_constant(const char* src) {
      return alternatives<double_quoted_string, single_quoted_string>(src);
    }
    // Match interpolants.
    extern const char hash_lbrace[] = "#{";
    extern const char rbrace[] = "}";
    const char* interpolant(const char* src) {
      return delimited_by<hash_lbrace, rbrace, false>(src);
    }
    
    // Whitespace handling.
    const char* optional_spaces(const char* src) { return optional<spaces>(src); }
    const char* optional_comment(const char* src) { return optional<comment>(src); }
    const char* spaces_and_comments(const char* src) {
      return zero_plus< alternatives<spaces, comment> >(src);
    }
    const char* no_spaces(const char* src) {
      return negate< spaces >(src);
    }
    
    // Match CSS identifiers.
    const char* identifier(const char* src) {
      return sequence< optional< exactly<'-'> >,
                       alternatives< alpha, exactly<'_'> >,
                       zero_plus< alternatives< alnum,
                                                exactly<'-'>,
                                                exactly<'_'> > > >(src);
    }
    // Match CSS '@' keywords.
    const char* at_keyword(const char* src) {
      return sequence<exactly<'@'>, identifier>(src);
    }
    extern const char import_kwd[] = "@import";
    const char* import(const char* src) {
      return exactly<import_kwd>(src);
    }
    extern const char mixin_kwd[] = "@mixin";
    const char* mixin(const char* src) {
      return exactly<mixin_kwd>(src);
    }
    
    const char* name(const char* src) {
      return one_plus< alternatives< alnum,
                                     exactly<'-'>,
                                     exactly<'_'> > >(src);
    }
    // Match CSS type selectors
    const char* namespace_prefix(const char* src) {
      return sequence< optional< alternatives< identifier, exactly<'*'> > >,
                       exactly<'|'> >(src);
    }
    const char* type_selector(const char* src) {
      return sequence< optional<namespace_prefix>, identifier>(src);
    }
    const char* universal(const char* src) {
      return sequence< optional<namespace_prefix>, exactly<'*'> >(src);
    }
    // Match CSS id names.
    const char* id_name(const char* src) {
      return sequence<exactly<'#'>, name>(src);
    }
    // Match CSS class names.
    const char* class_name(const char* src) {
      return sequence<exactly<'.'>, identifier>(src);
    }
    // Match CSS numeric constants.
    extern const char sign_chars[] = "-+";
    const char* sign(const char* src) {
      return class_char<sign_chars>(src);
    }
    const char* unsigned_number(const char* src) {
      return alternatives<sequence< zero_plus<digits>,
                                    exactly<'.'>,
                                    one_plus<digits> >,
                          digits>(src);
    }
    const char* number(const char* src) {
      return sequence< optional<sign>, unsigned_number>(src);
    }
    const char* coefficient(const char* src) {
      return alternatives< sequence< optional<sign>, digits >,
                           sign >(src);
    }
    const char* binomial(const char* src) {
      return sequence< optional<sign>,
                       optional<digits>,
                       exactly<'n'>, optional_spaces,
                       sign, optional_spaces,
                       digits >(src);
    }
    const char* percentage(const char* src) {
      return sequence< number, exactly<'%'> >(src);
    }
    extern const char em_kwd[] = "em";
    extern const char ex_kwd[] = "ex";
    extern const char px_kwd[] = "px";
    extern const char cm_kwd[] = "cm";
    extern const char mm_kwd[] = "mm";
    extern const char in_kwd[] = "in";
    extern const char pt_kwd[] = "pt";
    extern const char pc_kwd[] = "pc";
    extern const char deg_kwd[] = "deg";
    extern const char rad_kwd[] = "rad";
    extern const char grad_kwd[] = "grad";
    extern const char ms_kwd[] = "ms";
    extern const char s_kwd[] = "s";
    extern const char Hz_kwd[] = "Hz";
    extern const char kHz_kwd[] = "kHz";
    const char* em(const char* src) {
      return sequence< number, exactly<em_kwd> >(src);
    }
    const char* dimension(const char* src) {
      return sequence<number, identifier>(src);
    }
    const char* hex(const char* src) {
      const char* p = sequence< exactly<'#'>, one_plus<xdigit> >(src);
      int len = p - src;
      return (len != 4 && len != 7) ? 0 : p;
    }
    extern const char rgb_kwd[] = "rgb(";
    const char* rgb_prefix(const char* src) {
      return exactly<rgb_kwd>(src);
    }
    // Match CSS uri specifiers.
    extern const char url_kwd[] = "url(";
    const char* uri_prefix(const char* src) {
      return exactly<url_kwd>(src);
    }
    const char* uri(const char* src) {
      return sequence< exactly<url_kwd>,
                       optional<spaces>,
                       string_constant,
                       optional<spaces>,
                       exactly<')'> >(src);
    }
    // Match CSS "!important" keyword.
    extern const char important_kwd[] = "important";
    const char* important(const char* src) {
      return sequence< exactly<'!'>,
                       spaces_and_comments,
                       exactly<important_kwd> >(src);
    }
    // Match CSS pseudo-class/element prefixes.
    const char* pseudo_prefix(const char* src) {
      return sequence< exactly<':'>, optional< exactly<':'> > >(src);
    }
    // Match CSS function call openers.
    const char* functional(const char* src) {
      return sequence< identifier, exactly<'('> >(src);
    }
    // Match the CSS negation pseudo-class.
    extern const char not_kwd[] = ":not(";
    const char* pseudo_not(const char* src) {
      return exactly< not_kwd >(src);
    }
    // Match CSS 'odd' and 'even' keywords for functional pseudo-classes.
    extern const char even_chars[] = "even";
    extern const char odd_chars[]  = "odd";
    const char* even(const char* src) {
      return exactly<even_chars>(src);
    }
    const char* odd(const char* src) {
      return exactly<odd_chars>(src);
    }
    // Match CSS attribute-matching operators.
    extern const char tilde_equal[]  = "~=";
    extern const char pipe_equal[]   = "|=";
    extern const char caret_equal[]  = "^=";
    extern const char dollar_equal[] = "$=";
    extern const char star_equal[]   = "*=";
    const char* exact_match(const char* src) { return exactly<'='>(src); }
    const char* class_match(const char* src) { return exactly<tilde_equal>(src); }
    const char* dash_match(const char* src) { return exactly<pipe_equal>(src); }
    const char* prefix_match(const char* src) { return exactly<caret_equal>(src); }
    const char* suffix_match(const char* src) { return exactly<dollar_equal>(src); }
    const char* substring_match(const char* src) { return exactly<star_equal>(src); }
    // Match CSS combinators.
    const char* adjacent_to(const char* src) {
      return sequence< optional_spaces, exactly<'+'> >(src);
    }
    const char* precedes(const char* src) {
      return sequence< optional_spaces, exactly<'~'> >(src);
    }
    const char* parent_of(const char* src) {
      return sequence< optional_spaces, exactly<'>'> >(src);
    }
    const char* ancestor_of(const char* src) {
      return sequence< spaces, negate< exactly<'{'> > >(src);
    }
    
    // Match SCSS variable names.
    const char* variable(const char* src) {
      return sequence<exactly<'$'>, name>(src);
    }
    
    // Path matching functions.
    const char* folder(const char* src) {
      return sequence< zero_plus< any_char_except<'/'> >,
                       exactly<'/'> >(src);
    }
    const char* folders(const char* src) {
      return zero_plus< folder >(src);
    }
  }
}