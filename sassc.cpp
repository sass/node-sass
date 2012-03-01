#include <iostream>
#include <string>
#include "document.hpp"

using namespace Sass;
using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Hey, I need a file to read!" << endl;
    return 0;
  }

  Document doc(argv[1], 0);
  doc.parse_scss();
  // string output = doc.emit_css(doc.expanded);
  // cout << output;
  // cout << endl;
  string output = doc.emit_css(doc.nested);
  cout << output;

  return 0;
}