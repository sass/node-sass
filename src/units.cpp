#include "sass.hpp"
#include <stdexcept>
#include "units.hpp"
#include "error_handling.hpp"

namespace Sass {

  /* the conversion matrix can be readed the following way */
  /* if you go down, the factor is for the numerator (multiply) */
  /* if you go right, the factor is for the denominator (divide) */
  /* and yes, we actually use both, not sure why, but why not!? */

  const double size_conversion_factors[6][6] =
  {
             /*  in         cm         pc         mm         pt         px        */
    /* in   */ { 1,         2.54,      6,         25.4,      72,        96,       },
    /* cm   */ { 1.0/2.54,  1,         6.0/2.54,  10,        72.0/2.54, 96.0/2.54 },
    /* pc   */ { 1.0/6.0,   2.54/6.0,  1,         25.4/6.0,  72.0/6.0,  96.0/6.0  },
    /* mm   */ { 1.0/25.4,  1.0/10.0,  6.0/25.4,  1,         72.0/25.4, 96.0/25.4 },
    /* pt   */ { 1.0/72.0,  2.54/72.0, 6.0/72.0,  25.4/72.0, 1,         96.0/72.0 },
    /* px   */ { 1.0/96.0,  2.54/96.0, 6.0/96.0,  25.4/96.0, 72.0/96.0, 1,        }
  };

  const double angle_conversion_factors[4][4] =
  {
             /*  deg        grad       rad        turn      */
    /* deg  */ { 1,         40.0/36.0, PI/180.0,  1.0/360.0 },
    /* grad */ { 36.0/40.0, 1,         PI/200.0,  1.0/400.0 },
    /* rad  */ { 180.0/PI,  200.0/PI,  1,         0.5/PI    },
    /* turn */ { 360.0,     400.0,     2.0*PI,    1         }
  };

  const double time_conversion_factors[2][2] =
  {
             /*  s          ms        */
    /* s    */ { 1,         1000.0    },
    /* ms   */ { 1/1000.0,  1         }
  };
  const double frequency_conversion_factors[2][2] =
  {
             /*  Hz         kHz       */
    /* Hz   */ { 1,         1/1000.0  },
    /* kHz  */ { 1000.0,    1         }
  };
  const double resolution_conversion_factors[3][3] =
  {
             /*  dpi        dpcm       dppx     */
    /* dpi  */ { 1,         1/2.54,    1/96.0   },
    /* dpcm */ { 2.54,      1,         2.54/96  },
    /* dppx */ { 96,        96/2.54,   1        }
  };

  UnitClass get_unit_type(UnitType unit)
  {
    switch (unit & 0xFF00)
    {
      case UnitClass::LENGTH:      return UnitClass::LENGTH;
      case UnitClass::ANGLE:       return UnitClass::ANGLE;
      case UnitClass::TIME:        return UnitClass::TIME;
      case UnitClass::FREQUENCY:   return UnitClass::FREQUENCY;
      case UnitClass::RESOLUTION:  return UnitClass::RESOLUTION;
      default:                     return UnitClass::INCOMMENSURABLE;
    }
  };

  std::string get_unit_class(UnitType unit)
  {
    switch (unit & 0xFF00)
    {
      case UnitClass::LENGTH:      return "LENGTH";
      case UnitClass::ANGLE:       return "ANGLE";
      case UnitClass::TIME:        return "TIME";
      case UnitClass::FREQUENCY:   return "FREQUENCY";
      case UnitClass::RESOLUTION:  return "RESOLUTION";
      default:                     return "INCOMMENSURABLE";
    }
  };

