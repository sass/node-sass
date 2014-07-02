// include library
#include <stack>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <stdlib.h>

#include "sass2scss.h"

// using std::string
using namespace std;

// main program
int main (int argc, char** argv)
{

	// pretty print option
	// set by command line
	int pretty = 0;

	// bitwise options
	// set by command line
	int options = 0;

	// process command line arguments
	for (int i = 1; i < argc; ++i)
	{
		// handle prettyfying option
		if ("-p" == string(argv[i])) pretty ++;
		else if ("--pretty" == string(argv[i])) pretty ++;
		// handle other bitwise options (comment related)
		else if ("-k" == string(argv[i])) options |= SASS2SCSS_KEEP_COMMENT;
		else if ("--keep" == string(argv[i])) options |= SASS2SCSS_KEEP_COMMENT;
		else if ("-s" == string(argv[i])) options |= SASS2SCSS_STRIP_COMMENT;
		else if ("--strip" == string(argv[i])) options |= SASS2SCSS_STRIP_COMMENT;
		else if ("-c" == string(argv[i])) options |= SASS2SCSS_CONVERT_COMMENT;
		else if ("--convert" == string(argv[i])) options |= SASS2SCSS_CONVERT_COMMENT;
		// error out with a generic error message (keep it simple for now)
		else { cerr << "Unknown command line option " << argv[i] << endl; exit(EXIT_FAILURE); }
	}

	// declare variable
	string line;

	// accumulate for test
	string sass = "";

	// read from stdin
	while (cin)
	{
		// read a line
		getline(cin, line);
		// process the line
		sass += line + "\n";
		// break if at end of file
		if (cin.eof()) break;
	};

	// simple assertion for valid value
	if (pretty > SASS2SCSS_PRETTIFY_3)
	{ pretty = SASS2SCSS_PRETTIFY_3; }

	// print the resulting scss
	cout << Sass::sass2scss (sass, pretty | options);

}
