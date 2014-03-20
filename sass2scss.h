#ifdef __cplusplus

// using std::string
using namespace std;

// add namespace for c++
namespace ocbnet
{

	// maximum nested levels
	// should probably be dynamic
	const int maxnested = 256;

	// converter struct
	// hold of all states
	struct converter
	{
		// indent level
		int level;
		// pretty print option
		// set by command line
		int pretty;
		// concatenate comma lists
		bool comma;
		// comment context
		string comment;
		// whitespace buffer
		string whitespace;
		// strip some comments
		bool comment_strip;
		bool comment_convert;
		// indent strings
		// length should be dynamic
		// but my c++ is very rusty
		string indents[maxnested];
	};

	// only available in c++ code
	const string sass2scss (const string sass, const int pretty);

}
// EO namespace

// declare for c
extern "C" {
#endif

	// available to c and c++ code
	const char* sass2scss (const char* sass, const int pretty);

#ifdef __cplusplus
}
#endif
