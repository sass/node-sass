#define SASS_ERROR_HANDLING

namespace Sass {

  struct Error {
    enum Type { read, write, syntax, evaluation };

    Type type;
    string path;
    size_t line;
    string message;

    Error(Type type, string path, size_t line, string message)
    : type(type), path(path), line(line), message(message)
    { }

  };

  void error(string msg, string path, size_t line)
  {
    throw Error(Error::syntax, path, line, msg);
  }

}