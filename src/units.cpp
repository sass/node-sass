#include <stdexcept>
#include "units.hpp"

#ifdef IN
#undef IN
#endif

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
    /* dpi  */ { 1,         2.54,      96       },
    /* dpcm */ { 1/2.54,    1,         96/2.54  },
    /* dppx */ { 1/96.0,    2.54/96,   1        }
  };

  SassUnitType get_unit_type(SassUnit unit)
  {
    switch (unit & 0xFF00)
    {
      case SassUnitType::LENGTH:      return SassUnitType::LENGTH; break;
      case SassUnitType::ANGLE:       return SassUnitType::ANGLE; break;
      case SassUnitType::TIME:        return SassUnitType::TIME; break;
      case SassUnitType::FREQUENCY:   return SassUnitType::FREQUENCY; break;
      case SassUnitType::RESOLUTION:  return SassUnitType::RESOLUTION; break;
      default:                        return SassUnitType::INCOMMENSURABLE; break;
    }
  };

  SassUnit string_to_unit(const std::string& s)
  {
    // size units
    if      (s == "px")   return SassUnit::PX;
    else if (s == "pt")   return SassUnit::PT;
    else if (s == "pc")   return SassUnit::PC;
    else if (s == "mm")   return SassUnit::MM;
    else if (s == "cm")   return SassUnit::CM;
    else if (s == "in")   return SassUnit::IN;
    // angle units
    else if (s == "deg")  return SassUnit::DEG;
    else if (s == "grad") return SassUnit::GRAD;
    else if (s == "rad")  return SassUnit::RAD;
    else if (s == "turn") return SassUnit::TURN;
    // time units
    else if (s == "s")    return SassUnit::SEC;
    else if (s == "ms")   return SassUnit::MSEC;
    // frequency units
    else if (s == "Hz")   return SassUnit::HERTZ;
    else if (s == "kHz")  return SassUnit::KHERTZ;
    // resolutions units
    else if (s == "dpi")  return SassUnit::DPI;
    else if (s == "dpcm") return SassUnit::DPCM;
    else if (s == "dppx") return SassUnit::DPPX;
    // for unknown units
    else return SassUnit::UNKNOWN;
  }

  const char* unit_to_string(SassUnit unit)
  {
    switch (unit) {
      // size units
      case SassUnit::PX:      return "px"; break;
      case SassUnit::PT:      return "pt"; break;
      case SassUnit::PC:      return "pc"; break;
      case SassUnit::MM:      return "mm"; break;
      case SassUnit::CM:      return "cm"; break;
      case SassUnit::IN:      return "in"; break;
      // angle units
      case SassUnit::DEG:     return "deg"; break;
      case SassUnit::GRAD:    return "grad"; break;
      case SassUnit::RAD:     return "rad"; break;
      case SassUnit::TURN:    return "turn"; break;
      // time units
      case SassUnit::SEC:     return "s"; break;
      case SassUnit::MSEC:    return "ms"; break;
      // frequency units
      case SassUnit::HERTZ:   return "Hz"; break;
      case SassUnit::KHERTZ:  return "kHz"; break;
      // resolutions units
      case SassUnit::DPI:     return "dpi"; break;
      case SassUnit::DPCM:    return "dpcm"; break;
      case SassUnit::DPPX:    return "dppx"; break;
      // for unknown units
      default:                return ""; break;
    }
  }

  // throws incompatibleUnits exceptions
  double conversion_factor(const std::string& s1, const std::string& s2, bool strict)
  {
    // assert for same units
    if (s1 == s2) return 1;
    // get unit enum from string
    SassUnit u1 = string_to_unit(s1);
    SassUnit u2 = string_to_unit(s2);
    // query unit group types
    SassUnitType t1 = get_unit_type(u1);
    SassUnitType t2 = get_unit_type(u2);
    // get absolute offset
    // used for array acces
    size_t i1 = u1 - t1;
    size_t i2 = u2 - t2;
    // error if units are not of the same group
    // don't error for multiplication and division
    if (strict && t1 != t2) throw incompatibleUnits(u1, u2);
    // only process known units
    if (u1 != UNKNOWN && u2 != UNKNOWN) {
      switch (t1) {
        case SassUnitType::LENGTH:              return size_conversion_factors[i1][i2]; break;
        case SassUnitType::ANGLE:             return angle_conversion_factors[i1][i2]; break;
        case SassUnitType::TIME:              return time_conversion_factors[i1][i2]; break;
        case SassUnitType::FREQUENCY:         return frequency_conversion_factors[i1][i2]; break;
        case SassUnitType::RESOLUTION:        return resolution_conversion_factors[i1][i2]; break;
        // ToDo: should we throw error here?
        case SassUnitType::INCOMMENSURABLE:   return 0; break;
      }
    }
    // fallback
    return 1;
  }

}
