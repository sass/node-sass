#ifndef SASS_AST
#include "node.hpp"
#endif

#include "to_string.hpp"


namespace Sass {


  /*
		This is the equivalent of ruby's Sass::Util.paths.
   
    # Return an array of all possible paths through the given arrays.
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
   
   The following is the modified version of the ruby code that was more portable to C++. You
   should be able to drop it into ruby 3.2.19 and get the same results from ruby sass.

    def paths(arrs)
     	// I changed the inject and maps to an iterative approach to make it easier to implement in C++
      loopStart = [[]]

      for arr in arrs do
        permutations = []
        for e in arr do
          for path in loopStart do
            permutations.push(path + [e])
          end
        end
        loopStart = permutations
      end
    end
	*/
  Node paths(const Node& arrs, Context& ctx) {
    To_String to_string;

    Node loopStart = Node::createCollection();
    loopStart.collection()->push_back(Node::createCollection());
    
    for (NodeDeque::iterator arrsIter = arrs.collection()->begin(), arrsEndIter = arrs.collection()->end();
    	arrsIter != arrsEndIter; ++arrsIter) {
      
      Node& arr = *arrsIter;
      
      Node permutations = Node::createCollection();
      
      for (NodeDeque::iterator arrIter = arr.collection()->begin(), arrIterEnd = arr.collection()->end();
      	arrIter != arrIterEnd; ++arrIter) {
        
        Node& e = *arrIter;
        
        for (NodeDeque::iterator loopStartIter = loopStart.collection()->begin(), loopStartIterEnd = loopStart.collection()->end();
          loopStartIter != loopStartIterEnd; ++loopStartIter) {
        
          Node& path = *loopStartIter;
          
          Node newPermutation = path.clone(ctx);
          newPermutation.collection()->push_back(e.clone(ctx));
          
          permutations.collection()->push_back(newPermutation);
        }
        
        loopStart = permutations;
      }
    }
    
    return loopStart;
  }

}