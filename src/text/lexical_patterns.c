#include <stdio.h>
#include "lexical_patterns.h"

ALTERNATIVES_MATCHER(semicolon_or_lbrack, prefix_is_semicolon, prefix_is_lbrack);

CHARS_MATCHER(import,  "@import");
CHARS_MATCHER(include, "@include");
CHARS_MATCHER(mixin,   "@mixin");
CHARS_MATCHER(extend,  "@extend");

static SEQUENCE_MATCHER(sign_and_identifier, prefix_is_sign, prefix_is_identifier);
static ALTERNATIVES_MATCHER(word_initial, prefix_is_identifier, prefix_is_sign_and_identifier);
static ALTERNATIVES_MATCHER(word_trailer, prefix_is_identifier, prefix_is_sign, prefix_is_digits);

FIRST_REST_MATCHER(word, prefix_is_word_initial, prefix_is_word_trailer);

SEQUENCE_MATCHER(class, prefix_is_dot, prefix_is_word);
SEQUENCE_MATCHER(id, prefix_is_hash, prefix_is_word);
SEQUENCE_MATCHER(pseudo_class, prefix_is_colon, prefix_is_word);