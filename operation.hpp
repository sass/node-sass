#define SASS_OPERATION

namespace Sass {
	struct AST_Node;

	template<typename T>
	struct Operation {
		virtual T operator()(AST_Node* n);
		virtual ~Operation() = 0;
	};

}