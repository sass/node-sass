#define SASS_NODE_TYPE_INCLUDED

namespace Sass {

  enum Node_Type {
    none,
    flags,
    comment,

    root,
    ruleset,
    propset,

    selector_group,
    selector,
    selector_combinator,
    simple_selector_sequence,
    backref,
    simple_selector,
    type_selector,
    class_selector,
    id_selector,
    pseudo,
    pseudo_negation,
    functional_pseudo,
    attribute_selector,

    block,
    rule,
    property,

    nil,
    comma_list,
    space_list,

    disjunction,
    conjunction,

    relation,
    eq,
    neq,
    gt,
    gte,
    lt,
    lte,

    expression,
    add,
    sub,

    term,
    mul,
    div,

    factor,
    unary_plus,
    unary_minus,
    values,
    value,
    identifier,
    uri,
    textual_percentage,
    textual_dimension,
    textual_number,
    textual_hex,
    color_name,
    string_constant,
    number,
    numeric_percentage,
    numeric_dimension,
    numeric_color,
    boolean,
    important,

    value_schema,
    string_schema,

    css_import,
    function_call,
    mixin,
    parameters,
    expansion,
    arguments,

    variable,
    assignment
  };

}