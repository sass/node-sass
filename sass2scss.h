#ifdef __cplusplus

// using std::string
using namespace std;

// add namespace for c++
namespace ocbnet
{

	// pretty print options
	extern const int SASS2SCSS_PRETTIFY_0 = 0;
	extern const int SASS2SCSS_PRETTIFY_1 = 1;
	extern const int SASS2SCSS_PRETTIFY_2 = 2;
	extern const int SASS2SCSS_PRETTIFY_3 = 3;

	// remove one-line comment
	extern const int SASS2SCSS_KEEP_COMMENT    =  32;
	// remove multi-line comments
	extern const int SASS2SCSS_STRIP_COMMENT   =  64;
	// convert one-line to multi-line
	extern const int SASS2SCSS_CONVERT_COMMENT = 128;

	// maximum nested levels
	// should probably be dynamic
	const int maxnested = 256;

	// converter struct
	// hold of all states
	struct converter
	{
		// bit options
		int options;
		// indent level
		int level;
		// concat lists
		bool comma;
		// comment context
		string comment;
		// whitespace buffer
		string whitespace;
		// length should be dynamic
		// but my c++ is very rusty
		string indents[maxnested];
	};

	// only available in c++ code
	const string sass2scss (const string sass, const int options);

}
// EO namespace

// declare for c
extern "C" {
#endif

	// prettyfy print options
	#define SASS2SCSS_PRETTIFY_0   0
	#define SASS2SCSS_PRETTIFY_1   1
	#define SASS2SCSS_PRETTIFY_2   2
	#define SASS2SCSS_PRETTIFY_3   3

	// keep one-line comments
	#define SASS2SCSS_KEEP_COMMENT     32
	// remove multi-line comments
	#define SASS2SCSS_STRIP_COMMENT    64
	// convert one-line to multi-line
	#define SASS2SCSS_CONVERT_COMMENT  128

	// available to c and c++ code
	const char* sass2scss (const char* sass, const int options);

#ifdef __cplusplus
}
#endif
