// include library
#include <sstream>
#include <iostream>

// our own header
#include "sass2scss.h"

// using std::string
using namespace std;

// add namespace for c++
namespace ocbnet
{

	// helper function
	static string closer (const converter& converter)
	{
		return converter.pretty == 0 ? " }" : converter.pretty <= 1 ? " }"
		       : "\n" + converter.indents[converter.level] + "}";
	}

	// helper function
	static string opener (const converter& converter)
	{
		return converter.pretty == 0 ? " { " : converter.pretty <= 2 ? " {"
		       : "\n" + converter.indents[converter.level] + "{";
	}

	// flush whitespace and
	// print additional text
	// but only print additional
	// chars and buffer whitespace
	// ***************************************************************************************
	string flush (string& sass, converter& converter)
	{
		string scss = "";
		// print whitespace
		scss += converter.pretty > 0 ?
		        converter.whitespace : "";
		// reset whitespace
		converter.whitespace = "";
		// remove newlines
		int pos_right = sass.find_last_not_of("\n\r");
		if (pos_right == string::npos) return scss;

		string lfs = sass.substr(pos_right + 1);
		sass = sass.substr(0, pos_right + 1);
		// getline discharged newline
		converter.whitespace += lfs + "\n";
		// remove all whitespace?
		if (converter.pretty == 0)
		{
			int pos_left = sass.find_first_not_of(" \t\n\v\f\r");
			if (pos_left != string::npos) sass = sass.substr(pos_left);
		}
		// print text
		scss += sass;
		// return string
		return scss;
	}

	// process a line of the sass text
	string process (string& sass, converter& converter, const bool final = false)
	{

		// resulting string
		string scss = "";

		// get postion of first meaningfull char
		int pos_left = sass.find_first_not_of(" \t\n\v\f\r");

		// special case for final run
		if (final) pos_left = 0;

		// has only whitespace
		if (pos_left == string::npos)
		{
			// add whitespace
			converter.whitespace += sass + "\n";
		}
		// have meaningfull first char
		else
		{

			// store indentation string
			string indent = sass.substr(0, pos_left);

			// special case for multiline comment, when the next
			// line is on the same indentation as the actual comment
			// I assume that this means we should close the comment node
			if (converter.comment != "" && indent.length() <= converter.indents[converter.level].length())
			{
				// close comment
				if (converter.comment == "/*") scss += " */";
				else if (converter.comment == "//")
				{ if (!converter.comment_strip) scss += "\n"; }
				// unset flag
				converter.comment = "";
			}
			// current line has less or same indentation
			else if (converter.level > 0 && indent.length() <= converter.indents[converter.level].length())
			{
				// add semicolon if not in concat mode
				if (converter.comment == "" && converter.comma == false) scss += ";";
			}

			// make sure we close every "higher" block
			while (indent.length() < converter.indents[converter.level].length())
			{
				// reset string
				converter.indents[converter.level] == "";
				// close block
				converter.level --;
				// print closer
				if (converter.comment == "")
				{ scss += closer(converter); }
				else { scss += " */"; }
				// reset comment
				converter.comment = "";
			}

			// check if we have sass property syntax
			if (sass.substr(pos_left, 1) == ":")
			{
				// get postion of first meaningfull char
				int pos_value = sass.find_first_of(" \t\n\v\f\r", pos_left);
				// create new string by interchanging the colon sign for property and value
				sass = indent + sass.substr(pos_left + 1, pos_value - pos_left - 1) + ":" + sass.substr(pos_value);
			}

			// replace some specific sass shorthand directives
			else if (sass.substr(pos_left, 1) == "=") { sass = indent + "@mixin " + sass.substr(pos_left + 1); }
			else if (sass.substr(pos_left, 1) == "+") { sass = indent + "@include " + sass.substr(pos_left + 1); }
			// add quotes for import if needed
			else if (sass.substr(pos_left, 7) == "@import")
			{
				int pos_import = sass.find_first_of(" \t\n\v\f\r", pos_left + 7);
				int pos_quote = sass.find_first_not_of(" \t\n\v\f\r", pos_import);
				if (sass.substr(pos_quote, 1) != "\"")
				{
					int pos_end = sass.find_last_not_of(" \t\n\v\f\r");
					sass = sass.substr(0, pos_quote) + "\"" + sass.substr(pos_quote, pos_end - pos_quote + 1) + "\"";
				}
			}

			// check if current line starts a comment
			string open = sass.substr(pos_left, 2);

			// current line has more indentation
			if (indent.length() > converter.indents[converter.level].length())
			{
				// not in comment mode
				if (converter.comment == "")
				{
					// print block opener
					scss += opener(converter);
					// open new block
					converter.level ++;
					// store block indentation
					converter.indents[converter.level] = indent;
				}
				else if (converter.comment == "//")
				{
					sass[converter.indents[converter.level].length()+0] = '/';
					sass[converter.indents[converter.level].length()+1] = '/';
					// there is an edge case here (overwriting one char)
				}
			}

			// line is opening a new comment
			if (open == "/*" || open == "//")
			{
				// force single line comments
				// into a correct css comment
				if (converter.comment_convert)
				{ sass[pos_left + 1] = '*'; }
				// close previous comment
				if (converter.comment != "")
				{
					if (converter.comment != "") scss += " */";
				}
				// remove indentation from previous comment
				if (converter.comment == "//")
				{
					// reset string
					converter.indents[converter.level] == "";
					// close block
					converter.level --;
				}
				// set comment flag
				converter.comment = open;
			}

			// flush line
			if (converter.comment != "//" || !converter.comment_strip)
				scss += flush(sass, converter);

			// get postion of last meaningfull char
			int pos_right = sass.find_last_not_of(" \t\n\v\f\r");

			// check for invalid result
			if (pos_right != string::npos)
			{

				// get the last meaningfull char
				string close = sass.substr(pos_right, 1);

				// check if next line should be concatenated
				converter.comma = converter.comment == "" && close == ",";

				// check if we have more than
				// one meaningfull char
				if (pos_right > 0)
				{

					// get the last two chars from string
					string close = sass.substr(pos_right - 1, 2);
					// is comment node closed expicitly
					if (close == "*/")
					{
						// close implicit comment
						converter.comment = "";
						// reset string
						// converter.indents[converter.level] == "";
						// close block
						// converter.level --;
					}

				}
				// EO have at least two meaningfull chars from end

			}
			// EO have meaningfull chars from end

		}
		// EO have meaningfull chars from start

		// return scss
		return scss;

	}
	// EO process

	// the main converter function for c++
	const string sass2scss (const string sass, const int pretty)
	{

		// local variables
		string line;
		string scss = "";
		const char delim = '\n';
		stringstream stream(sass);

		// create converter variable
		converter converter;
		// initialise all options
		converter.level = 0;
		converter.pretty = pretty;
		converter.whitespace = "";
		converter.indents[0] = "";
		converter.comment_strip = true;
		converter.comment_convert = false;

		// read line by line and process them
		while(std::getline(stream, line, delim))
		{ scss += process(line, converter); }

		// create mutable string
		string closer = "";
		// process to close all open blocks
		scss += process(closer, converter, true);

		// return result
		return scss;

	}
	// EO sass2scss

}
// EO namespace

// implement for c
extern "C"
{

	const char* sass2scss (const char* sass, const int pretty)
	{
		return ocbnet::sass2scss(sass, pretty).c_str();
	}

}
