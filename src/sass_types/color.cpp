#include <nan.h>
#include "color.h"

namespace SassTypes
{
  Color::Color(napi_env env, Sass_Value* v) : SassValueWrapper(env, v) {}

  Sass_Value* Color::construct(napi_env env, const std::vector<napi_value> raw_val, Sass_Value **out) {
    double a = 1.0, r = 0, g = 0, b = 0;
    napi_valuetype t;
    uint32_t argb;

    switch (raw_val.size()) {
    case 1:
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[0], &t));
      if (t != napi_number) {
        return fail("Only argument should be an integer.", out);
      }
      
      CHECK_NAPI_RESULT(napi_get_value_uint32(env, raw_val[0], &argb));
      a = (double)((argb >> 030) & 0xff) / 0xff;
      r = (double)((argb >> 020) & 0xff);
      g = (double)((argb >> 010) & 0xff);
      b = (double)(argb & 0xff);
      break;

    case 4:
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[3], &t));
      if (t != napi_number) {
        return fail("Constructor arguments should be numbers exclusively.", out);
      }
      CHECK_NAPI_RESULT(napi_get_value_double(env, raw_val[3], &a));
      // fall through vvv

    case 3:
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[0], &t));
      if (t != napi_number) {
        return fail("Constructor arguments should be numbers exclusively.", out);
      }
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[1], &t));
      if (t != napi_number) {
        return fail("Constructor arguments should be numbers exclusively.", out);
      }
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[2], &t));
      if (t != napi_number) {
        return fail("Constructor arguments should be numbers exclusively.", out);
      }

      CHECK_NAPI_RESULT(napi_get_value_double(env, raw_val[0], &r));
      CHECK_NAPI_RESULT(napi_get_value_double(env, raw_val[1], &g));
      CHECK_NAPI_RESULT(napi_get_value_double(env, raw_val[2], &b));
      break;

    case 0:
      break;

    default:
      return fail("Constructor should be invoked with either 0, 1, 3 or 4 arguments.", out);
    }

    return *out = sass_make_color(r, g, b, a);
  }

  napi_value Color::getConstructor(napi_env env, napi_callback cb) {
    napi_value ctor;
    napi_property_descriptor descriptors[] = {
      { "getR", GetR },
      { "getG", GetG },
      { "getB", GetB },
      { "getA", GetA },
      { "setR", SetR },
      { "setG", SetG },
      { "setB", SetB },
      { "setA", SetA },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), cb, nullptr, 8, descriptors, &ctor));
    return ctor;
  }

  NAPI_METHOD(Color::GetR) {
    CommonGetNumber(env, info, sass_color_get_r);
  }

  NAPI_METHOD(Color::GetG) {
    CommonGetNumber(env, info, sass_color_get_g);
  }

  NAPI_METHOD(Color::GetB) {
    CommonGetNumber(env, info, sass_color_get_b);
  }

  NAPI_METHOD(Color::GetA) {
    CommonGetNumber(env, info, sass_color_get_a);
  }

  NAPI_METHOD(Color::SetR) {
    CommonSetNumber(env, info, sass_color_set_r);
  }

  NAPI_METHOD(Color::SetG) {
    CommonSetNumber(env, info, sass_color_set_g);
  }

  NAPI_METHOD(Color::SetB) {
    CommonSetNumber(env, info, sass_color_set_b);
  }

  NAPI_METHOD(Color::SetA) {
    CommonSetNumber(env, info, sass_color_set_a);
  }
}
