#include <iostream>
#include <sstream>
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
    
  stringstream output;
  for (int i = 0; i < doc.statements.size(); ++i) {
    doc.statements[i].emit_expanded_css(output, "");
  }    
  cout << output.str();

  return 0;
}