#include <stdio.h>
#include "lexical_patterns.h"

static SEQUENCE_MATCHER(sign_and_identifier, prefix_is_sign, prefix_is_identifier);
static ALTERNATIVES_MATCHER(word_initial, prefix_is_identifier, prefix_is_sign_and_identifier);
static ALTERNATIVES_MATCHER(word_trailer, prefix_is_identifier, prefix_is_sign, prefix_is_digits);
FIRST_REST_MATCHER(word, prefix_is_word_initial, prefix_is_word_trailer);
