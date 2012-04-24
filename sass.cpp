#include <v8.h>
#include <node.h>

using namespace v8;

Handle<Value> Render(const Arguments& args) {
    HandleScope scope;

    Local<String> result = args[0]->ToString();

    return scope.Close(result);
}

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "render", Render);
}

NODE_MODULE(sass, RegisterModule);