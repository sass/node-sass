#include <string>

namespace Sass {
  using namespace std;
  
  class Node {

  private:
    Node_Impl* ip_;
    
  public:
    bool has_children();
    bool has_statements();
    bool has_blocks();
    bool has_expansions();
    bool has_backref();
    bool from_variable();
    bool eval_me();
    bool is_unquoted();
    bool is_numeric();
    
    string file_name() const;
    size_t line_number() const;
    size_t size() const;
    
    Node_Impl& at(size_t i) const;
    Node_Impl& operator[](size_t i) const;
    Node_Impl& push_back(Node n);
    Node_Impl& operator<<(Node n);
  };
}