#include <deque>
#include <iostream>

#ifndef SASS_AST
#include "ast.hpp"
#endif

#include "node.hpp"
#include "debug.hpp"


namespace Sass {

  
  using namespace std;


  /*
   This is for ports of functions in the Sass:Util module.
   */
  

	/*
    # Return a Node collection of all possible paths through the given Node collection of Node collections.
    #
    # @param arrs [NodeCollection<NodeCollection<Node>>]
    # @return [NodeCollection<NodeCollection<Node>>]
    #
    # @example
    #   paths([[1, 2], [3, 4], [5]]) #=>
    #     # [[1, 3, 5],
    #     #  [2, 3, 5],
    #     #  [1, 4, 5],
    #     #  [2, 4, 5]]
  */
	Node paths(const Node& arrs, Context& ctx);
  
  
  /*
  This class is a default implementation of a Node comparator that can be passed to the lcs function below.
  It uses operator== for equality comparision. It then returns one if the Nodes are equal.
  */
  class DefaultLcsComparator {
  public:
  	bool operator()(const Node& one, const Node& two, Node& out) const {
    	// TODO: Is this the correct C++ interpretation?
      // block ||= proc {|a, b| a == b && a}
      if (one == two) {
      	out = one;
        return true;
      }

      return false;
    }
  };

  
	typedef vector<vector<int> > LCSTable;
  
  
  /*
  This is the equivalent of ruby's Sass::Util.lcs_backtrace.
  
  # Computes a single longest common subsequence for arrays x and y.
  # Algorithm from http://en.wikipedia.org/wiki/Longest_common_subsequence_problem#Reading_out_an_LCS
	*/
  template<typename ComparatorType>
  Node lcs_backtrace(const LCSTable& c, const Node& x, const Node& y, int i, int j, const ComparatorType& comparator) {
  	DEBUG_PRINTLN("LCSBACK: C=" /*<< c*/ << "X=" << x << " Y=" << y << " I=" << i << " J=" << j)

  	if (i == 0 || j == 0) {
    	DEBUG_PRINTLN("RETURNING EMPTY")
    	return Node::createCollection();
    }
    
  	NodeDeque& xChildren = *(x.collection());
    NodeDeque& yChildren = *(y.collection());

    Node compareOut = Node::createNil();
    if (comparator(xChildren[i], yChildren[j], compareOut)) {
      DEBUG_PRINTLN("RETURNING AFTER ELEM COMPARE")
      Node result = lcs_backtrace(c, x, y, i - 1, j - 1, comparator);
      result.collection()->push_back(compareOut);
      return result;
    }
    
    if (c[i][j - 1] > c[i - 1][j]) {
    	DEBUG_PRINTLN("RETURNING AFTER TABLE COMPARE")
    	return lcs_backtrace(c, x, y, i, j - 1, comparator);
    }
    
    DEBUG_PRINTLN("FINAL RETURN")
    return lcs_backtrace(c, x, y, i - 1, j, comparator);
  }
  

  /*
  This is the equivalent of ruby's Sass::Util.lcs_table.
  
  # Calculates the memoization table for the Least Common Subsequence algorithm.
  # Algorithm from http://en.wikipedia.org/wiki/Longest_common_subsequence_problem#Computing_the_length_of_the_LCS
  */
  template<typename ComparatorType>
  void lcs_table(const Node& x, const Node& y, const ComparatorType& comparator, LCSTable& out) {
  	DEBUG_PRINTLN("LCSTABLE: X=" << x << " Y=" << y)

  	NodeDeque& xChildren = *(x.collection());
    NodeDeque& yChildren = *(y.collection());

  	LCSTable c(xChildren.size(), vector<int>(yChildren.size()));
    
    // These shouldn't be necessary since the vector will be initialized to 0 already.
    // x.size.times {|i| c[i][0] = 0}
    // y.size.times {|j| c[0][j] = 0}

    for (size_t i = 1; i < xChildren.size(); i++) {
    	for (size_t j = 1; j < yChildren.size(); j++) {
        Node compareOut = Node::createNil();

      	if (comparator(xChildren[i], yChildren[j], compareOut)) {
        	c[i][j] = c[i - 1][j - 1] + 1;
        } else {
        	c[i][j] = max(c[i][j - 1], c[i - 1][j]);
        }
      }
    }

    out = c;
  }


  /*
  This is the equivalent of ruby's Sass::Util.lcs.
  
  # Computes a single longest common subsequence for `x` and `y`.
  # If there are more than one longest common subsequences,
  # the one returned is that which starts first in `x`.

  # @param x [NodeCollection]
  # @param y [NodeCollection]
  # @comparator An equality check between elements of `x` and `y`.
  # @return [NodeCollection] The LCS

  http://en.wikipedia.org/wiki/Longest_common_subsequence_problem
  */
  template<typename ComparatorType>
  Node lcs(const Node& x, const Node& y, const ComparatorType& comparator, Context& ctx) {
  	DEBUG_PRINTLN("LCS: X=" << x << " Y=" << y)

    Node newX = x.clone(ctx);
    newX.collection()->push_front(Node::createNil());
    
    Node newY = y.clone(ctx);
    newY.collection()->push_front(Node::createNil());
    
    LCSTable table;
    lcs_table(newX, newY, comparator, table);
    
		return lcs_backtrace(table, newX, newY, newX.collection()->size() - 1, newY.collection()->size() - 1, comparator);
  }


	/*
  This is the equivalent of ruby sass' Sass::Util.flatten and [].flatten.
  Sass::Util.flatten requires the number of levels to flatten, while
  [].flatten doesn't and will flatten the entire array. This function
  supports both.
  
  # Flattens the first `n` nested arrays. If n == -1, all arrays will be flattened
  #
  # @param arr [NodeCollection] The array to flatten
  # @param n [int] The number of levels to flatten
  # @return [NodeCollection] The flattened array
  */
  Node flatten(const Node& arr, Context& ctx, int n = -1);


}
