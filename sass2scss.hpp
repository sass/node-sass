// include library
#include <sstream>
#include <iostream>

// using std::string
using namespace std;

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
	// indent strings
	// length should be dynamic
	// but my c++ is very rusty
	string indents[maxnested];
};

// main function to be used by others
string sass2scss (string sass, int pretty);
