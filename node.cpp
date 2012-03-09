#include <iostream>
#include <string>
#include "node.hpp"

using std::string;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;

namespace Sass {
  size_t Node::fresh = 0;
  size_t Node::copied = 0;
  
  void Node::echo(size_t depth) {
    string indentation(2*depth, ' ');
    switch (type) {
    case comment:
      cout << indentation << string(token) << endl;
      break;
    case ruleset:
      cout << indentation;
      children->at(0).echo(depth);
      children->at(1).echo(depth);
      break;
    case selector_group:
      children->at(0).echo(depth);
      for (int i = 1; i < children->size(); ++i) {
        cout << ", ";
        children->at(i).echo(depth);
      }
      break;
    case selector:
      cout << string(token);
      break;
    case block:
      cout << " {" << endl;
      for (int i = 0; i < children->size(); children->at(i++).echo(depth+1)) ;
      cout << indentation << "}" << endl;
      break;
    case rule:
      cout << indentation;
      children->at(0).echo(depth);
      cout << ':';
      children->at(1).echo(depth);
      cout << ';' << endl;
      break;
    case property:
      cout << string(token);
      break;
    case values:
      for (int i = 0; i < children->size(); children->at(i++).echo(depth)) ;
      break;
    case value:
      cout << ' ' << string(token);
      break;
    }
  }
  
  
  void Node::emit_nested_css(stringstream& buf,
                             size_t depth,
                             const vector<string>& prefixes)
  {
    string indentation(2*depth, ' ');
    switch (type) {  
    case ruleset: {
      Node sel_group(children->at(0));
      Node block(children->at(1));
      vector<string> new_prefixes;
      
      if (prefixes.empty()) {
        new_prefixes.reserve(sel_group.children->size());
        for (int i = 0; i < sel_group.children->size(); ++i) {
          new_prefixes.push_back(string(sel_group.children->at(i).token));
        }
      }
      else {
        new_prefixes.reserve(prefixes.size() * sel_group.children->size());
        for (int i = 0; i < prefixes.size(); ++i) {
          for (int j = 0; j < sel_group.children->size(); ++j) {
            new_prefixes.push_back(prefixes[i] +
                                   ' ' +
                                   string(sel_group.children->at(j).token));
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
    switch (type) {
    case rule:
      buf << endl << string(2*depth, ' ');
      children->at(0).emit_nested_css(buf, depth); // property
      children->at(1).emit_nested_css(buf, depth); // values
      buf << ";";
      break;
    case property:
      buf << string(token) << ":";
      break;
    case values:
      for (int i = 0; i < children->size(); ++i) {
        buf << " " << string((*children)[i].token);
      }
      break;
    case comment:
      if (depth != 0) buf << endl;
      buf << string(2*depth, ' ') << string(token);
      if (depth == 0) buf << endl;
      break;
    default:
      break;
    }
  }
        
        
  
  
  // void Node::emit_nested_css(stringstream& buf,
  //                            const string& prefix,
  //                            size_t depth) {
  //   string indentation(2 * depth, ' ');
  //   vector<Node>* contents;
  //   if (type == ruleset) {
  //     Node block(children->at(1));
  //     contents              = block.children;
  //     has_rules_or_comments = block.has_rules_or_comments;
  //     has_rulesets          = block.has_rulesets;
  //   }
  //   switch (type) {
  //   case ruleset:
  //     if (has_rules_or_comments) {
  //       buf << indentation;
  //       children->at(0).emit_nested_css(buf, prefix, depth); // selector group
  //       buf << " {";
  //       for (int i = 0; i < contents->size(); ++i) {
  //         if (contents->at(i).type == comment || contents->at(i).type == rule) contents->at(i).emit_nested_css(buf, "", depth + 1); // rules
  //       }
  //       buf << " }" << endl;
  //     }
  //     if (has_rulesets) {
  //       for (int i = 0; i < contents->size(); ++i) { // do each nested ruleset
  //         if (contents->at(i).type == ruleset) contents->at(i).emit_nested_css(buf, prefix + (prefix.empty() ? "" : " ") + string((*children)[0].token), depth + (has_rules_or_comments ? 1 : 0));
  //       }
  //     }
  //     if (depth == 0 && prefix.empty()) buf << endl;
  //     break;
  //   case rule:
  //     buf << endl << indentation;
  //     children->at(0).emit_nested_css(buf, "", depth); // property
  //     children->at(1).emit_nested_css(buf, "", depth); // values
  //     buf << ";";
  //     break;
  //   case property:
  //     buf << string(token) << ":";
  //     break;
  //   case values:
  //     for (int i = 0; i < children->size(); ++i) {
  //       buf << " " << string((*children)[i].token);
  //     }
  //     break;
  //   case selector_group:
  //     children->at(0).emit_nested_css(buf, prefix, depth);
  //     for (int i = 1; i < children->size(); ++i) {
  //       children->at(i).emit_nested_css(buf, prefix, depth);
  //     }
  //     break;
  //   case selector:
  //     buf << prefix << (prefix.empty() ? "" : " ") << string(token);
  //     break;
  //   case comment:
  //   if (depth != 0) buf << endl;
  //     buf << indentation << string(token);
  //     if (depth == 0) buf << endl;
  //     break;
  //   }
  // }
  
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