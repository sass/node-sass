#include "error.h"
#include "../create_string.h"

namespace SassTypes
{
  Error::Error(napi_env env, Sass_Value* v) : SassValueWrapper(env, v) {}

  Sass_Value* Error::construct(napi_env env, const std::vector<napi_value> raw_val, Sass_Value **out) {
    char const* value = "";

    if (raw_val.size() >= 1) {
      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[0], &t));

      if (t != napi_string) {
        return fail("First argument should be a string.", out);
      }

      value = create_string(env, raw_val[0]);
    }

    return *out = sass_make_error(value);
  }

  napi_value Error::getConstructor(napi_env env, napi_callback cb) {
    napi_value ctor;
    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), NAPI_AUTO_LENGTH, cb, nullptr, 0, nullptr, &ctor));
    return ctor;
  }
}
