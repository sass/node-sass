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
		string arg = string(argv[i]);

		// handle prettyfying option
		if (arg == "-p" || arg == "--pretty")
		{
			pretty ++;
		}

		// handle other bitwise options (comment related)
		else if (arg == "-k" || arg == "--keep")
		{
			options |= SASS2SCSS_KEEP_COMMENT;
		}
		else if (arg == "-s" || arg == "--strip")
		{
			options |= SASS2SCSS_STRIP_COMMENT;
		}
		else if (arg == "-c" || arg == "--convert")
		{
			options |= SASS2SCSS_CONVERT_COMMENT;
		}

		// Items for printing output and exit
		else if (arg == "-h" || arg == "--help")
		{
			cout << "sass2scss [options] < file.sass" << endl;
			cout << "---------" << endl;
			cout << "-p, --pretty       pretty print output" << endl;
			cout << "-c, --convert      convert src comments" << endl;
			cout << "-s, --strip        strip all comments" << endl;
			cout << "-k, --keep         keep all comments" << endl;
			cout << "-h, --help         help text" << endl;
			cout << "-v, --version      version information" << endl;
			exit(0);
		}
		else if (arg == "-v" || arg == "--version")
		{
			// check current hash and if it matches any of the tags, use tag
			cout << Sass::SASS2SCSS_VERSION << endl;
			exit(0);
		}

		// error out with a generic error message (keep it simple for now)
		else
		{
			cerr << "Unknown command line option " << argv[i] << endl;
			exit(EXIT_FAILURE);
		}
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
