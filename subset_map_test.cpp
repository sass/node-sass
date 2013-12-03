#include <string>
#include <iostream>
#include "subset_map.hpp"

using namespace std;
using namespace Sass;

Subset_Map<int, string> ssm;

int main()
{
	vector<int> s1;
	s1.push_back(1);
	s1.push_back(2);

	vector<int> s2;
	s2.push_back(2);
	s2.push_back(3);

	vector<int> s3;
	s3.push_back(3);
	s3.push_back(4);

	ssm.put(s1, "value1");
	ssm.put(s2, "value2");
	ssm.put(s3, "value3");

	vector<int> s4;
	s4.push_back(1);
	s4.push_back(2);
	s4.push_back(3);

	vector<pair<string, vector<int> > > fetched(ssm.get_kv(s4));

	cout << "PRINTING RESULTS:" << endl;
	for (size_t i = 0, S = fetched.size(); i < S; ++i) {
		cout << fetched[i].first << endl;
	}

	return 0;
}