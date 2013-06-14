#include "units.hpp"

namespace Sass {

  double conversion_factors[6][6] = {
             /*  in       cm       pc       mm       pt       px   */
    /* in */ { 1,       2.54,    6,       25.4,    72,      96      },
    /* cm */ { 1/2.54,  1,       6/2.54,  10,      72/2.54, 96/2.54 },
    /* pc */ { 1/6,     2.54/6,  1,       25.4/6,  72/6,    96/6    },
    /* mm */ { 1/25.4,  1/10,    6/25.4,  1,       72/25.4, 96/25.4 },
    /* pt */ { 1/72,    2.54/72, 6/72,    25.4/72, 1,       96/72   },
    /* px */ { 1/96,    2.54/96, 6/96,    25.4/96, 72/96,   1       }
  };

  Unit string_to_unit(const string& s)
  {
    if      (s == "in") return IN;
    else if (s == "cm") return CM;
    else if (s == "mm") return MM;
    else if (s == "pt") return PT;
    else if (s == "pc") return PC;
    else if (s == "px") return PX;
    else                return INCOMMENSURABLE;
  }

  double conversion_factor(const string& s1, const string& s2)
  {
    Unit u1 = string_to_unit(s1);
    Unit u2 = string_to_unit(s2);
    double factor;
    if (u1 == INCOMMENSURABLE || u2 == INCOMMENSURABLE)
      factor = 0;
    else
      factor = conversion_factors[u1][u2];
    return factor;
  }

  double convert(double n, const string& from, const string& to)
  {
    double factor = conversion_factor(from, to);
    return factor ? factor * n : n;
  }

}