namespace Sass {
  
  struct Error {
    enum Type { read, write, syntax, evaluation };
    
    size_t line_number;
    string file_name;
    string message;
    
    Error(size_t line_number, string file_name, string message)
    : line_number(line_number), file_name(file_name), message(message)
    { }

  };
  
}