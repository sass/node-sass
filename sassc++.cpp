#include <cstdlib>
#include <string>
#include <iostream>
#include "context.hpp"

#ifndef SASS_ERROR_HANDLING
#include "error_handling.hpp"
#endif

using namespace std;

int main(int argc, char** argv)
{
	if (argc < 2) {
		cout << "Please specify a file on the command line." << endl;
		return 1;
	}

	string file_name(argv[1]);

	try {
		Sass::Context ctx(
		  Sass::Context::Data().entry_point(file_name)
		                       .output_style(Sass::FORMATTED)
		);
		char* result = ctx.compile_file();
		if (result) {
			cout << result;
			free(result);
		}
	}
	catch (Sass::Error& e) {
		cout << e.path << ":" << e.position.line << ": " << e.message << endl;
	}
	catch (string& msg) {
		cout << msg << endl;
	}

	return 0;
}