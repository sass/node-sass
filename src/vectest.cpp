#include <vector>
#include <iostream>

using namespace std;

struct box {
  vector<int> vec;
  vector<box> vec2;
};

int main() {
  box b;
  
  b.vec.push_back(1);
  b.vec.push_back(2);
  b.vec.push_back(3);
  
  box c = b;
  c.vec.push_back(4);
  
  cout << b.vec.size() << endl;
  cout << c.vec.size() << endl;
  
  c.vec[0] = 42;
  cout << b.vec[0] << endl;
  cout << c.vec[0] << endl;
  
  b.vec2.push_back(c);
  cout << b.vec2[0].vec[0] << endl;
  
  return 0;
}

