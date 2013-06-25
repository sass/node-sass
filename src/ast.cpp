#include "ast.hpp"
#include "to_string.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  bool Simple_Selector_Sequence::operator<(const Simple_Selector_Sequence& rhs) const
  {
    To_String to_string;
    // ugly
    return const_cast<Simple_Selector_Sequence*>(this)->perform(&to_string) <
           const_cast<Simple_Selector_Sequence&>(rhs).perform(&to_string);
  }

  Simple_Selector_Sequence* Selector_Combination::base()
  {
    // To_String to_string;
    // cerr << "finding base of " << perform(&to_string) << endl;
    // Simple_Selector_Sequence* h = head();
    // Selector_Combination* t = 0;
    // while (t = tail()) h = t->head();
    // return h;
    if (!tail()) return head();
    else return tail()->base();
  }

}