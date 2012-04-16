#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include "node.hpp"
#include "error.hpp"

using std::string;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;

namespace Sass {
  
  bool Node::operator==(const Node& rhs) const
  {
    if (type != rhs.type) return false;
    
    switch (type)
    {
      case comma_list:
      case space_list:
      case expression:
      case term: {
        for (int i = 0; i < size(); ++i) {
          if (at(i) == rhs[i]) continue;
          else return false;
        }
        return true;
      } break;
      
      case identifier:
      case uri:
      case textual_percentage:
      case textual_dimension:
      case textual_number:
      case textual_hex:
      case string_constant: {
        return content.token.unquote() == rhs.content.token.unquote();
      } break;
      
      case number:
      case numeric_percentage: {
        return numeric_value() == rhs.numeric_value();
      } break;
      
      case numeric_dimension: {
        if (Token::make(content.dimension.unit, Prelexer::identifier(content.dimension.unit)) ==
            Token::make(rhs.content.dimension.unit, Prelexer::identifier(rhs.content.dimension.unit))) {
          return numeric_value() == rhs.numeric_value();
        }
        else {
          return false;
        }
      } break;
      
      case boolean: {
        return content.boolean_value == rhs.content.boolean_value;
      } break;
      
      default: {
        return true;
      } break;
      
    }
  }
  
  bool Node::operator!=(const Node& rhs) const
  { return !(*this == rhs); }
  
  bool Node::operator<(const Node& rhs) const
  {
    if (type == number && rhs.type == number ||
        type == numeric_percentage && rhs.type == numeric_percentage) {
      return numeric_value() < rhs.numeric_value();
    }
    else if (type == numeric_dimension && rhs.type == numeric_dimension) {
      if (Token::make(content.dimension.unit, Prelexer::identifier(content.dimension.unit)) ==
          Token::make(rhs.content.dimension.unit, Prelexer::identifier(rhs.content.dimension.unit))) {
        return numeric_value() < rhs.numeric_value();
      }
      else {
        throw Error(Error::evaluation, line_number, "", "incompatible units");
      }
    }
    else {
      throw Error(Error::evaluation, line_number, "", "incomparable types");
    }
  }
  
  bool Node::operator<=(const Node& rhs) const
  { return *this < rhs || *this == rhs; }
  
  bool Node::operator>(const Node& rhs) const
  { return !(*this <= rhs); }
  
  bool Node::operator>=(const Node& rhs) const
  { return !(*this < rhs); }
  
}