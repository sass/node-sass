#include <cmath>
#include <sstream>
#include <iomanip>

#ifndef SASS_TO_STRING
#include "to_string.hpp"
#endif

#include "inspect.hpp"
#include "ast.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  string To_String::operator()(List* list)
  {
    string sep(list->separator() == List::SPACE ? " " : ", ");
    if (list->empty()) return "()";
    string first_item((*list)[0]->perform(this));
    string next_item;
    string acc(first_item);
    for (size_t i = 1, L = list->length(); i < L; ++i) {
      next_item = (*list)[i]->perform(this);
      if (i == 1 && !first_item.empty() && !next_item.empty()) acc += sep;
      else if (!next_item.empty())                             acc += sep;
      acc += next_item;
    }
    return acc;
  }

  string To_String::operator()(Function_Call* call)
  {
    string acc(call->name());
    acc += call->arguments()->perform(this);
    return acc;
  }

  string To_String::operator()(Function_Call_Schema* call)
  {
    string acc(call->name()->perform(this));
    acc += call->arguments()->perform(this);
    return acc;
  }

  string To_String::operator()(Textual* txt)
  {
    return txt->value();
  }

  // helper functions for serializing numbers
  static string frac_to_string(double f, size_t p) {
    stringstream ss;
    ss.setf(ios::fixed, ios::floatfield);
    ss.precision(p);
    ss << f;
    string result(ss.str().substr(f < 0 ? 2 : 1));
    size_t i = result.size() - 1;
    while (result[i] == '0') --i;
    result = result.substr(0, i+1);
    return result;
  }
  static string double_to_string(double d, size_t p) {
    stringstream ss;
    double ipart;
    double fpart = std::modf(d, &ipart);
    ss << ipart;
    if (fpart != 0) ss << frac_to_string(fpart, 5);
    return ss.str();
  }

  string To_String::operator()(Number* n)
  {
    return double_to_string(n->value(), 5);
  }

  string To_String::operator()(Percentage* p)
  {
    return double_to_string(p->value(), 5) += '%';
  }

  string To_String::operator()(Dimension* d)
  {
    // TODO: check for sane units
    return double_to_string(d->value(), 5) += d->numerator_units()[0];
  }

  // helper function for serializing colors
  template <size_t range>
  static double cap_channel(double c) {
    if      (c > range) return range;
    else if (c < 0)     return 0;
    else                return c;
  }

  string To_String::operator()(Color* c)
  {
    stringstream ss;
    double r = cap_channel<0xff>(c->r());
    double g = cap_channel<0xff>(c->g());
    double b = cap_channel<0xff>(c->b());
    double a = cap_channel<1>   (c->a());
    if (a >= 1) {
      ss << '#' << setw(2) << setfill('0');
      ss << hex << setw(2) << static_cast<unsigned long>(floor(r+0.5));
      ss << hex << setw(2) << static_cast<unsigned long>(floor(g+0.5));
      ss << hex << setw(2) << static_cast<unsigned long>(floor(b+0.5));
    }
    else {
      ss << "rgba(";
      ss << static_cast<unsigned long>(r) << ", ";
      ss << static_cast<unsigned long>(g) << ", ";
      ss << static_cast<unsigned long>(b) << ", ";
      ss << static_cast<unsigned long>(a) << ')';
    }
    return ss.str();
  }

  string To_String::operator()(Boolean* b)
  {
    return b->value() ? "true" : "false";
  }

  string To_String::operator()(String_Schema* ss)
  {

    string acc;
    for (size_t i = 0, L = ss->length(); i < L; ++i) {
      if ((*ss)[i]->is_interpolant()) acc += "#{";
      acc += (*ss)[i]->perform(this);
      if ((*ss)[i]->is_interpolant()) acc += '}';
    }
    return acc;
  }

  string To_String::operator()(String_Constant* s)
  {
    return s->value();
  }

  string To_String::operator()(Media_Query* mq)
  {
    string acc;
    size_t i = 0;
    if (mq->media_type()) {
      if      (mq->is_negated())    acc += "not ";
      else if (mq->is_restricted()) acc += "only ";
      acc += mq->media_type()->perform(this);
    }
    else {
      acc += (*mq)[i++]->perform(this);
    }
    for (size_t L = mq->length(); i < L; ++i) {
      acc += " and ";
      acc += (*mq)[i]->perform(this);
    }
    return acc;
  }

  string To_String::operator()(Media_Query_Expression* mqe)
  {
    string acc("(");
    acc += mqe->feature()->perform(this);
    if (mqe->value()) {
      acc += ": ";
      acc += mqe->value()->perform(this);
    }
    acc += ')';
    return acc;
  }

  string To_String::operator()(Argument* a)
  {
    string acc;
    if (!a->name().empty()) {
      acc += a->name();
      acc += ": ";
    }
    acc += a->value()->perform(this);
    if (a->is_rest_argument()) {
      acc += "...";
    }
    return acc;
  }

  string To_String::operator()(Arguments* a)
  {
    string acc("(");
    if (!a->empty()) {
      acc += (*a)[0]->perform(this);
      for (size_t i = 1, L = a->length(); i < L; ++i) {
        acc += ", ";
        acc += (*a)[i]->perform(this);
      }
    }
    acc += ')';
    return acc;
  }

  string To_String::operator()(Selector_Schema* ss)
  {
    return ss->contents()->perform(this);
  }

  string To_String::operator()(Selector_Reference* ref)
  {
    if (ref->selector()) return ref->selector()->perform(this);
    else                 return "&";
  }

  string To_String::operator()(Selector_Placeholder* p)
  {
    return p->name();
  }

  string To_String::operator()(Type_Selector* t)
  {
    return t->name();
  }

  string To_String::operator()(Selector_Qualifier* q)
  {
    return q->name();
  }

  string To_String::operator()(Attribute_Selector* attr)
  {
    string acc("[");
    acc += attr->name();
    if (!attr->matcher().empty()) {
      acc += attr->matcher();
      acc += attr->value();
    }
    acc += ']';
    return acc;
  }

  string To_String::operator()(Pseudo_Selector* ps)
  {
    string acc(ps->name());
    if (ps->expression()) {
      acc += ps->expression()->perform(this);
      acc += ')';
    }
    return acc;
  }

  string To_String::operator()(Negated_Selector* ns)
  {
    string acc(":not(");
    acc += ns->selector()->perform(this);
    acc += ')';
    return acc;
  }

  string To_String::operator()(Simple_Selector_Sequence* sss)
  {
    string acc;
    for (size_t i = 0, L = sss->length(); i < L; ++i)
      acc += (*sss)[i]->perform(this);
    return acc;
  }

  string To_String::operator()(Selector_Combination* c)
  {
    string lhs(c->head() ? c->head()->perform(this) : "");
    string rhs(c->tail() ? c->tail()->perform(this) : "");
    if (!lhs.empty() && !rhs.empty()) lhs += ' ';
    switch (c->combinator()) {
      case Selector_Combination::ANCESTOR_OF:             break;
      case Selector_Combination::PARENT_OF:   lhs += '>'; break;
      case Selector_Combination::PRECEDES:    lhs += '~'; break;
      case Selector_Combination::ADJACENT_TO: lhs += '+'; break;
    }
    if (!rhs.empty() &&
        !lhs.empty() &&
        c->combinator() != Selector_Combination::ANCESTOR_OF) {
      lhs += ' ';
    }
    lhs += rhs;
    return lhs;
  }

  string To_String::operator()(Selector_Group* g)
  {
    if (g->empty()) return string();
    string acc((*g)[0]->perform(this));
    for (size_t i = 1, L = g->length(); i < L; ++i) {
      acc += ", ";
      acc += (*g)[i]->perform(this);
    }
    return acc;
  }

  // will only be called on non-terminal interpolants
  inline string To_String::fallback_impl(AST_Node* n)
  {
    Inspect i;
    n->perform(&i);
    return i.get_buffer();
  }

}