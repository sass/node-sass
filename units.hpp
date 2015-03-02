#ifndef SASS_UNITS_H
#define SASS_UNITS_H

#include <string>

namespace Sass {
  using namespace std;
  enum Unit { IN, CM, PC, MM, PT, PX, DEG, GRAD, RAD, TURN, INCOMMENSURABLE };
  extern double conversion_factors[10][10];
  Unit string_to_unit(const string&);
  double conversion_factor(const string&, const string&);
  // double convert(double, const string&, const string&);
}

#endif
