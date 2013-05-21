#include "operation.hpp"

#ifndef SASS_AST
#include "ast.hpp"
#endif

namespace Sass {

	template <typename T>
	T Operation<T>::operator()(AST_Node* node) { return node->perform(this); }

}