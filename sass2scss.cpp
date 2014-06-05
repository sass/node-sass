// include library
#include <stack>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

///*
//
// src comments: comments in sass syntax (staring with //)
// css comments: multiline comments in css syntax (starting with /*)
//
// KEEP_COMMENT: keep src comments in the resulting css code
// STRIP_COMMENT: strip out all comments (either src or css)
// CONVERT_COMMENT: convert all src comments to css comments
//
//*/

// our own header
#include "sass2scss.h"

// using std::string
using namespace std;

// add namespace for c++
namespace Sass
{

	// return the actual prettify value from options
	#define PRETTIFY(converter) (converter.options - (converter.options & 248))
	// query the options integer to check if the option is enables
	#define KEEP_COMMENT(converter) ((converter.options & SASS2SCSS_KEEP_COMMENT) == SASS2SCSS_KEEP_COMMENT)
	#define STRIP_COMMENT(converter) ((converter.options & SASS2SCSS_STRIP_COMMENT) == SASS2SCSS_STRIP_COMMENT)
	#define CONVERT_COMMENT(converter) ((converter.options & SASS2SCSS_CONVERT_COMMENT) == SASS2SCSS_CONVERT_COMMENT)

	// some makros to access the indentation stack
	#define INDENT(converter) (converter.indents.top())

	// some makros to query comment parser status
	#define IS_PARSING(converter) (converter.comment == "")
	#define IS_COMMENT(converter) (converter.comment != "")
	#define IS_SRC_COMMENT(converter) (converter.comment == "//" && ! CONVERT_COMMENT(converter))
	#define IS_CSS_COMMENT(converter) (converter.comment == "/*" || (converter.comment == "//" && CONVERT_COMMENT(converter)))

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

