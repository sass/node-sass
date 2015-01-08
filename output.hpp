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

    string get_buffer(void)
    {
      string charset("");

      Inspect comments(ctx);
      size_t size_com = top_comments.size();
      for (size_t i = 0; i < size_com; i++) {
        top_comments[i]->perform(&comments);
        comments.append_to_buffer(ctx->linefeed);
      }

      Inspect imports(ctx);
      size_t size_imp = top_imports.size();
      for (size_t i = 0; i < size_imp; i++) {
        top_imports[i]->perform(&imports);
        imports.append_to_buffer(ctx->linefeed);
      }

      // create combined buffer string
      string buffer = comments.get_buffer()
                    + imports.get_buffer()
                    + this->buffer;

      // search for unicode char
      for(const char& chr : buffer) {
        // skip all ascii chars
        if (chr >= 0) continue;
        // declare the charset
        charset = "@charset \"UTF-8\";";
        // abort search
        break;
      }

      // add charset as the very first line, before top comments and imports
      return (charset.empty() ? "" : charset + ctx->linefeed) + buffer;
    }

    // append some text or token to the buffer
    void append_to_buffer(const string& data)
    {
      // add to buffer
      buffer += data;
      // account for data in source-maps
      ctx->source_map.update_column(data);
    }

    // append some text or token to the buffer
    // this adds source-mappings for node start and end
    void append_to_buffer(const string& data, AST_Node* node)
    {
      ctx->source_map.add_open_mapping(node);
      append_to_buffer(data);
      ctx->source_map.add_close_mapping(node);
    }

  };

}

#endif
