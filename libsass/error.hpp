namespace Sass {
  
  struct Error {
    enum Type { read, write, syntax, evaluation };
    
    Type type;
    size_t line_number;
    string file_name;
    string message;
    
    Error(Type type, size_t line_number, string file_name, string message)
    : type(type), line_number(line_number), file_name(file_name), message(message)
    { }

  };
  
}