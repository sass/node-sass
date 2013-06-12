#include <string>

namespace Sass {
  using namespace std;

	enum Unit { IN, CM, MM, PT, PC, INCOMMENSURABLE };
	extern double conversion_factors[5][5];
  Unit string_to_unit(const string&);
  double convertible(const string&, const string&);
  double convert(double, const string&, const string&);
}