	// flush whitespace and print additional text, but
	// only print additional chars and buffer whitespace
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
		size_t pos_right = sass.find_last_not_of("\n\r");
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
			size_t pos_left = sass.find_first_not_of(" \t\n\v\f\r");
			if (pos_left != string::npos) sass = sass.substr(pos_left);
		}

		// add flushed data
		scss += sass;

		// return string
		return scss;

	}
	// EO flush

	// process a line of the sass text
	string process (string& sass, converter& converter)
	{

		// resulting string
		string scss = "";

		// get postion of first meaningfull character in string
		size_t pos_left = sass.find_first_not_of(" \t\n\v\f\r");

		// special case for final run
		if (converter.end_of_file) pos_left = 0;

		// maybe has only whitespace
		if (pos_left == string::npos)
		{
			// just add complete whitespace
			converter.whitespace += sass + "\n";
		}
		// have meaningfull first char
		else
		{

			// extract and store indentation string
			string indent = sass.substr(0, pos_left);

			// line has less or same indentation
			// finalize previous open parser context
			if (indent.length() <= INDENT(converter).length())
			{

				// close multilinie comment
				if (IS_CSS_COMMENT(converter))
				{
					// check if comments will be stripped anyway
					if (!STRIP_COMMENT(converter)) scss += " */";
				}
				// close src comment comment
				else if (IS_SRC_COMMENT(converter))
				{
					// add a newline to avoid closer on same line
					// this would but the bracket in the comment node
					if (KEEP_COMMENT(converter)) scss += "\n";
				}
				// close css properties
				else if (converter.property)
				{
					// add semicolon unless in concat mode
					if (!converter.comma) scss += ";";
				}

				// reset comment state
				converter.comment = "";

			}

			// make sure we close every "higher" block
			while (indent.length() < INDENT(converter).length())
			{
				// pop stacked context
				converter.indents.pop();
				// print close bracket
				if (IS_PARSING(converter))
				{ scss += closer(converter); }
				else { scss += " */"; }
				// reset comment state
				converter.comment = "";
			}

			// check if we have sass property syntax
			if (sass.substr(pos_left, 1) == ":")
			{
				// get postion of first meaningfull char
				size_t pos_value = sass.find_first_of(" \t\n\v\f\r", pos_left);
				// create new string by interchanging the colon sign for property and value
				sass = indent + sass.substr(pos_left + 1, pos_value - pos_left - 1) + ":" + sass.substr(pos_value);
			}

			// replace some specific sass shorthand directives
			else if (sass.substr(pos_left, 1) == "=") { sass = indent + "@mixin " + sass.substr(pos_left + 1); }
			else if (sass.substr(pos_left, 1) == "+") { sass = indent + "@include " + sass.substr(pos_left + 1); }

			// add quotes for import if needed
			else if (sass.substr(pos_left, 7) == "@import")
			{
				// get positions for the actual import url
				size_t pos_import = sass.find_first_of(" \t\n\v\f\r", pos_left + 7);
				size_t pos_quote = sass.find_first_not_of(" \t\n\v\f\r", pos_import);
				// check if the url is quoted
				if (sass.substr(pos_quote, 1) != "\"")
				{
					// get position of the last char on the line
					size_t pos_end = sass.find_last_not_of(" \t\n\v\f\r");
					// add quotes around the full line after the import statement
					sass = sass.substr(0, pos_quote) + "\"" + sass.substr(pos_quote, pos_end - pos_quote + 1) + "\"";
				}
			}

			// check if current line starts a comment
			string open = sass.substr(pos_left, 2);

			// current line has more indentation
			if (indent.length() >= INDENT(converter).length())
			{
				// not in comment mode
				if (IS_PARSING(converter))
				{
					// probably a property
					converter.property = true;
				}
			}
			// current line has more indentation
			if (indent.length() > INDENT(converter).length())
			{
				// not in comment mode
				if (IS_PARSING(converter))
				{
					// print block opener
					scss += opener(converter);
					// push new stack context
					converter.indents.push("");
					// store block indentation
					INDENT(converter) = indent;
				}
				// is and will be a src comment
				else if (!IS_CSS_COMMENT(converter))
				{
					// scss does not allow multiline src comments
					// therefore add forward slashes to all lines
					sass[INDENT(converter).length()+0] = '/';
					// there is an edge case here if indentation
					// is minimal (will overwrite the fist char)
					sass[INDENT(converter).length()+1] = '/';
					// could code around that, but I dont' think
					// this will ever be the cause for any trouble
				}
			}

			// line is opening a new comment
			if (open == "/*" || open == "//")
			{
				// reset the property state
				converter.property = false;
				// close previous comment
				if (IS_CSS_COMMENT(converter) && open != "")
				{
					if (!STRIP_COMMENT(converter) && !CONVERT_COMMENT(converter)) scss += " */";
				}
				// force single line comments
				// into a correct css comment
				if (CONVERT_COMMENT(converter))
				{
					if (IS_PARSING(converter))
					{ sass[pos_left + 1] = '*'; }
				}
				// set comment flag
				converter.comment = open;


			}

			// flush data only under certain conditions
			if (!(
				// strip css and src comments if option is set
				(IS_COMMENT(converter) && STRIP_COMMENT(converter)) ||
				// strip src comment even if strip option is not set
				// but only if the keep src comment option is not set
				(IS_SRC_COMMENT(converter) && ! KEEP_COMMENT(converter))
			))
			{
				// flush data and buffer whitespace
				scss += flush(sass, converter);
			}

			// get postion of last meaningfull char
			size_t pos_right = sass.find_last_not_of(" \t\n\v\f\r");

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
					// update parser status for expicitly closed comment
					if (close == "*/") converter.comment = "";

				}

			}
			// EO have meaningfull chars from end

		}
		// EO have meaningfull chars from start

		// return scss
		return scss;

	}
	// EO process

	// the main converter function for c++
	char* sass2scss (const string sass, const int options)
	{

		// local variables
		string line;
		string scss = "";
		const char delim = '\n';
		stringstream stream(sass);

		// create converter variable
		converter converter;
		// initialise all options
		converter.comma = false;
		converter.property = false;
		converter.end_of_file = false;
		converter.comment = "";
		converter.whitespace = "";
		converter.indents.push("");
		converter.options = options;

		// read line by line and process them
		while(std::getline(stream, line, delim))
		{ scss += process(line, converter); }

		// create mutable string
		string closer = "";
		// set the end of file flag
		converter.end_of_file = true;
		// process to close all open blocks
		scss += process(closer, converter);

		// allocate new memory on the heap
		// caller has to free it after use
		char * cstr = new char [scss.length()+1];
		// create a copy of the string
		strcpy (cstr, scss.c_str());
		// return pointer
		return &cstr[0];

	}
	// EO sass2scss

}
// EO namespace

// implement for c
extern "C"
{

	char* sass2scss (const char* sass, const int options)
	{
		return Sass::sass2scss(sass, options);
	}

}
