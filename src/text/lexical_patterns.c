#include <stdio.h>
#include "lexical_patterns.h"
//#include "prefix_primitives.h"

static SEQUENCE_MATCHER(hyphen_and_id, prefix_is_hyphen, prefix_is_identifier);
static ALTERNATIVES_MATCHER(word_initial, prefix_is_identifier, prefix_is_hyphen_and_id);
static ALTERNATIVES_MATCHER(word_trailer, prefix_is_identifier, prefix_is_hyphen, prefix_is_digits);
FIRST_REST_MATCHER(word, prefix_is_word_initial, prefix_is_word_trailer);
