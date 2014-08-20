#pragma once

#include <iostream>

#ifndef SASS_AST
#include "ast.hpp"
#endif

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

namespace Sass {

    using namespace std;

    struct Context;

    class RemovePlaceholders : public Operation_CRTP<void, RemovePlaceholders> {

        Context&          ctx;

        void fallback_impl(AST_Node* n) {};

    public:
        RemovePlaceholders(Context&);
        virtual ~RemovePlaceholders() { }

        using Operation<void>::operator();

        void operator()(Block*);
        void operator()(Ruleset*);
        void operator()(Media_Block*);
        void operator()(At_Rule*);

        template <typename U>
        void fallback(U x) { return fallback_impl(x); }
    };

}
