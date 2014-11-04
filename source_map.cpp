#include "source_map.hpp"
#include "json.hpp"

#ifndef SASS_CONTEXT
#include "context.hpp"
#endif

#include <string>
#include <sstream>
#include <cstddef>
#include <iomanip>

namespace Sass {
  using std::ptrdiff_t;
  SourceMap::SourceMap(const string& file) : current_position(Position(1, 1)), file(file) { }

  string SourceMap::generate_source_map(Context &ctx) {

    const bool include_sources = ctx.source_map_contents;
    const vector<string> includes = ctx.include_links;
    const vector<const char*> sources = ctx.sources;

    JsonNode *json_srcmap = json_mkobject();

    json_append_member(json_srcmap, "version", json_mknumber(3));

    const char *include = file.c_str();
    JsonNode *json_include = json_mkstring(include);
    json_append_member(json_srcmap, "file", json_include);

    JsonNode *json_includes = json_mkarray();
    for (size_t i = 0; i < source_index.size(); ++i) {
      const char *include = includes[source_index[i]].c_str();
      JsonNode *json_include = json_mkstring(include);
      json_append_element(json_includes, json_include);
    }
    json_append_member(json_srcmap, "sources", json_includes);

    JsonNode *json_contents = json_mkarray();
    if (include_sources) {
      for (size_t i = 0; i < source_index.size(); ++i) {
        const char *content = sources[source_index[i]];
        JsonNode *json_content = json_mkstring(content);
        json_append_element(json_contents, json_content);
      }
    }
    json_append_member(json_srcmap, "sourcesContent", json_contents);

    string mappings = serialize_mappings();
    JsonNode *json_mappings = json_mkstring(mappings.c_str());
    json_append_member(json_srcmap, "mappings", json_mappings);

    JsonNode *json_names = json_mkarray();
    // so far we have no implementation for names
    // no problem as we do not alter any identifiers
    json_append_member(json_srcmap, "names", json_names);

    char *str = json_stringify(json_srcmap, "\t");
    string result = string(str);
    free(str);
    json_delete(json_srcmap);
    return result;
  }

  string SourceMap::serialize_mappings() {
    string result = "";

    size_t previous_generated_line = 0;
    size_t previous_generated_column = 0;
    size_t previous_original_line = 0;
    size_t previous_original_column = 0;
    size_t previous_original_file = 0;
    for (size_t i = 0; i < mappings.size(); ++i) {
      const size_t generated_line = mappings[i].generated_position.line - 1;
      const size_t generated_column = mappings[i].generated_position.column - 1;
      const size_t original_line = mappings[i].original_position.line - 1;
      const size_t original_column = mappings[i].original_position.column - 1;
      const size_t original_file = mappings[i].original_position.file - 1;

      if (generated_line != previous_generated_line) {
        previous_generated_column = 0;
        if (generated_line > previous_generated_line) {
          result += std::string(generated_line - previous_generated_line, ';');
          previous_generated_line = generated_line;
        }
      }
      else if (i > 0) {
        result += ",";
      }

      // generated column
      result += base64vlq.encode(static_cast<int>(generated_column) - static_cast<int>(previous_generated_column));
      previous_generated_column = generated_column;
      // file
      result += base64vlq.encode(static_cast<int>(original_file) - static_cast<int>(previous_original_file));
      previous_original_file = original_file;
      // source line
      result += base64vlq.encode(static_cast<int>(original_line) - static_cast<int>(previous_original_line));
      previous_original_line = original_line;
      // source column
      result += base64vlq.encode(static_cast<int>(original_column) - static_cast<int>(previous_original_column));
      previous_original_column = original_column;
    }

    return result;
  }

  void SourceMap::remove_line()
  {
    // prevent removing non existing lines
    if (current_position.line > 1) {
      current_position.line -= 1;
      current_position.column = 1;
    }
  }

  void SourceMap::update_column(const string& str)
  {
    const ptrdiff_t new_line_count = std::count(str.begin(), str.end(), '\n');
    current_position.line += new_line_count;
    if (new_line_count > 0) {
      current_position.column = str.size() - str.find_last_of('\n');
    } else {
      current_position.column += str.size();
    }
  }

  void SourceMap::add_mapping(AST_Node* node)
  {
    mappings.push_back(Mapping(node->position(), current_position));
  }

}
