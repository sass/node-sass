#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include "node.hpp"

using std::string;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;

namespace Sass {

  string Node::to_string(Type inside_of) const
  {
    switch (type())
    {
      case selector_group:
      case media_expression_group: { // really only needed for arg to :not
        string result(at(0).to_string());
        for (size_t i = 1, S = size(); i < S; ++i) {
          result += ", ";
          result += at(i).to_string();
        }
        return result;
      } break;

      case media_expression: {
        string result;
        if (at(0).type() == rule) {
          result += "(";
          result += at(0).to_string();
          result += ")";
        }
        else {
          result += at(0).to_string();
        }
        for (size_t i = 1, S = size(); i < S; ++i) {
          result += " ";
          if (at(i).type() == rule) {
            result += "(";
            result += at(i).to_string();
            result += ")";
          }
          else {
            result += at(i).to_string();
          }
        }
        return result;
      } break;
      
      case selector: {
        string result;
        
        result += at(0).to_string();
        for (size_t i = 1, S = size(); i < S; ++i) {
          result += " ";
          result += at(i).to_string();
        }
        return result;
      }  break;
      
      case selector_combinator: {
        return token().to_string();
      } break;
      
      case simple_selector_sequence: {
        string result;
        for (size_t i = 0, S = size(); i < S; ++i) {
          result += at(i).to_string();
        }
        return result;
      }  break;
      
      case pseudo:
      case simple_selector: {
        return token().to_string();
      } break;
      
      case pseudo_negation: {
        string result;
        result += at(0).to_string();
        result += at(1).to_string();
        result += ')';
        return result;
      } break;
      
      case functional_pseudo: {
        string result;
        result += at(0).to_string();
        for (size_t i = 1, S = size(); i < S; ++i) {
          result += at(i).to_string();
        }
        result += ')';
        return result;
      } break;
      
      case attribute_selector: {
        string result;
        result += "[";
        for (size_t i = 0, S = size(); i < S; ++i) {
          result += at(i).to_string();
        }
        result += ']';
        return result;
      } break;

      case rule: {
        string result(at(0).to_string(property));
        result += ": ";
        result += at(1).to_string();
        return result;
      } break;

      case comma_list: {
        string result(at(0).to_string());
        for (size_t i = 1, S = size(); i < S; ++i) {
          if (at(i).type() == nil) continue;
          result += ", ";
          result += at(i).to_string();
        }
        return result;
      } break;
      
      case space_list: {
        string result(at(0).to_string());
        for (size_t i = 1, S = size(); i < S; ++i) {
          if (at(i).type() == nil) continue;
          result += " ";
          result += at(i).to_string();
        }
        return result;
      } break;
      
      case expression:
      case term: {
        string result(at(0).to_string());
        for (size_t i = 1, S = size(); i < S; ++i) {
          if (at(i).type() != add && at(i).type() != mul) {
            result += at(i).to_string();
          }
        }
        return result;
      } break;
      
      //edge case
      case sub: {
        return "-";
      } break;
      
      case div: {
        return "/";
      } break;
      
      case css_import: {
        stringstream ss;
        ss << "@import url(";
        ss << at(0).to_string();
        ss << ")";
        return ss.str();
      }
      
      case function_call: {
        stringstream ss;
        ss << at(0).to_string();
        ss << "(";
        ss << at(1).to_string();
        ss << ")";
        return ss.str();
      }
      
      case arguments: {
        stringstream ss;
        size_t S = size();
        if (S > 0) {
          ss << at(0).to_string();
          for (size_t i = 1; i < S; ++i) {
            ss << ", ";
            ss << at(i).to_string();
          }
        }
        return ss.str();
      }
      
      case unary_plus: {
        stringstream ss;
        ss << "+";
        ss << at(0).to_string();
        return ss.str();
      }
      
      case unary_minus: {
        stringstream ss;
        ss << "-";
        ss << at(0).to_string();
        return ss.str();
      }
      
      case numeric_percentage: {
        stringstream ss;
        ss << numeric_value();
        ss << '%';
        return ss.str();
      }
      
      case numeric_dimension: {
        stringstream ss;
        ss << numeric_value() << unit().to_string();
        return ss.str();
      } break;
      
      case number: {
        stringstream ss;
        ss << numeric_value();
        return ss.str();
      } break;
      
      case numeric_color: {
        if (at(3).numeric_value() >= 1.0)
        {
          double a = at(0).numeric_value();
          double b = at(1).numeric_value();
          double c = at(2).numeric_value();
          if (a >= 0xff && b >= 0xff && c >= 0xff)
          { return "white"; }
          else if (a >= 0xff && b >= 0xff && c == 0)
          { return "yellow"; }
          else if (a == 0 && b >= 0xff && c >= 0xff)
          { return "aqua"; } 
          else if (a >= 0xff && b == 0 && c >= 0xff)
          { return "fuchsia"; }
          else if (a >= 0xff && b == 0 && c == 0)
          { return "red"; }
          else if (a == 0 && b >= 0xff && c == 0)
          { return "lime"; }
          else if (a == 0 && b == 0 && c >= 0xff)
          { return "blue"; }
          else if (a <= 0 && b <= 0 && c <= 0)
          { return "black"; }
          else
          {
            stringstream ss;
            ss << '#' << std::setw(2) << std::setfill('0') << std::hex;
            for (size_t i = 0; i < 3; ++i) {
              double x = at(i).numeric_value();
              if (x > 0xff) x = 0xff;
              else if (x < 0) x = 0;
              ss << std::hex << std::setw(2) << static_cast<unsigned long>(std::floor(x+0.5));
            }
            return ss.str();
          }
        }
        else
        {
          stringstream ss;
          ss << "rgba(" << static_cast<unsigned long>(at(0).numeric_value());
          for (size_t i = 1; i < 3; ++i) {
            ss << ", " << static_cast<unsigned long>(at(i).numeric_value());
          }
          ss << ", " << at(3).numeric_value() << ')';
          return ss.str();
        }
      } break;
      
      case uri: {
        string result("url(");
        result += token().to_string();
        result += ")";
        return result;
      } break;

      case expansion: {
        // ignore it
        return "";
      } break;
      
      case string_constant: {
        if (is_unquoted()) return token().unquote();
        else {
          string result(token().to_string());
          if (result[0] != '"' && result[0] != '\'') return "\"" + result + "\"";
          else                                       return result;
        }
      } break;
      
      case boolean: {
        if (boolean_value()) return "true";
        else return "false";
      } break;
      
      case important: {
        return "!important";
      } break;
      
      case value_schema:
      case identifier_schema: {
        string result;
        for (size_t i = 0, S = size(); i < S; ++i) {
          if (at(i).type() == string_constant) {
            result += at(i).token().unquote();
          }
          else {
            result += at(i).to_string(identifier_schema);
          }
        }
        return result;
      } break;
      
      case string_schema: {
        string result;
        for (size_t i = 0, S = size(); i < S; ++i) {
          string chunk(at(i).to_string());
          if (at(i).type() == string_constant) {
            result += chunk.substr(1, chunk.size()-2);
          }
          else {
            result += chunk;
          }
        }
        return result;
      } break;

      case concatenation: {
        string result;
        for (size_t i = 0, S = size(); i < S; ++i) {
          result += at(i).to_string().substr(1, at(i).token().length()-2);
        }
        if (inside_of == identifier_schema || inside_of == property) return result;
        else               return "\"" + result + "\"";
      } break;

      case warning: {
        string prefix("WARNING: ");
        string indent("         ");
        Node contents(at(0));
        string result(contents.to_string());
        if (contents.type() == string_constant || contents.type() == string_schema) {
          result = result.substr(1, result.size()-2); // unquote if it's a single string
        }
        // These cerrs aren't log lines! They're supposed to be here!
        cerr << prefix << result << endl;
        cerr << indent << "on line " << at(0).line() << " of " << at(0).path();
        cerr << endl << endl;
        return "";
      } break;
      
      default: {
        // return content.token.to_string();
        if (!has_children()) return token().to_string();
        else return "";
      } break;
    }
  }

