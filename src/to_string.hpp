#include <string>

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

namespace Sass {
	class To_String : public Operation<string> {
	public:
		virtual ~To_String() { }
	};
}