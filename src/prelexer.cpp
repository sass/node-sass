#include <cctype>
#include "prelexer.hpp"

namespace Sass {
  namespace Prelexer {
    
    // Matches zero characters (always succeeds without consuming input).
    char *epsilon(char *src) {
      return src;
    }
    // Matches the empty string.
    char *empty(char *src) {
      return *src ? 0 : src;
    }
    
    // Match a single character satisfying the ctype predicates.
    char* space(char* src) { return std::isspace(*src) ? src+1 : 0; }
    char* alpha(char* src) { return std::isalpha(*src) ? src+1 : 0; }
    char* digit(char* src) { return std::isdigit(*src) ? src+1 : 0; }
    char* xdigit(char* src) { return std::isxdigit(*src) ? src+1 : 0; }
    char* alnum(char* src) { return std::isalnum(*src) ? src+1 : 0; }
    char* punct(char* src) { return std::ispunct(*src) ? src+1 : 0; }
    // Match multiple ctype characters.
    char* spaces(char* src) { return one_plus<space>(src); }
    char* alphas(char* src) { return one_plus<alpha>(src); }
    char* digits(char* src) { return one_plus<digit>(src); }
    char* xdigits(char* src) { return one_plus<xdigit>(src); }
    char* alnums(char* src) { return one_plus<alnum>(src); }
    char* puncts(char* src) { return one_plus<punct>(src); }
    
    // Match a line comment.
    extern const char slash_slash[] = "//";
    char* line_comment(char* src) { return to_endl<slash_slash>(src); }
    // Match a block comment.
    extern const char slash_star[] = "/*";
    extern const char star_slash[] = "*/";
    char* block_comment(char* src) {
      return delimited_by<slash_star, star_slash, false>(src);
    }
    // Match double- and single-quoted strings.
    char* double_quoted_string(char* src) {
      return delimited_by<'"', '"', true>(src);
    }
    char* single_quoted_string(char* src) {
      return delimited_by<'\'', '\'', true>(src);
    }
    char* string_constant(char* src) {
      return alternatives<double_quoted_string, single_quoted_string>(src);
    }
    // Match interpolants.
    extern const char hash_lbrace[] = "#{";
    extern const char rbrace[] = "}";
    char* interpolant(char* src) {
      return delimited_by<hash_lbrace, rbrace, false>(src);
    }
    
    // Match CSS identifiers.
    char* identifier(char* src) {
      return sequence< optional< exactly<'-'> >,
                       alternatives< alpha, exactly<'_'> >,
                       zero_plus< alternatives< alnum,
                                                exactly<'-'>,
                                                exactly<'_'> > > >(src);
    }
    // Match CSS '@' keywords.
    char* at_keyword(char* src) {
      return sequence<exactly<'@'>, identifier>(src);
    }
    char* name(char* src) {
      return one_plus< alternatives< alnum,
                                     exactly<'-'>,
                                     exactly<'_'> > >(src);
    }
    // Match CSS id names.
    char* id_name(char* src) {
      return sequence<exactly<'#'>, name>(src);
    }
    // Match CSS class names.
    char* class_name(char* src) {
      return sequence<exactly<'.'>, identifier>(src);
    }
    // Match CSS numeric constants.
    extern const char sign[] = "-+";
    char* unsigned_number(char* src) {
      return alternatives<sequence< zero_plus<digits>,
                                    exactly<'.'>,
                                    one_plus<digits> >,
                          digits>(src);
    }
    char* number(char* src) {
      return sequence< optional< class_char<sign> >, unsigned_number>(src);
    }
    char* percentage(char* src) {
      return sequence< number, exactly<'%'> >(src);
    }
    char* dimension(char* src) {
      return sequence<number, identifier>(src);
    }
    // Match CSS uri specifiers.
    extern const char url_call[] = "url(";
    char* uri(char* src) {
      return sequence< exactly<url_call>,
                       optional<spaces>,
                       string_constant,
                       optional<spaces>,
                       exactly<')'> >(src);
    }
    // Match CSS function call openers.
    char* function(char* src) {
      return sequence< identifier, exactly<'('> >(src);
    }
    // Match CSS attribute-matching operators.
    extern const char tilde_equal[]  = "~=";
    extern const char pipe_equal[]   = "|=";
    extern const char caret_equal[]  = "^=";
    extern const char dollar_equal[] = "$=";
    extern const char star_equal[]   = "*=";
    char* exact_match(char* src) { return exactly<'='>(src); }
    char* class_match(char* src) { return exactly<tilde_equal>(src); }
    char* dash_match(char* src) { return exactly<pipe_equal>(src); }
    char* prefix_match(char* src) { return exactly<caret_equal>(src); }
    char* suffix_match(char* src) { return exactly<dollar_equal>(src); }
    char* substring_match(char* src) { return exactly<star_equal>(src); }
  }
}