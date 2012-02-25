namespace Sass {
  namespace Prelexer {

    typedef int (*ctype_predicate)(int);
    typedef char *(*prelexer)(char *);

    // Match a single character literal.
    template <char pre>
    char *exactly(char *src) {
      return *src == pre ? src + 1 : 0;
    }
  
    // Match a string constant.
    template <const char *prefix>
    char *exactly(char *src) {
      char *pre = prefix;
      while (*pre && *src == *pre) ++src, ++pre;
      return *pre ? 0 : src;
    }

    // Match a single character that satifies the supplied ctype predicate.
    template <ctype_predicate pred>
    char *class_char(char *src) {
      return pred(*src) ? src + 1 : 0;
    }

    // Match a single character that is a member of the supplied class.
    template <const char *char_class>
    char *class_char(char *src) {
      char *cc = char_class;
      while (*cc && *src != *cc) ++cc;
      return *cc ? src + 1 : 0;
    }

    // Match a sequence of characters that all satisfy the supplied ctype predicate.
    template <ctype_predicate pred>
    char *class_chars(char *src) {
      char *p = src;
      while (pred(*p)) ++p;
      return p == src ? 0 : p;
    }

    // Match a sequence of characters that are all members of the supplied class.
    template <const char *char_class>
    char *class_chars(char *src) {
      char *p = src;
      while (class_char<char_class>(p)) ++p;
      return p == src ? 0 : p;
    }
    
    // Match a sequence of characters delimited by the supplied strings.
    template <const char *beg, const char *end, bool esc>
    char *delimited_by(char *src) {
      src = exactly<beg>(src);
      if (!src) return 0;
      char *stop;
      while (1) {
        if (!*src) return 0;
        stop = exactly<end>(src);
        if (stop && (!esc || *(src - 1) != '\\')) return stop;
        src = stop ? stop : src + 1;
      }
    }
    
    // Matches zero characters (always succeeds without consuming input).
    char *epsilon(char *);
    
    // Matches the empty string.
    char *empty(char *);
    
    // Succeeds of the supplied matcher fails, and vice versa.
    template <prelexer mx>
    char *negate(char *src) {
      return mx(src) ? 0 : src;
    }
    
    // Tries the matchers in sequence and returns the first match (or none)
    template <prelexer mx1, prelexer mx2>
    char *alternatives(char *src) {
      char *rslt;
      (rslt = mx1(src)) || (rslt = mx2(src));
      return rslt;
    }
    
    // Same as above, but with 3 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3>
    char *alternatives(char *src) {
      char *rslt;
      (rslt = mx1(src)) || (rslt = mx2(src)) || (rslt = mx3(src));
      return rslt;
    }
    
    // Same as above, but with 4 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3, prelexer mx4>
    char *alternatives(char *src) {
      char *rslt;
      (rslt = mx1(src)) || (rslt = mx2(src)) ||
      (rslt = mx3(src)) || (rslt = mx4(src));
      return rslt;
    }
    
    // Tries the matchers in sequence and succeeds if they all succeed.
    template <prelexer mx1, prelexer mx2>
    char *sequence(char *src) {
      char *rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt));
      return rslt;
    }
    
    // Same as above, but with 3 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3>
    char *sequence(char *src) {
      char *rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt)) && (rslt = mx3(rslt));
      return rslt;
    }
    
    // Same as above, but with 4 arguments.
    template <prelexer mx1, prelexer mx2, prelexer mx3, prelexer mx4>
    char *sequence(char *src) {
      char *rslt = src;
      (rslt = mx1(rslt)) && (rslt = mx2(rslt)) &&
      (rslt = mx3(rslt)) && (rslt = mx4(rslt));
      return rslt;
    }
    
    // Match a pattern or not. Always succeeds.
    template <prelexer mx>
    char *optional(char *src) {
      char *p = mx(src);
      return p ? p : src;
    }
    
    

  }
}
