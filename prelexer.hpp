namespace Sass {
  namespace Prelexer {

    typedef int (*ctype_predicate)(int);
    typedef char* (*prelexer)(char*);

    // Match a single character literal.
    template <char pre>
    char* exactly(char* src) {
      return *src == pre ? src + 1 : 0;
    }
  
    // Match a string constant.
    template <const char* prefix>
    char* exactly(char* src) {
      const char* pre = prefix;
      while (*pre && *src == *pre) ++src, ++pre;
      return *pre ? 0 : src;
    }

    // Match a single character that satifies the supplied ctype predicate.
    template <ctype_predicate pred>
    char* class_char(char* src) {
      return pred(*src) ? src + 1 : 0;
    }

    // Match a single character that is a member of the supplied class.
    template <const char* char_class>
    char* class_char(char* src) {
      const char* cc = char_class;
      while (*cc && *src != *cc) ++cc;
      return *cc ? src + 1 : 0;
    }

    // Match a sequence of characters that all satisfy the supplied ctype predicate.
    template <ctype_predicate pred>
    char* class_chars(char* src) {
      char* p = src;
      while (pred(*p)) ++p;
      return p == src ? 0 : p;
    }

    // Match a sequence of characters that are all members of the supplied class.
    template <const char* char_class>
    char* class_chars(char* src) {
      char* p = src;
      while (class_char<char_class>(p)) ++p;
      return p == src ? 0 : p;
    }
    
    // Match a sequence of characters up to the next newline.
    template <const char* prefix>
    char* to_endl(char* src) {
      if (!(src = exactly<prefix>(src))) return 0;
      while (*src && *src != '\n') ++src;
      return src;
    }
    
    // Match a sequence of characters delimited by the supplied chars.
    template <char beg, char end, bool esc>
    char* delimited_by(char* src) {
      src = exactly<beg>(src);
      if (!src) return 0;
      char* stop;
      while (1) {
        if (!*src) return 0;
        stop = exactly<end>(src);
        if (stop && (!esc || *(src - 1) != '\\')) return stop;
        src = stop ? stop : src + 1;
      }
    }
    
    // Match a sequence of characters delimited by the supplied strings.
    template <const char* beg, const char* end, bool esc>
    char* delimited_by(char* src) {
      src = exactly<beg>(src);
      if (!src) return 0;
      char* stop;
      while (1) {
        if (!*src) return 0;
        stop = exactly<end>(src);
        if (stop && (!esc || *(src - 1) != '\\')) return stop;
        src = stop ? stop : src + 1;
      }
    }
    
    // Matches zero characters (always succeeds without consuming input).
    char* epsilon(char*);
    
    // Matches the empty string.
    char* empty(char*);
    
    // Succeeds of the supplied matcher fails, and vice versa.
    template <prelexer mx>
    char* negate(char* src) {
      return mx(src) ? 0 : src;
    }
    
    // Tries the matchers in sequence and returns the first match (or none)
    template <prelexer mx1, prelexer mx2>
    char* alternatives(char* src) {
      char* rslt;
      (rslt = mx1(src)) || (rslt = mx2(src));
      return rslt;
    }
    
    // Same as above, but with 3 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3>
    char* alternatives(char* src) {
      char* rslt;
      (rslt = mx1(src)) || (rslt = mx2(src)) || (rslt = mx3(src));
      return rslt;
    }
    
    // Same as above, but with 4 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3, prelexer mx4>
    char* alternatives(char* src) {
      char* rslt;
      (rslt = mx1(src)) || (rslt = mx2(src)) ||
      (rslt = mx3(src)) || (rslt = mx4(src));
      return rslt;
    }
    
    // Same as above, but with 5 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3,
              prelexer mx4, prelexer mx5>
    char* alternatives(char* src) {
      char* rslt;
      (rslt = mx1(src)) || (rslt = mx2(src)) || (rslt = mx3(src)) ||
      (rslt = mx4(src)) || (rslt = mx5(src));
      return rslt;
    }
    
    // Same as above, but with 6 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3,
              prelexer mx4, prelexer mx5, prelexer mx6>
    char* alternatives(char* src) {
      char* rslt;
      (rslt = mx1(src)) || (rslt = mx2(src)) || (rslt = mx3(src)) ||
      (rslt = mx4(src)) || (rslt = mx5(src)) || (rslt = mx6(src));
      return rslt;
    }
    
    // Tries the matchers in sequence and succeeds if they all succeed.
    template <prelexer mx1, prelexer mx2>
    char* sequence(char* src) {
      char* rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt));
      return rslt;
    }
    
    // Same as above, but with 3 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3>
    char* sequence(char* src) {
      char* rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt)) && (rslt = mx3(rslt));
      return rslt;
    }
    
    // Same as above, but with 4 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3, prelexer mx4>
    char* sequence(char* src) {
      char* rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt)) &&
      (rslt = mx3(rslt)) && (rslt = mx4(rslt));
      return rslt;
    }
    
    // Same as above, but with 5 arguments.
    template <prelexer mx1, prelexer mx2,
              prelexer mx3, prelexer mx4,
              prelexer mx5>
    char* sequence(char* src) {
      char* rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt)) &&
      (rslt = mx3(rslt)) && (rslt = mx4(rslt)) &&
      (rslt = mx5(rslt));
      return rslt;
    }
    
    // Same as above, but with 6 arguments.
    template <prelexer mx1, prelexer mx2,
              prelexer mx3, prelexer mx4,
              prelexer mx5, prelexer mx6>
    char* sequence(char* src) {
      char* rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt)) &&
      (rslt = mx3(rslt)) && (rslt = mx4(rslt)) &&
      (rslt = mx5(rslt)) && (rslt = mx6(rslt));
      return rslt;
    }
    
    // Same as above, but with 7 arguments.
    template <prelexer mx1, prelexer mx2,
              prelexer mx3, prelexer mx4,
              prelexer mx5, prelexer mx6,
              prelexer mx7>
    char* sequence(char* src) {
      char* rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt)) &&
      (rslt = mx3(rslt)) && (rslt = mx4(rslt)) &&
      (rslt = mx5(rslt)) && (rslt = mx6(rslt)) &&
      (rslt = mx7(rslt));
      return rslt;
    }
    
    // Match a pattern or not. Always succeeds.
    template <prelexer mx>
    char* optional(char* src) {
      char* p = mx(src);
      return p ? p : src;
    }
    
    // Match zero or more of the supplied pattern
    template <prelexer mx>
    char* zero_plus(char* src) {
      char* p = mx(src);
      while (p) src = p, p = mx(src);
      return src;
    }
    
    // Match one or more of the supplied pattern
    template <prelexer mx>
    char* one_plus(char* src) {
      char* p = mx(src);
      if (!p) return 0;
      while (p) src = p, p = mx(src);
      return src;
    }
    
    // Match a single character satisfying the ctype predicates.
    char *space(char *src);
    char *alpha(char *src);
    char *digit(char *src);
    char *xdigit(char *src);
    char *alnum(char *src);
    char *punct(char *src);
    // Match multiple ctype characters.
    char* spaces(char* src);
    char* alphas(char* src);
    char* digits(char* src);
    char* xdigits(char* src);
    char* alnums(char* src);
    char* puncts(char* src);
    
    // Match a line comment.
    char* line_comment(char* src);
    // Match a block comment.
    char* block_comment(char* src);
    // Match either.
    char* comment(char* src);
    // Match double- and single-quoted strings.
    char* double_quoted_string(char* src);
    char* single_quoted_string(char* src);
    char* string_constant(char* src);
    // Match interpolants.
    char* interpolant(char* src);

    // Whitespace handling.
    char* optional_spaces(char* src);
    char* optional_comment(char* src);
    char* spaces_and_comments(char* src);
    char* no_spaces(char *src);

    // Match a CSS identifier.
    char* identifier(char* src);
    // Match CSS '@' keywords.
    char* at_keyword(char* src);
    // Match CSS type selectors
    char* namespace_prefix(char* src);
    char* type_selector(char* src);
    char* universal(char* src);
    // Match CSS id names.
    char* id_name(char* src);
    // Match CSS class names.
    char* class_name(char* src);
    // Match CSS numeric constants.
    char* sign(char* src);
    char* unsigned_number(char* src);
    char* number(char* src);
    char* coefficient(char* src);
    char* binomial(char* src);
    char* percentage(char* src);
    char* dimension(char* src);
    char* hex(char* src);
    // Match CSS uri specifiers.
    char* uri(char* src);
    // Match CSS pseudo-class/element prefixes
    char* pseudo_prefix(char* src);
    // Match CSS function call openers.
    char* functional(char* src);
    // Match CSS 'odd' and 'even' keywords for functional pseudo-classes.
    char* even(char* src);
    char* odd(char* src);
    // Match CSS attribute-matching operators.
    char* exact_match(char* src);
    char* class_match(char* src);
    char* dash_match(char* src);
    char* prefix_match(char* src);
    char* suffix_match(char* src);
    char* substring_match(char* src);
    // Match CSS combinators.
    char* adjacent_to(char* src);
    char* precedes(char* src);
    char* parent_of(char* src);
    char* ancestor_of(char* src);
    
    // Match SCSS variable names.
    char* variable(char* src);
    
    // Utility functions for finding and counting characters in a string.
    template<char c>
    char* find_first(char* src) {
      while (*src && *src != c) ++src;
      return *src ? src : 0;
    }
    template<prelexer mx>
    char* find_first(char* src) {
      while (*src && !mx(src)) ++src;
      return *src ? src : 0;
    }
    template <char c>
    unsigned int count_interval(char* beg, char* end) {
      unsigned int counter = 0;
      while (beg < end && *beg) {
        if (*beg == c) ++counter;
        ++beg;
      }
      return counter;
    }
    template <prelexer mx>
    unsigned int count_interval(char* beg, char* end) {
      unsigned int counter = 0;
      while (beg < end && *beg) {
        char* p;
        if (p = mx(beg)) {
          ++counter;
          beg = p;
        }
        else {
          ++beg;
        }
      }
      return counter;
    }
    
  }
}
