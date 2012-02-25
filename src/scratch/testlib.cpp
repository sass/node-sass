#include <iostream>
#include "testlib.hpp"

namespace TestLib {
  
  using std::cout;
  using std::endl;
  
  extern const char HELLO[] = "Hello, world!";

  template <const char *str>
  void msg() {
    cout << str << endl;
  }
  
}

void say_hello() {
  TestLib::msg<TestLib::HELLO>();
}