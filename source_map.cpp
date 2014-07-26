#include "source_map.hpp"

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

  // taken from http://stackoverflow.com/a/7725289/1550314
  std::string encodeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
        switch (*iter) {
            case '\\': ss << "\\\\"; break;
            case '"': ss << "\\\""; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            // is a legal escape in JSON
            case '/': ss << "\\/"; break;
            default: ss << *iter; break;
        }
    }

    return ss.str();
  }

  string SourceMap::generate_source_map() {
    string result = "{\n";
    result += "  \"version\": 3,\n";
    result += "  \"file\": \"" + encodeJsonString(file) + "\",\n";
    result += "  \"sources\": [";
    for (size_t i = 0; i < files.size(); ++i) {
      result+="\"" + encodeJsonString(files[i]) + "\",";
    }
    if (!files.empty()) result.erase(result.length() - 1);
    result += "],\n";
    result += "  \"names\": [],\n";
    result += "  \"mappings\": \"" + serialize_mappings() + "\"\n";
    result += "}";

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
        while (generated_line != previous_generated_line) {
          result += ";";
          previous_generated_line += 1;
        }
      }
      else {
        if (i > 0) result += ",";
      }

      // generated column
      result += base64vlq.encode(generated_column - previous_generated_column);
      previous_generated_column = generated_column;
      // file
      result += base64vlq.encode(original_file - previous_original_file);
      previous_original_file = original_file;
      // source line
      result += base64vlq.encode(original_line - previous_original_line);
      previous_original_line = original_line;
      // source column
      result += base64vlq.encode(original_column - previous_original_column);
      previous_original_column = original_column;
    }

    return result;
  }

  void SourceMap::remove_line()
  {
    current_position.line -= 1;
    current_position.column = 1;
  }

  void SourceMap::update_column(const string& str)
  {
    const ptrdiff_t new_line_count = std::count(str.begin(), str.end(), '\n');
    current_position.line += new_line_count;
    if (new_line_count >= 1) {
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
