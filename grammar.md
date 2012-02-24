Initial pass at a Sass grammar. Nowhere close to complete.
              
    stylesheet    ->  statement*
              
    statement     ->  ruleset
              
    ruleset       ->  SELECTOR '{' declarations '}'
              
    declarations  ->  rule [';' declarations]?
                  ->  ruleset declarations?
                  ->  []
              
    rule          ->  property ':' value
                  ->  []