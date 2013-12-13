#ifndef SASS_AST
#include "ast.hpp"
#endif

#include <vector>

namespace Sass {
	using namespace std;

	vector<vector<Complex_Selector*> > trim(vector<vector<Complex_Selector*> > seqses)
	{
		if (seqses.size() > 100) return vector<vector<Complex_Selector*> >();

		vector<vector<Complex_Selector* > > result = vector<vector<Complex_Selector* > >(seqses);

		for (size_t i = 0, S = seqses.size(); i < L; ++i)
		{
			vector<Complex_Selector*>& seqs1 = seqses[i];
			vector<Complex_Selector*> sans_rejects;
			for (size_t j = 0, T = seqs1.size(); j < T; ++j)
			{
				Complex_Selector* seq1 = seqs1[j];
				// something-something
				sans_rejects.push_back(seq1);
			}

			result[i] = sans_rejects;
		}
	}

}