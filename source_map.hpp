#ifndef SASS_SOURCE_MAP_H
#define SASS_SOURCE_MAP_H

#include <vector>

#include "ast_fwd_decl.hpp"
#include "base64vlq.hpp"
#include "mapping.hpp"

namespace Sass {
  using std::vector;

  struct Context;

  class SourceMap {

  public:
    vector<size_t> source_index;
    SourceMap();
    SourceMap(const string& file);

    void remove_line();
    void update_column(const string& str);
    void add_open_mapping(AST_Node* node);
    void add_close_mapping(AST_Node* node);

    string generate_source_map(Context &ctx);
    ParserState remap(const ParserState& pstate);

  private:

    string serialize_mappings();

    vector<Mapping> mappings;
    Position current_position;
    string file;
    Base64VLQ base64vlq;
  };

}

#endif
