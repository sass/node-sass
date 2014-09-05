#include <deque>
#include <iostream>

#ifndef SASS_AST
#include "ast.hpp"
#endif

#include "node.hpp"


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
  Node lcs(const Node& x, const Node& y, const ComparatorType& comparator, Context& ctx);
  

}