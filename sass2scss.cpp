// include library
#include <sstream>
#include <iostream>

// using std::string
using namespace std;

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
	string indents[255];
};

// helper function
static string closer (converter& converter)
{
	return converter.pretty > 0 ? "\n" + converter.indents[converter.level] + "}" : " }";
}

// helper function
static string opener (converter& converter)
{
	return converter.pretty > 1 ? "\n" + converter.indents[converter.level] + "{" : " {";
}

// flush whitespace and
// print additional text
// but only print additional
// chars and buffer whitespace
// ***************************************************************************************
string flush (string& str, converter& converter)
{
	string scss = "";
	// print whitespace
	scss += converter.whitespace;
	// reset whitespace
	converter.whitespace = "";
	// remove newlines
	int pos = str.find_last_not_of("\n\r");
	string lfs = str.substr(pos + 1);
	str = str.substr(0, pos + 1);
	// getline discharged newline
	converter.whitespace += lfs + "\n";
	// print text
	scss += str;
	// return string
	return scss;
}

// process a line of the sass text
string process (string& sass, converter& converter, bool final = false)
{

	string scss = "";

	// get postion of first meaningfull char
	int pos_left = sass.find_first_not_of(" \t\n\v\f\r");

	// special case for final run
	if (final) pos_left = 0;

	// has only whitespace
	if (pos_left == string::npos)
	{
		// add whitespace
		converter.whitespace += sass;
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
			scss += " */";
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
			// open new block if comment is opening
			// be smart and only require the same indentation
			// level as the comment node itself, plus one char
			if (converter.comment == "/*" && converter.comment == "//")
			{
				// only increase indentation minimaly
				// this will accept everything that is more
				// indented than the opening comment line
				converter.indents[converter.level] = converter.indents[converter.level] + ' ';
				// count new block
				converter.level ++;
			}
			// set comment to current indent
			// multiline comments must be indented
			// indicates multiline if not eq "*"
			if (converter.comment != "") converter.comment = indent;
		}

		// line is opening a new comment
		if (open == "/*" || open == "//")
		{
			// force single line comments
			// into a correct css comment
			sass[pos_left + 1] = '*';
			// close previous comment
			if (converter.comment != "") scss += " */";;
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
					converter.indents[converter.level] == "";
					// close block
					converter.level --;
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

string sass2scss (string sass, int pretty)
{

	string out = "";
	char delims = '\n';
	stringstream stream(sass);
	string line;

	converter converter;
	converter.pretty = pretty;
	converter.level = 0;
	converter.whitespace = "";
	converter.indents[0] = "";

	while(std::getline(stream, line, '\n')){
		out += process(line, converter);
	}

	string end = "";
	out += process(end, converter, true);

	return out;

}

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
	cout << sass2scss (sass, pretty);

}