  void Node::emit_nested_css(stringstream& buf, size_t depth, bool at_toplevel, bool in_media_query)
  {
    switch (type())
    {
      case root: {
        if (has_expansions()) flatten();
        for (size_t i = 0, S = size(); i < S; ++i) {
          at(i).emit_nested_css(buf, depth, true);
        }
      } break;

      case ruleset: {
        Node sel_group(at(2));
        Node block(at(1));

        if (block.has_expansions()) block.flatten();
        if (block.has_statements()) {
          buf << string(2*depth, ' ');
          buf << sel_group.to_string();
          buf << " {";
          for (size_t i = 0, S = block.size(); i < S; ++i) {
            Type stm_type = block[i].type();
            if (stm_type == comment || stm_type == rule || stm_type == css_import || stm_type == propset || stm_type == warning) {
              block[i].emit_nested_css(buf, depth+1);
            }
          }
          buf << " }";
          if (!in_media_query || (in_media_query && block.has_blocks())) buf << endl;
          ++depth; // if we printed content at this level, we need to indent any nested rulesets
        }
        if (block.has_blocks()) {
          for (size_t i = 0, S = block.size(); i < S; ++i) {
            if (block[i].type() == ruleset || block[i].type() == media_query) {
              block[i].emit_nested_css(buf, depth, false, in_media_query);
            }
          }
        }
        if (block.has_statements()) --depth; // see previous comment
        if ((depth == 0) && at_toplevel && !in_media_query) buf << endl;
      } break;

      case media_query: {
        buf << string(2*depth, ' ');
        buf << "@media " << at(0).to_string() << " {" << endl;
        at(1).emit_nested_css(buf, depth+1, false, true);
        buf << " }" << endl;
      } break;

      case propset: {
        emit_propset(buf, depth, "");
      } break;
        
      case rule: {
        buf << endl << string(2*depth, ' ');
        buf << to_string();
        // at(0).emit_nested_css(buf, depth); // property
        // at(1).emit_nested_css(buf, depth); // values
        buf << ";";
      } break;
        
      case css_import: {
        buf << string(2*depth, ' ');
        buf << to_string();
        buf << ";" << endl;
      } break;

      case property: {
        buf << token().to_string() << ": ";
      } break;

      case values: {
        for (size_t i = 0, S = size(); i < S; ++i) {
          buf << " " << at(i).token().to_string();
        }
      } break;

      case comment: {
        if (depth != 0) buf << endl;
        buf << string(2*depth, ' ') << token().to_string();
        if (depth == 0) buf << endl;
      } break;

      default: {
        buf << to_string();
      } break;
    }
  }
  
  void Node::emit_propset(stringstream& buf, size_t depth, const string& prefix)
  {
    string new_prefix(prefix);
    // bool has_prefix = false;
    if (new_prefix.empty()) {
      new_prefix += "\n";
      new_prefix += string(2*depth, ' ');
      new_prefix += at(0).token().to_string();
    }
    else {
      new_prefix += "-";
      new_prefix += at(0).token().to_string();
      // has_prefix = true;
    }
    Node rules(at(1));
    for (size_t i = 0, S = rules.size(); i < S; ++i) {
      if (rules[i].type() == propset) {
        rules[i].emit_propset(buf, depth+1, new_prefix);
      }
      else {
        buf << new_prefix;
        if (rules[i][0].token().to_string() != "") buf << '-';
        rules[i][0].emit_nested_css(buf, depth);
        rules[i][1].emit_nested_css(buf, depth);
        buf << ';';
      }
    }
  }

  void Node::echo(stringstream& buf, size_t depth) { }
  void Node::emit_expanded_css(stringstream& buf, const string& prefix) { }
}