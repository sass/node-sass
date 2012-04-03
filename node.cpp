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
  size_t Node::allocations = 0;
  
  Node Node::clone() const
  {
    Node n(*this);
    if (has_children) {
      n.content.children = new vector<Node>;
      ++allocations;
      n.content.children->reserve(size());
      for (int i = 0; i < size(); ++i) {
        n << at(i).clone();
      }
    }
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
        if (at(0).type == selector_combinator) {
          result += at(0).content.token.to_string();
          result += ' ';
        }
        else {
          result += at(0).to_string(prefix);
        }
        for (int i = 1; i < size(); ++i) {
          result += at(i).to_string(prefix);
        }
        return result;
      }  break;
      
      case selector_combinator: {
        if (std::isspace(content.token.begin[0])) return string(" ");
        else return string(" ") += string(content.token) += string(" ");
      } break;
      
      case simple_selector_sequence: {
        string result;
        for (int i = 0; i < size(); ++i) {
          result += at(i).to_string(prefix);
        }
        return result;
      }  break;
      
      case pseudo_negation: {
        string result(at(0).to_string(prefix));
        result += at(1).to_string(prefix);
        result += ')';
        return result;
      } break;
      
      case functional_pseudo: {
        string result(at(0).to_string(prefix));
        for (int i = 1; i < size(); ++i) {
          result += at(i).to_string(prefix);
        }
        result += ')';
        return result;
      } break;
      
      case attribute_selector: {
        string result("[");
        for (int i = 0; i < 3; ++i)
        { result += at(i).to_string(prefix); }
        result += ']';
        return result;
      } break;
      
      case backref: {
        return prefix;
      } break;
      
      case comma_list: {
        string result(at(0).to_string(prefix));
        for (int i = 1; i < size(); ++i) {
          result += ", ";
          result += at(i).to_string(prefix);
        }
        return result;
      } break;
      
      case space_list: {
        string result(at(0).to_string(prefix));
        for (int i = 1; i < size(); ++i) {
          result += " ";
          result += at(i).to_string(prefix);
        }
        return result;
      } break;
      
      case expression:
      case term: {
        string result(at(0).to_string(prefix));
        for (int i = 1; i < size(); ++i) {
          if (!(at(i).type == add ||
                // at(i).type == sub ||  // another edge case -- consider uncommenting
                at(i).type == mul)) {
            result += at(i).to_string(prefix);
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
        ss << content.dimension.numeric_value;
        ss << string(content.dimension.unit, 2);
           // << string(content.dimension.unit, Prelexer::identifier(content.dimension.unit) - content.dimension.unit);
         // cerr << Token::make(content.dimension.unit, content.dimension.unit + 2).to_string();
           // << Token::make(content.dimension.unit, Prelexer::identifier(content.dimension.unit)).to_string();
        return ss.str();
      } break;
      
      case number: {
        stringstream ss;
        ss << content.numeric_value;
        return ss.str();
      } break;
      
      case numeric_color: {
        double a = at(0).content.numeric_value;
        double b = at(1).content.numeric_value;
        double c = at(2).content.numeric_value;
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
            double x = at(i).content.numeric_value;
            if (x > 0xff) x = 0xff;
            else if (x < 0) x = 0;
            ss  << std::hex << std::setw(2) << static_cast<unsigned long>(x);
          }
          return ss.str();
        }
      } break;
      
      case uri: {
        string result("url(");
        result += string(content.token);
        result += ")";
        return result;
      } break;
      
      // case expansion: {
      //   string result("MIXIN CALL: ");
      //   return result;
      // } break;

      default: {
        // return content.token.to_string();
        if (!has_children && type != flags) return content.token.to_string();
        else return "";
      } break;
    }
  }
  
  void Node::echo(stringstream& buf, size_t depth) {
    string indentation(2*depth, ' ');
    switch (type) {
    case comment:
      buf << indentation << string(content.token) << endl;
      break;
    case ruleset:
      buf << indentation;
      at(0).echo(buf, depth);
      at(1).echo(buf, depth);
      break;
    case selector_group:
      at(0).echo(buf, depth);
      for (int i = 1; i < size(); ++i) {
        buf << ", ";
        at(i).echo(buf, depth);
      }
      break;
    case selector:
      for (int i = 0; i < size(); ++i) {
        at(i).echo(buf, depth);
      }
      break;
    case selector_combinator:
      if (std::isspace(content.token.begin[0])) buf << ' ';
      else buf << ' ' << string(content.token) << ' ';
      break;
    case simple_selector_sequence:
      for (int i = 0; i < size(); ++i) {
        buf << at(i).to_string(string());
      }
      break;
    case simple_selector:
      buf << string(content.token);
      break;
    case block:
      buf << " {" << endl;
      for (int i = 0; i < size(); at(i++).echo(buf, depth+1)) ;
      buf << indentation << "}" << endl;
      break;
    case rule:
      buf << indentation;
      at(0).echo(buf, depth);
      buf << ": ";
      at(1).echo(buf, depth);
      buf << ';' << endl;
      break;
    case property:
      buf << string(content.token);
      break;
    case values:
      for (int i = 0; i < size(); at(i++).echo(buf, depth)) ;
      break;
    case value:
      buf << ' ' << string(content.token);
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
      if (at(0).has_expansions) {
        flatten();
      }
      for (int i = 0; i < size(); ++i) {
        at(i).emit_nested_css(buf, depth, prefixes);
      }
      break;

    case ruleset: {
      Node sel_group(at(0));
      Node block(at(1));
      vector<string> new_prefixes;
      
      if (prefixes.empty()) {
        new_prefixes.reserve(sel_group.size());
        for (int i = 0; i < sel_group.size(); ++i) {
          new_prefixes.push_back(sel_group[i].to_string(string()));
        }
      }
      else {
        new_prefixes.reserve(prefixes.size() * sel_group.size());
        for (int i = 0; i < prefixes.size(); ++i) {
          for (int j = 0; j < sel_group.size(); ++j) {
            new_prefixes.push_back(sel_group[j].to_string(prefixes[i]));
          }
        }
      }
      if (block[0].has_expansions) block.flatten();
      if (block[0].has_statements) {
        buf << string(2*depth, ' ') << new_prefixes[0];
        for (int i = 1; i < new_prefixes.size(); ++i) {
          buf << ", " << new_prefixes[i];
        }
        buf << " {";
        for (int i = 0; i < block.size(); ++i) {
          Type stm_type = block[i].type;
          if (stm_type == comment || stm_type == rule) {
            block[i].emit_nested_css(buf, depth+1); // NEED OVERLOADED VERSION FOR COMMENTS AND RULES
          }
          // else if (stm_type == expansion) {
          //   // buf << string(2*(depth+1), ' ') << block[i].to_string(""); // TEMPORARY
          //   for (int j = 0; j < block[i].size(); ++j) {
          //     block[i][j].emit_nested_css(buf, depth+1);
          //   }
          // }
        }
        buf << " }" << endl;
        ++depth; // if we printed content at this level, we need to indent any nested rulesets
      }
      if (block[0].has_blocks) {
        for (int i = 0; i < block.size(); ++i) {
          if (block[i].type == ruleset) {
            block[i].emit_nested_css(buf, depth, new_prefixes);
          }
        }
      }
      if (block[0].has_statements) --depth;
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
      at(0).emit_nested_css(buf, depth); // property
      at(1).emit_nested_css(buf, depth); // values
      buf << ";";
      break;

    case property:
      buf << string(content.token) << ": ";
      break;

    case values:
      for (int i = 0; i < size(); ++i) {
        buf << " " << string(at(i).content.token);
      }
      break;

    case comment:
      if (depth != 0) buf << endl;
      buf << string(2*depth, ' ') << string(content.token);
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
  
  void Node::flatten()
  {
    if (type != block && type != expansion && type != root) return;
    for (int i = 0; i < size(); ++i) {
      if (at(i).type == expansion) {
        Node expn = at(i);
        if (expn[0].has_expansions) expn.flatten();
        at(0).has_statements |= expn[0].has_statements;
        at(0).has_blocks     |= expn[0].has_blocks;
        at(0).has_expansions |= expn[0].has_expansions;
        at(i).type = none;
        content.children->insert(content.children->begin() + i,
                                 expn.content.children->begin(),
                                 expn.content.children->end());
      }
    }
  }
  // 
  // void flatten_block(Node& block)
  // {
  //   
  //   for (int i = 0; i < block.size(); ++i) {
  //     
  //     if (block[i].type == Node::expansion
  //     
  //   }
  //   
  //   
  //   
  // }

}