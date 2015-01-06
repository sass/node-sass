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
    bool seen_utf8;
    virtual void fallback_impl(AST_Node* n) = 0;

  public:
    Output(Context* ctx = 0)
    : ctx(ctx),
      buffer(""),
      top_imports(0),
      top_comments(0),
      seen_utf8(false)
    { }
    virtual ~Output() { };

    string get_buffer(void)
    {
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

      return comments.get_buffer()
           + imports.get_buffer()
           + buffer;
    }

    // append some text or token to the buffer
    // it is the main function for source-mapping
    void append_to_buffer(const string& data)
    {
      // search for unicode char
      for(const char& chr : data) {
        // abort clause
        if (seen_utf8) break;
        // skip all normal ascii chars
        if (chr >= 0) continue;
        // prepend charset to buffer
        buffer = "@charset \"UTF-8\";\n" + buffer;
        // singleton
        seen_utf8 = true;
      }
      // add to buffer
      buffer += data;
      // account for data in source-maps
      ctx->source_map.update_column(data);
    }

    void append_to_buffer(const string& data, AST_Node* node)
    {
      ctx->source_map.add_open_mapping(node);
      append_to_buffer(data);
      ctx->source_map.add_close_mapping(node);
    }

  };

}

#endif
