#include "ast.hpp"
#include "to_string.hpp"

namespace Sass {

  bool Simple_Selector_Sequence::operator<(const Simple_Selector_Sequence& rhs) const
  {
    To_String to_string;
    // ugly
    return const_cast<Simple_Selector_Sequence*>(this)->perform(&to_string) <
           const_cast<Simple_Selector_Sequence&>(rhs).perform(&to_string);
  }

}