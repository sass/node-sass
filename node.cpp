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
  // Node::Node() { ++fresh; children = 0; opt_children = 0; }
  // Node::Node(Node_Type _type) {
  //   type = _type;
  //   children = new vector<Node>;
  //   opt_children = new vector<Node>;
  //   ++fresh;
  // }
  // Node::Node(Node_Type _type, Token& _token) {
  //   type = _type;
  //   token = _token;
  //   children = 0;
  //   opt_children = 0;
  //   ++fresh;
  // }
  // Node::Node(const Node& n) {
  //   type = n.type;
  //   token = n.token;
  //   children = n.children;
  //   opt_children = n.opt_children;
  //   ++copied;
  // }
  // 
  // void Node::push_child(const Node& node) {
  //   if (!children) children = new vector<Node>;
  //   children->push_back(node);
  // }
  // void Node::push_opt_child(const Node& node) {
  //   if (!opt_children) opt_children = new vector<Node>;
  //   opt_children->push_back(node);
  // }
  
  // void Node::dump(unsigned int depth) {
    // switch (type) {
    // case comment:
    //   for (int i = depth; i > 0; --i) cout << "  ";
    //   cout << string(token) << endl;
    //   break;
    // case selector:
    //   cout << string(token);
    //   break;
    // case value:
    //   cout << string(token);
    //   break;
    // case property:
    //   cout << string(token) << ":";
    //   break;
    // case values:
    //   for (int i = 0; i < children.size(); ++i)
    //     cout << " " << string(children[i].token);
    //   break;
    // case rule:
    //   for (int i = depth; i > 0; --i) cout << "  ";
    //   children[0].dump(depth);
    //   children[1].dump(depth);
    //   cout << ";" << endl;
    //   break;
    // case clauses:
    //   cout << " {" << endl;
    //   for (int i = 0; i < children.size(); ++i) {
    //     children[i].dump(depth + 1);
    //   }
    //   for (int i = 0; i < opt_children.size(); ++i) {
    //     opt_children[i].dump(depth + 1);
    //   }
    //   for (int i = depth; i > 0; --i) cout << "  ";
    //   cout << "}" << endl;
    //   break;
    // case ruleset:
    //   for (int i = depth; i > 0; --i) cout << "  ";
    //   children[0].dump(depth);
    //   children[1].dump(depth);
    //   break;
    // default: cout << "HUH?"; break;
    // }
  // }
  
  void Node::emit_nested_css(stringstream& buf,
                             const string& prefix,
                             size_t depth) {
    string indentation(2 * depth, ' ');
    vector<Node>* nodes;
    if (type == ruleset) {
      nodes = children->at(1).children;
      has_rules = children->at(1).has_rules;
      has_rulesets = children->at(1).has_rulesets;
    }
    switch (type) {
    case ruleset:
      if (has_rules) {
        buf << indentation;
        children->at(0).emit_nested_css(buf, prefix, depth); // selector
        buf << " {";
        for (int i = 0; i < nodes->size(); ++i) {
          if (nodes->at(i).type == rule) nodes->at(i).emit_nested_css(buf, "", depth + 1); // rules
        }
        buf << " }" << endl;
      }
      if (has_rulesets) {
        for (int i = 0; i < nodes->size(); ++i) { // do each nested ruleset
          if (nodes->at(i).type == ruleset) nodes->at(i).emit_nested_css(buf, prefix + (prefix.empty() ? "" : " ") + string((*children)[0].token), depth + (has_rules ? 1 : 0));
        }
      }
      if (depth == 0 && prefix.empty()) buf << endl;
      break;
    case rule:
      buf << endl << indentation;
      children->at(0).emit_nested_css(buf, "", depth); // property
      children->at(1).emit_nested_css(buf, "", depth); // values
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
    case selector:
      buf << prefix << (prefix.empty() ? "" : " ") << string(token);
      break;
    case comment:
    if (depth != 0) buf << endl;
      buf << indentation << string(token);
      if (depth == 0) buf << endl;
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