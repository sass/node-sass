#ifndef SASS_OUTPUT_H
#define SASS_OUTPUT_H

#include <string>
#include <vector>

#include "util.hpp"
#include "context.hpp"
#include "operation.hpp"

namespace Sass {
  using namespace std;
  struct Context;

  template<typename T>
  class Output : public Operation_CRTP<void, T> {
    // import class-specific methods and override as desired
    using Operation_CRTP<void, T>::operator();

  protected:
    Context* ctx;
    string buffer;
    vector<Import*> top_imports;
    vector<Comment*> top_comments;
    virtual void fallback_impl(AST_Node* n) = 0;

  public:
    Output(Context* ctx = 0)
    : ctx(ctx),
      buffer(""),
      top_imports(0),
      top_comments(0)
    { }
    virtual ~Output() { };

    // return buffer as string
    string get_buffer(void);

    // append some text or token to the buffer
    void append_to_buffer(const string& data);

    // append some text or token to the buffer
    // this adds source-mappings for node start and end
    void append_to_buffer(const string& data, AST_Node* node);

  };

}

#endif
