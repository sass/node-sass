#include "units.hpp"

#define PI 3.14159265358979323846

namespace Sass {

  double conversion_factors[10][10] = {
             /*  in         cm         pc         mm         pt         px                 deg        grad       rad        turn      */
    /* in   */ { 1,         2.54,      6,         25.4,      72,        96,                1,         1,         1,         1         },
    /* cm   */ { 1.0/2.54,  1,         6.0/2.54,  10,        72.0/2.54, 96.0/2.54,         1,         1,         1,         1         },
    /* pc   */ { 1.0/6.0,   2.54/6.0,  1,         25.4/6.0,  72.0/6.0,  96.0/6.0,          1,         1,         1,         1         },
    /* mm   */ { 1.0/25.4,  1.0/10.0,  6.0/25.4,  1,         72.0/25.4, 96.0/25.4,         1,         1,         1,         1         },
    /* pt   */ { 1.0/72.0,  2.54/72.0, 6.0/72.0,  25.4/72.0, 1,         96.0/72.0,         1,         1,         1,         1         },
    /* px   */ { 1.0/96.0,  2.54/96.0, 6.0/96.0,  25.4/96.0, 72.0/96.0, 1,                 1,         1,         1,         1         },
    /* deg  */ { 1       ,  1        , 1       ,  1        , 1        , 1,                 1,         40.0/36.0, PI/180.0,  1.0/360.0 },
    /* grad */ { 1       ,  1        , 1       ,  1        , 1        , 1,                 36.0/40.0, 1,         PI/200.0,  1.0/400.0 },
    /* rad  */ { 1       ,  1        , 1       ,  1        , 1        , 1,                 180.0/PI,  200.0/PI,  1,         PI/2.0    },
    /* turn */ { 1       ,  1        , 1       ,  1        , 1        , 1,                 360.0/1.0, 400.0/1.0, 2.0*PI,    1         }
  };

  Unit string_to_unit(const string& s)
  {
    if      (s == "in") return IN;
    else if (s == "cm") return CM;
    else if (s == "pc") return PC;
    else if (s == "mm") return MM;
    else if (s == "pt") return PT;
    else if (s == "px") return PX;
    else if (s == "deg") return DEG;
    else if (s == "grad") return GRAD;
    else if (s == "rad") return RAD;
    else if (s == "turn") return TURN;
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

  /* not used anymore - remove?
  double convert(double n, const string& from, const string& to)
  {
    double factor = conversion_factor(from, to);
    return factor ? factor * n : n;
  } */

}
