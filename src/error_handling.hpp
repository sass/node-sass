#define SASS_ERROR_HANDLING
#include <string>

namespace Sass {
	using namespace std;

  struct Error {
    enum Type { read, write, syntax, evaluation };

    Type type;
    string path;
    size_t line;
    string message;

    Error(Type type, string path, size_t line, string message);

  };

  void error(string msg, string path, size_t line);

}