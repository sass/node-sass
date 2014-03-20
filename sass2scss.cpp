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

	// return the actual prettify value from options
	#define PRETTIFY(converter) (converter.options - (converter.options & 248))
	// query the options integer to check if the option is enables
	#define KEEP_COMMENT(converter) ((converter.options & SASS2SCSS_KEEP_COMMENT) == SASS2SCSS_KEEP_COMMENT)
	#define STRIP_COMMENT(converter) ((converter.options & SASS2SCSS_STRIP_COMMENT) == SASS2SCSS_STRIP_COMMENT)
	#define CONVERT_COMMENT(converter) ((converter.options & SASS2SCSS_CONVERT_COMMENT) == SASS2SCSS_CONVERT_COMMENT)

	// some makros to access the indentation stack
	#define INDENT(converter) (converter.indents[converter.level])

	// some makros to query comment parser status
	#define IS_PARSING(converter) (converter.comment == "")
	#define IS_COMMENT(converter) (converter.comment != "")
	#define IS_ONELINE(converter) (converter.comment == "//")
	#define IS_MULTILINE(converter) (converter.comment == "/*")

	// pretty printer helper function
	static string closer (const converter& converter)
	{
		return PRETTIFY(converter) == 0 ? " }" :
		     PRETTIFY(converter) <= 1 ? " }" :
		       "\n" + INDENT(converter) + "}";
	}

	// pretty printer helper function
	static string opener (const converter& converter)
	{
		return PRETTIFY(converter) == 0 ? " { " :
		     PRETTIFY(converter) <= 2 ? " {" :
		       "\n" + INDENT(converter) + "{";
	}

	// flush whitespace and
	// print additional text
	// but only print additional
	// chars and buffer whitespace
	// ***************************************************************************************
	string flush (string& sass, converter& converter)
	{

		// return flushed
		string scss = "";

		// print whitespace buffer
		scss += PRETTIFY(converter) > 0 ?
		        converter.whitespace : "";
		// reset whitespace buffer
		converter.whitespace = "";

		// remove possible newlines from string
		int pos_right = sass.find_last_not_of("\n\r");
		if (pos_right == string::npos) return scss;

		// get the linefeeds from the string
		string lfs = sass.substr(pos_right + 1);
		sass = sass.substr(0, pos_right + 1);

		// add newline as getline discharged it
		converter.whitespace += lfs + "\n";

		// maybe remove any leading whitespace
		if (PRETTIFY(converter) == 0)
		{
			// remove leading whitespace and update string
			int pos_left = sass.find_first_not_of(" \t\n\v\f\r");
			if (pos_left != string::npos) sass = sass.substr(pos_left);
		}

		// add flushed data
		scss += sass;

		// return string
		return scss;

	}
	// EO flush

	// process a line of the sass text
	string process (string& sass, converter& converter, const bool final = false)
	{

		// resulting string
		string scss = "";

		// get postion of first meaningfull character in string
		int pos_left = sass.find_first_not_of(" \t\n\v\f\r");

		// special case for final run
		if (final) pos_left = 0;

		// maybe has only whitespace
		if (pos_left == string::npos)
		{
			// just add complete whitespace
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
			if (IS_COMMENT(converter) && indent.length() <= INDENT(converter).length())
			{
				// close open comments in data stream
				if (IS_MULTILINE(converter)) scss += " */";
				else if (IS_ONELINE(converter) && CONVERT_COMMENT(converter)) scss += " */";
				else if (IS_ONELINE(converter))
				{
					// add a newline to avoid closers on same line
					if (KEEP_COMMENT(converter)) scss += "\n";
				}
				// close comment mode
				converter.comment = "";
			}

			// line has less or same indentation (css property?)
			else if (indent.length() <= INDENT(converter).length())
			{
				// prevent on root level
				if (converter.level > 0)
				{
					// add semicolon if not in comment and not in concat mode
					if (IS_PARSING(converter) && converter.comma == false) scss += ";";
				}
			}

			// make sure we close every "higher" block
			while (indent.length() < INDENT(converter).length())
			{
				// reset string
				INDENT(converter) == "";
				// close block
				converter.level --;
				// print closer
				if (IS_PARSING(converter))
				{ scss += closer(converter); }
				else { scss += " */"; }
				// close comment mode
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
			if (indent.length() > INDENT(converter).length())
			{
				// not in comment mode
				if (IS_PARSING(converter))
				{
					// print block opener
					scss += opener(converter);
					// open new block
					converter.level ++;
					// store block indentation
					INDENT(converter) = indent;
				}
				else if (IS_ONELINE(converter) && !CONVERT_COMMENT(converter))
				{
					sass[INDENT(converter).length()+0] = '/';
					// there is an edge case here if indentation
					// is minimal (will overwrite the fist char)
					sass[INDENT(converter).length()+1] = '/';
				}
			}

			// line is opening a new comment
			if (open == "/*" || open == "//")
			{
				// force single line comments
				// into a correct css comment
				if (CONVERT_COMMENT(converter))
				{
					if (IS_PARSING(converter))
					{ sass[pos_left + 1] = '*'; }
				}
				// close previous comment
				if (IS_MULTILINE(converter))
				{
					if (!STRIP_COMMENT(converter)) scss += " */";
				}
				// remove indentation from previous comment
				if (IS_ONELINE(converter))
				{
					// reset string
					INDENT(converter) == "";
					// close block
					// converter.level --;
				}
				// set comment flag
				converter.comment = open;
			}

			// flush line
			if (!IS_ONELINE(converter) || IS_ONELINE(converter) && CONVERT_COMMENT(converter) || KEEP_COMMENT(converter))
				if (!(STRIP_COMMENT(converter) && !IS_PARSING(converter))) scss += flush(sass, converter);

			// get postion of last meaningfull char
			int pos_right = sass.find_last_not_of(" \t\n\v\f\r");

			// check for invalid result
			if (pos_right != string::npos)
			{

				// get the last meaningfull char
				string close = sass.substr(pos_right, 1);

				// check if next line should be concatenated (list mode)
				converter.comma = IS_PARSING(converter) && close == ",";

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
						// INDENT(converter) == "";
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
	const string sass2scss (const string sass, const int options)
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
		converter.whitespace = "";
		converter.indents[0] = "";
		converter.options = options;

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

	const char* sass2scss (const char* sass, const int options)
	{
		return ocbnet::sass2scss(sass, options).c_str();
	}

}
