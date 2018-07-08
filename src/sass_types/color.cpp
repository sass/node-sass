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
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[0], &t));
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
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[3], &t));
      if (t != napi_number) {
        return fail("Constructor arguments should be numbers exclusively.", out);
      }
      CHECK_NAPI_RESULT(napi_get_value_double(env, raw_val[3], &a));
      // fall through vvv

    case 3:
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[0], &t));
      if (t != napi_number) {
        return fail("Constructor arguments should be numbers exclusively.", out);
      }
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[1], &t));
      if (t != napi_number) {
        return fail("Constructor arguments should be numbers exclusively.", out);
      }
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[2], &t));
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
      { "getR", nullptr, GetR },
      { "getG", nullptr, GetG },
      { "getB", nullptr, GetB },
      { "getA", nullptr, GetA },
      { "setR", nullptr, SetR },
      { "setG", nullptr, SetG },
      { "setB", nullptr, SetB },
      { "setA", nullptr, SetA },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), NAPI_AUTO_LENGTH, cb, nullptr, 8, descriptors, &ctor));
    return ctor;
  }

  napi_value Color::GetR(napi_env env, napi_callback_info info) {
    return CommonGetNumber(env, info, sass_color_get_r);
  }

  napi_value Color::GetG(napi_env env, napi_callback_info info) {
    return CommonGetNumber(env, info, sass_color_get_g);
  }

  napi_value Color::GetB(napi_env env, napi_callback_info info) {
    return CommonGetNumber(env, info, sass_color_get_b);
  }

  napi_value Color::GetA(napi_env env, napi_callback_info info) {
    return CommonGetNumber(env, info, sass_color_get_a);
  }

  napi_value Color::SetR(napi_env env, napi_callback_info info) {
    return CommonSetNumber(env, info, sass_color_set_r);
  }

  napi_value Color::SetG(napi_env env, napi_callback_info info) {
    return CommonSetNumber(env, info, sass_color_set_g);
  }

  napi_value Color::SetB(napi_env env, napi_callback_info info) {
    return CommonSetNumber(env, info, sass_color_set_b);
  }

  napi_value Color::SetA(napi_env env, napi_callback_info info) {
    return CommonSetNumber(env, info, sass_color_set_a);
  }
}