  UnitType string_to_unit(const std::string& s)
  {
    // size units
    if      (s == "px")   return UnitType::PX;
    else if (s == "pt")   return UnitType::PT;
    else if (s == "pc")   return UnitType::PC;
    else if (s == "mm")   return UnitType::MM;
    else if (s == "cm")   return UnitType::CM;
    else if (s == "in")   return UnitType::IN;
    // angle units
    else if (s == "deg")  return UnitType::DEG;
    else if (s == "grad") return UnitType::GRAD;
    else if (s == "rad")  return UnitType::RAD;
    else if (s == "turn") return UnitType::TURN;
    // time units
    else if (s == "s")    return UnitType::SEC;
    else if (s == "ms")   return UnitType::MSEC;
    // frequency units
    else if (s == "Hz")   return UnitType::HERTZ;
    else if (s == "kHz")  return UnitType::KHERTZ;
    // resolutions units
    else if (s == "dpi")  return UnitType::DPI;
    else if (s == "dpcm") return UnitType::DPCM;
    else if (s == "dppx") return UnitType::DPPX;
    // for unknown units
    else return UnitType::UNKNOWN;
  }

  const char* unit_to_string(UnitType unit)
  {
    switch (unit) {
      // size units
      case UnitType::PX:      return "px";
      case UnitType::PT:      return "pt";
      case UnitType::PC:      return "pc";
      case UnitType::MM:      return "mm";
      case UnitType::CM:      return "cm";
      case UnitType::IN:      return "in";
      // angle units
      case UnitType::DEG:     return "deg";
      case UnitType::GRAD:    return "grad";
      case UnitType::RAD:     return "rad";
      case UnitType::TURN:    return "turn";
      // time units
      case UnitType::SEC:     return "s";
      case UnitType::MSEC:    return "ms";
      // frequency units
      case UnitType::HERTZ:   return "Hz";
      case UnitType::KHERTZ:  return "kHz";
      // resolutions units
      case UnitType::DPI:     return "dpi";
      case UnitType::DPCM:    return "dpcm";
      case UnitType::DPPX:    return "dppx";
      // for unknown units
      default:                return "";
    }
  }

  std::string unit_to_class(const std::string& s)
  {
    if      (s == "px")   return "LENGTH";
    else if (s == "pt")   return "LENGTH";
    else if (s == "pc")   return "LENGTH";
    else if (s == "mm")   return "LENGTH";
    else if (s == "cm")   return "LENGTH";
    else if (s == "in")   return "LENGTH";
    // angle units
    else if (s == "deg")  return "ANGLE";
    else if (s == "grad") return "ANGLE";
    else if (s == "rad")  return "ANGLE";
    else if (s == "turn") return "ANGLE";
    // time units
    else if (s == "s")    return "TIME";
    else if (s == "ms")   return "TIME";
    // frequency units
    else if (s == "Hz")   return "FREQUENCY";
    else if (s == "kHz")  return "FREQUENCY";
    // resolutions units
    else if (s == "dpi")  return "RESOLUTION";
    else if (s == "dpcm") return "RESOLUTION";
    else if (s == "dppx") return "RESOLUTION";
    // for unknown units
    return "CUSTOM:" + s;
  }

  // throws incompatibleUnits exceptions
  double conversion_factor(const std::string& s1, const std::string& s2, bool strict)
  {
    // assert for same units
    if (s1 == s2) return 1;
    // get unit enum from string
    UnitType u1 = string_to_unit(s1);
    UnitType u2 = string_to_unit(s2);
    // query unit group types
    UnitClass t1 = get_unit_type(u1);
    UnitClass t2 = get_unit_type(u2);
    // get absolute offset
    // used for array acces
    size_t i1 = u1 - t1;
    size_t i2 = u2 - t2;
    // error if units are not of the same group
    // don't error for multiplication and division
    if (strict && t1 != t2) throw Exception::IncompatibleUnits(u1, u2);
    // only process known units
    if (u1 != UNKNOWN && u2 != UNKNOWN) {
      switch (t1) {
        case UnitClass::LENGTH:            return size_conversion_factors[i1][i2];
        case UnitClass::ANGLE:             return angle_conversion_factors[i1][i2];
        case UnitClass::TIME:              return time_conversion_factors[i1][i2];
        case UnitClass::FREQUENCY:         return frequency_conversion_factors[i1][i2];
        case UnitClass::RESOLUTION:        return resolution_conversion_factors[i1][i2];
        // ToDo: should we throw error here?
        case UnitClass::INCOMMENSURABLE:   return 0;
        default: break;
      }
    }
    // fallback
    return 0;
  }

}
