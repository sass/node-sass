// include library
#include <stack>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

#include "sass2scss.h"

// using std::string
using namespace std;

// main program
int main (int argc, char** argv)
{

	// pretty print option
	// set by command line
	int pretty = 0;

	// process command line arguments
	for (int i = 0; i < argc; ++i)
	{
		// only handle prettyfying option
		if ("-p" == string(argv[i])) pretty ++;
		if ("--pretty" == string(argv[i])) pretty ++;
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

	// print the resulting scss
	cout << Sass::sass2scss (sass, pretty);

}
