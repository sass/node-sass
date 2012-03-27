#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <cstdlib>
#include "node.hpp"

using std::string;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;

namespace Sass {
  size_t Node::fresh = 0;
  size_t Node::copied = 0;
  
  Node Node::clone() const
  {
    Node n;
    n.line_number = line_number;
    n.token = token;
    n.numeric_value = numeric_value;
    n.type = type;
    n.has_rules_or_comments = has_rules_or_comments;
    n.has_rulesets = has_rulesets;
    n.has_propsets = has_propsets;
    n.has_backref = has_backref;
    n.from_variable = from_variable;
    n.eval_me = eval_me;
    n.is_hex = is_hex;
    if (children) {
      n.children = new vector<Node>();
      n.children->reserve(size());
      for (int i = 0; i < size(); ++i) {
        n << at(i).clone();
      }
    }
    ++fresh;
    return n;
  }
  
  string Node::to_string(const string& prefix) const
  {
    switch (type)
    {
      case selector_group: { // really only needed for arg to :not
        string result(at(0).to_string(""));
        for (int i = 1; i < size(); ++i) {
          result += ", ";
          result += at(i).to_string("");
        }
        return result;
      } break;

      case selector: {
        string result;
        if (!has_backref && !prefix.empty()) {
          result += prefix;
          result += ' ';
        }
        if (children->at(0).type == selector_combinator) {
          result += string(children->at(0).token);
          result += ' ';
        }
        else {
          result += children->at(0).to_string(prefix);
        }
        for (int i = 1; i < children->size(); ++i) {
          result += children->at(i).to_string(prefix);
        }
        return result;
      }  break;

      case selector_combinator: {
        if (std::isspace(token.begin[0])) return string(" ");
        else return string(" ") += string(token) += string(" ");
      } break;

      case simple_selector_sequence: {
        string result;
        for (int i = 0; i < children->size(); ++i) {
          result += children->at(i).to_string(prefix);
        }
        return result;
      }  break;

      case pseudo_negation: {
        string result(children->at(0).to_string(prefix));
        result += children->at(1).to_string(prefix);
        result += ')';
        return result;
      } break;

      case functional_pseudo: {
        string result(children->at(0).to_string(prefix));
        for (int i = 1; i < children->size(); ++i) {
          result += children->at(i).to_string(prefix);
        }
        result += ')';
        return result;
      } break;

      case attribute_selector: {
        string result("[");
        for (int i = 0; i < 3; ++i)
        { result += children->at(i).to_string(prefix); }
        result += ']';
        return result;
      } break;

      case backref: {
        return prefix;
      } break;
      
      case comma_list: {
        string result(children->at(0).to_string(prefix));
        for (int i = 1; i < children->size(); ++i) {
          result += ", ";
          result += children->at(i).to_string(prefix);
        }
        return result;
      } break;
      
      case space_list: {
        string result(children->at(0).to_string(prefix));
        for (int i = 1; i < children->size(); ++i) {
          result += " ";
          result += children->at(i).to_string(prefix);
        }
        return result;
      } break;
      
      case expression:
      case term: {
        string result(children->at(0).to_string(prefix));
        // for (int i = 2; i < children->size(); i += 2) {
        //   // result += " ";
        //   result += children->at(i).to_string(prefix);
        // }
        for (int i = 1; i < children->size(); ++i) {
          if (!(children->at(i).type == add ||
                // children->at(i).type == sub ||  // another edge case -- consider uncommenting
                children->at(i).type == mul)) {
            result += children->at(i).to_string(prefix);
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
      
      case numeric_dimension: {
        stringstream ss;
        // ss.setf(std::ios::fixed, std::ios::floatfield);
        // ss.precision(3);
        ss << numeric_value << string(token);
        return ss.str();
      } break;
      
      case number: {
        stringstream ss;
        // ss.setf(std::ios::fixed, std::ios::floatfield);
        // ss.precision(3);
        ss << numeric_value;
        return ss.str();
      } break;
      
      case hex_triple: {
        double a = children->at(0).numeric_value;
        double b = children->at(1).numeric_value;
        double c = children->at(2).numeric_value;
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
          for (int i = 0; i < 3; ++i) {
            double x = children->at(i).numeric_value;
            if (x > 0xff) x = 0xff;
            else if (x < 0) x = 0;
            ss  << std::hex << std::setw(2) << static_cast<unsigned long>(x);
          }
          return ss.str();
        }
      } break;
      
      case uri: {
        string result("url(");
        result += string(token);
        result += ")";
        return result;
      } break;

      default: {
        return string(token);
      } break;
    }
  }
  
  void Node::echo(stringstream& buf, size_t depth) {
    string indentation(2*depth, ' ');
    switch (type) {
    case comment:
      buf << indentation << string(token) << endl;
      break;
    case ruleset:
      buf << indentation;
      children->at(0).echo(buf, depth);
      children->at(1).echo(buf, depth);
      break;
    case selector_group:
      children->at(0).echo(buf, depth);
      for (int i = 1; i < children->size(); ++i) {
        buf << ", ";
        children->at(i).echo(buf, depth);
      }
      break;
    case selector:
      for (int i = 0; i < children->size(); ++i) {
        children->at(i).echo(buf, depth);
      }
      break;
    case selector_combinator:
      if (std::isspace(token.begin[0])) buf << ' ';
      else buf << ' ' << string(token) << ' ';
      break;
    case simple_selector_sequence:
      for (int i = 0; i < children->size(); ++i) {
        buf << children->at(i).to_string(string());
      }
      break;
    case simple_selector:
      buf << string(token);
      break;
    case block:
      buf << " {" << endl;
      for (int i = 0; i < children->size(); children->at(i++).echo(buf, depth+1)) ;
      buf << indentation << "}" << endl;
      break;
    case rule:
      buf << indentation;
      children->at(0).echo(buf, depth);
      buf << ": ";
      children->at(1).echo(buf, depth);
      buf << ';' << endl;
      break;
    case property:
      buf << string(token);
      break;
    case values:
      for (int i = 0; i < children->size(); children->at(i++).echo(buf, depth)) ;
      break;
    case value:
      buf << ' ' << string(token);
      break;
    }
  }

  void Node::emit_nested_css(stringstream& buf,
                             size_t depth,
                             const vector<string>& prefixes)
  {
    switch (type)
    {
    case root:
      for (int i = 0; i < children->size(); ++i) {
        children->at(i).emit_nested_css(buf, depth, prefixes);
      }
      break;

    case ruleset: {
      Node sel_group(children->at(0));
      Node block(children->at(1));
      vector<string> new_prefixes;
      
      if (prefixes.empty()) {
        new_prefixes.reserve(sel_group.children->size());
        for (int i = 0; i < sel_group.children->size(); ++i) {
          new_prefixes.push_back(sel_group.children->at(i).to_string(string()));
        }
      }
      else {
        new_prefixes.reserve(prefixes.size() * sel_group.children->size());
        for (int i = 0; i < prefixes.size(); ++i) {
          for (int j = 0; j < sel_group.children->size(); ++j) {
            new_prefixes.push_back(sel_group.children->at(j).to_string(prefixes[i]));
          }
        }
      }
      if (block.has_rules_or_comments) {
        buf << string(2*depth, ' ') << new_prefixes[0];
        for (int i = 1; i < new_prefixes.size(); ++i) {
          buf << ", " << new_prefixes[i];
        }
        buf << " {";
        for (int i = 0; i < block.children->size(); ++i) {
          Type stm_type = block.children->at(i).type;
          if (stm_type == comment || stm_type == rule) {
            block.children->at(i).emit_nested_css(buf, depth+1); // NEED OVERLOADED VERSION FOR COMMENTS AND RULES
          }
        }
        buf << " }" << endl;
        ++depth; // if we printed content at this level, we need to indent any nested rulesets
      }
      if (block.has_rulesets) {
        for (int i = 0; i < block.children->size(); ++i) {
          if (block.children->at(i).type == ruleset) {
            block.children->at(i).emit_nested_css(buf, depth, new_prefixes);
          }
        }
      }
      if (block.has_rules_or_comments) --depth;
      if (depth == 0 && prefixes.empty()) buf << endl;
    } break;

    default:
      emit_nested_css(buf, depth); // pass it along to the simpler version
      break;
    }
  }
  
  void Node::emit_nested_css(stringstream& buf, size_t depth)
  {
    switch (type)
    {
    case rule:
      buf << endl << string(2*depth, ' ');
      children->at(0).emit_nested_css(buf, depth); // property
      children->at(1).emit_nested_css(buf, depth); // values
      buf << ";";
      break;

    case property:
      buf << string(token) << ": ";
      break;

    case values:
      for (int i = 0; i < children->size(); ++i) {
        buf << " " << string(children->at(i).token);
      }
      break;

    case comment:
      if (depth != 0) buf << endl;
      buf << string(2*depth, ' ') << string(token);
      if (depth == 0) buf << endl;
      break;

    default:
      buf << to_string("");
      break;
    }
  }

  void Node::emit_expanded_css(stringstream& buf, const string& prefix) {
    // switch (type) {
    // case selector:
    //   if (!prefix.empty()) buf << " ";
    //   buf << string(token);
    //   break;
    // case comment:
    //   if (!prefix.empty()) buf << "  ";
    //   buf << string(token) << endl;
    //   break;
    // case property:
    //   buf << string(token) << ":";
    //   break;
    // case values:
    //   for (int i = 0; i < children.size(); ++i) {
    //     buf << " " << string(children[i].token);
    //   }
    //   break;
    // case rule:
    //   buf << "  ";
    //   children[0].emit_expanded_css(buf, prefix);
    //   children[1].emit_expanded_css(buf, prefix);
    //   buf << ";" << endl;
    //   break;
    // case clauses:
    //   if (children.size() > 0) {
    //     buf << " {" << endl;
    //     for (int i = 0; i < children.size(); ++i)
    //       children[i].emit_expanded_css(buf, prefix);
    //     buf << "}" << endl;
    //   }
    //   for (int i = 0; i < opt_children.size(); ++i)
    //     opt_children[i].emit_expanded_css(buf, prefix);
    //   break;
    // case ruleset:
    //   // buf << prefix;
    //   if (children[1].children.size() > 0) {
    //     buf << prefix << (prefix.empty() ? "" : " ");
    //     children[0].emit_expanded_css(buf, "");
    //   }
    //   string newprefix(prefix.empty() ? prefix : prefix + " ");
    //   children[1].emit_expanded_css(buf, newprefix + string(children[0].token));
    //   if (prefix.empty()) buf << endl;
    //   break;
    // }
    
  }

}