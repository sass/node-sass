#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

namespace Sass {
	using namespace std;

	class Memory_Manager;

	class Cloner : public Operation<AST_Node*> {
		Memory_Manager& mem;
	public:
		// import all the class-specific methods and override as desired
		using Operation<AST_Node*>::operator();
		// override this to define a catch-all
		virtual void fallback(AST_Node* n);
	};
}