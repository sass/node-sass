#include "extend.hpp"
#include "context.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"
#include "backtrace.hpp"
#include "paths.hpp"
#include "parser.hpp"
#include <iostream>
#include <deque>


/*
 NOTES:

 - The print* functions print to cerr. This allows our testing frameworks (like sass-spec) to ignore the output, which
   is very helpful when debugging. The format of the output is mainly to wrap things in square brackets to match what
   ruby already outputs (to make comparisons easier).

 - For the direct porting effort, we're trying to port method-for-method until we get all the tests passing.
   Where applicable, I've tried to include the ruby code above the function for reference until all our tests pass.
   The ruby code isn't always directly portable, so I've tried to include any modified ruby code that was actually
   used for the porting.

 - DO NOT try to optimize yet. We get a tremendous benefit out of comparing the output of each stage of the extend to the ruby
   output at the same stage. This makes it much easier to determine where problems are. Try to keep as close to
   the ruby code as you can until we have all the sass-spec tests passing. Then, we should optimize. However, if you see
   something that could probably be optimized, let's not forget it. Add a // TODO: or // IMPROVEMENT: comment.
 
 - Coding conventions in this file (these may need to be changed before merging back into master)
   - Very basic hungarian notation:
     p prefix for pointers (pSelector)
     no prefix for value types and references (selector)
   - Use STL iterators where possible
   - prefer verbose naming over terse naming
   - use typedefs for STL container types for make maintenance easier
 
 - You may see a lot of comments that say "// TODO: is this the correct combinator?". See the comment referring to combinators
   in extendCompoundSelector for a more extensive explanation of my confusion. I think our divergence in data model from ruby
   sass causes this to be necessary.
 

 GLOBAL TODOS:
 
 - wrap the contents of the print functions in DEBUG preprocesser conditionals so they will be optimized away in non-debug mode.
 
 - consider making the extend* functions member functions to avoid passing around ctx and subsetMap map around. This has the
   drawback that the implementation details of the operator are then exposed to the outside world, which is not ideal and
   can cause additional compile time dependencies.
 
 - mark the helper methods in this file static to given them compilation unit linkage.
 
 - implement parent directive matching
 
 - fix compilation warnings for unused Extend members if we really don't need those references anymore.
 */


namespace Sass {

  typedef deque<Complex_Selector*> ComplexSelectorDeque;
	typedef deque<ComplexSelectorDeque> ComplexSelectorDequeDeque;
  
  typedef pair<Complex_Selector*, Compound_Selector*> ExtensionPair;
  typedef vector<ExtensionPair> SubsetMapEntries;

  
  // Create a Selector_List* from a ComplexSelectorDeque
  // Complex_Selectors are NOT cloned.
  Selector_List* createSelectorListFromDeque(ComplexSelectorDeque& deque, Context& ctx, Selector_List* pSelectorGroupTemplate) {
    Selector_List* pSelectorGroup = new (ctx.mem) Selector_List(pSelectorGroupTemplate->path(), pSelectorGroupTemplate->position(), pSelectorGroupTemplate->length());
    for (ComplexSelectorDeque::iterator iterator = deque.begin(), iteratorEnd = deque.end(); iterator != iteratorEnd; ++iterator) {
      *pSelectorGroup << *iterator;
    }
    return pSelectorGroup;
  }


  // Take the Complex_Selector pointers in Selector_List and append all of them to the passed in deque.
  // Complex_Selectors are NOT cloned.
  void fillDequeFromSelectorList(ComplexSelectorDeque& deque, Selector_List* pSelectorList) {
    for (size_t index = 0, length = pSelectorList->length(); index < length; index++) {
      deque.push_back((*pSelectorList)[index]);
    }
  }

  
  // Print a string representation of a Compound_Selector
  void printCompoundSelector(Compound_Selector* pCompoundSelector, const char* message=NULL, bool newline=true) {
		To_String to_string;
  	if (message) {
    	cerr << message;
    }

    cerr << "(" << (pCompoundSelector ? pCompoundSelector->perform(&to_string) : "NULL") << ")";

		if (newline) {
    	cerr << endl;
    }
  }

  
  // Print a string representation of a Complex_Selector
  void printComplexSelector(Complex_Selector* pComplexSelector, const char* message=NULL, bool newline=true) {
		To_String to_string;

  	if (message) {
    	cerr << message;
    }

    cerr << "[";
    Complex_Selector* pIter = pComplexSelector;
    while (pIter) {
      if (pIter != pComplexSelector) {
        cerr << ", ";
      }
      cerr << pIter->head()->perform(&to_string);
      pIter = pIter->tail();
    }
    cerr << "]";

		if (newline) {
    	cerr << endl;
    }
  }

  
  // Print a string representation of a ComplexSelectorDeque
  void printComplexSelectorDeque(ComplexSelectorDeque& deque, const char* message=NULL, bool newline=true) {
  	To_String to_string;

  	if (message) {
    	cerr << message;
    }
    
    cerr << "[";
    for (ComplexSelectorDeque::iterator iterator = deque.begin(), iteratorEnd = deque.end(); iterator != iteratorEnd; ++iterator) {
      Complex_Selector* pComplexSelector = *iterator;
      if (iterator != deque.begin()) {
      	cerr << ", ";
      }
      printComplexSelector(pComplexSelector, "", false /*newline*/);
    }
    cerr << "]";
    
    if (newline) {
      cerr << endl;
    }
  }
 
  
  // Print a string representation of a ComplexSelectorDequeDeque
  void printComplexSelectorDequeDeque(ComplexSelectorDequeDeque& dequeDeque, const char* message=NULL, bool newline=true) {
  	To_String to_string;
    
  	if (message) {
    	cerr << message;
    }
    
    cerr << "[";
    for (ComplexSelectorDequeDeque::iterator iterator = dequeDeque.begin(), iteratorEnd = dequeDeque.end(); iterator != iteratorEnd; ++iterator) {
      ComplexSelectorDeque& deque = *iterator;
      if (iterator != dequeDeque.begin()) {
      	cerr << ", ";
      }
      printComplexSelectorDeque(deque, "", false /*newline*/);
    }
    cerr << "]";
    
    if (newline) {
      cerr << endl;
    }
  }

  
  // Print a string representation of a SourcesSet
  void printSourcesSet(SourcesSet& sources, const char* message=NULL, bool newline=true) {
  	To_String to_string;
    
  	if (message) {
    	cerr << message;
    }

    cerr << "SourcesSet[";
    for (SourcesSet::iterator iterator = sources.begin(), iteratorEnd = sources.end(); iterator != iteratorEnd; ++iterator) {
      Complex_Selector* pSource = *iterator;
      if (iterator != sources.begin()) {
      	cerr << ", ";
      }
      printComplexSelector(pSource, "", false /*newline*/);
    }
    cerr << "]";
    
    if (newline) {
      cerr << endl;
    }
  }

  
  // Clone the source ComplexSelectorDeque into dest. This WILL clone the Complex_Selectors.
  void cloneComplexSelectorDeque(ComplexSelectorDeque& source, ComplexSelectorDeque& dest, Context& ctx) {
    for (ComplexSelectorDeque::iterator iterator = source.begin(), iteratorEnd = source.end(); iterator != iteratorEnd; ++iterator) {
      Complex_Selector* pComplexSelector = *iterator;
			dest.push_back(pComplexSelector->clone(ctx));
    }
  }
  
  
  // Clone the source ComplexSelectorDequeDeque into dest. This WILL clone the Complex_Selectors.
  void cloneComplexSelectorDequeDeque(ComplexSelectorDequeDeque& source, ComplexSelectorDequeDeque& dest, Context& ctx) {
    for (ComplexSelectorDequeDeque::iterator iterator = source.begin(), iteratorEnd = source.end(); iterator != iteratorEnd; ++iterator) {

      ComplexSelectorDeque& toClone = *iterator;
      
      ComplexSelectorDeque cloned;
      cloneComplexSelectorDeque(toClone, cloned, ctx);

			dest.push_back(cloned);
    }
  }
  
  
  // Compare two ComplexSelectorDeques to see if they are equivalent. This uses the Complex_Selector operator< so it will compare
  // the Complex_Selector's contents instead of just the pointers.
  bool complexSelectorDequesEqual(ComplexSelectorDeque& one, ComplexSelectorDeque& two) {
    if (one.size() != two.size()) {
      return false;
    }
    
    for (int index = 0; index < one.size(); index++) {
      Complex_Selector* pOne = one[index];
      Complex_Selector* pTwo = two[index];

      if (*pOne < *pTwo || *pTwo < *pOne) {
        return false;
      }
    }
    
    return true;
  }
  

  /* This is the equivalent of ruby's Sequence.trim.
   
   The following is the modified version of the ruby code that was more portable to C++. You
   should be able to drop it into ruby 3.2.19 and get the same results from ruby sass.

        # Avoid truly horrific quadratic behavior. TODO: I think there
        # may be a way to get perfect trimming without going quadratic.
        return seqses if seqses.size > 100

        # Keep the results in a separate array so we can be sure we aren't
        # comparing against an already-trimmed selector. This ensures that two
        # identical selectors don't mutually trim one another.
        result = seqses.dup

        # This is n^2 on the sequences, but only comparing between
        # separate sequences should limit the quadratic behavior.
        seqses.each_with_index do |seqs1, i|
          tempResult = []

          for seq1 in seqs1 do
            max_spec = 0
            for seq in _sources(seq1) do
              max_spec = [max_spec, seq.specificity].max
            end


            isMoreSpecificOuter = false
            for seqs2 in result do
              if seqs1.equal?(seqs2) then
                next
              end

              # Second Law of Extend: the specificity of a generated selector
              # should never be less than the specificity of the extending
              # selector.
              #
              # See https://github.com/nex3/sass/issues/324.
              isMoreSpecificInner = false
              for seq2 in seqs2 do
                isMoreSpecificInner = _specificity(seq2) >= max_spec && _superselector?(seq2, seq1)
                if isMoreSpecificInner then
                  break
                end
              end
              
              if isMoreSpecificInner then
                isMoreSpecificOuter = true
                break
              end
            end

            if !isMoreSpecificOuter then
              tempResult.push(seq1)
            end
          end

          result[i] = tempResult

        end

        result
   */
  /*
   - IMPROVEMENT: We could probably work directly in the output trimmed deque.
   */
  void trim(ComplexSelectorDequeDeque& toTrim, ComplexSelectorDequeDeque& trimmed, Context& ctx) {
    // See the comments in the above ruby code before embarking on understanding this function.

    // Avoid poor performance in extreme cases.
    if (toTrim.size() > 100) {
    	trimmed = toTrim;
      return;
    }

    // Copy the input to a temporary result so we can modify it without
    ComplexSelectorDequeDeque result;
    cloneComplexSelectorDequeDeque(toTrim, result, ctx);
    
    // Normally we use the standard STL iterators, but in this case, we need to access the result collection by index since we're
    // iterating the input collection, computing a value, and then setting the result in the output collection. We have to keep track
    // of the index manually.
    int resultIndex = 0;

    for (ComplexSelectorDequeDeque::iterator toTrimIterator = toTrim.begin(), toTrimIteratorEnd = toTrim.end(); toTrimIterator != toTrimIteratorEnd; ++toTrimIterator) {
    	ComplexSelectorDeque& seqs1 = *toTrimIterator;
      
      ComplexSelectorDeque tempResult;

      for (ComplexSelectorDeque::iterator seqs1Iterator = seqs1.begin(), seqs1IteratorEnd = seqs1.end(); seqs1Iterator != seqs1IteratorEnd; ++seqs1Iterator) {
       	Complex_Selector* pSeq1 = *seqs1Iterator;
  
        // Compute the maximum specificity. This requires looking at the "sources" of the sequence. See SimpleSequence.sources in the ruby code
        // for a good description of sources.
        //
        // TODO: I'm pretty sure there's a bug in the sources code. It was implemented for sass-spec's 182_test_nested_extend_loop test.
        // While the test passes, I compared the state of each trim call to verify correctness. The last trim call had incorrect sources. We
        // had an extra source that the ruby version did not have. Without a failing test case, this is going to be extra hard to find. My
        // best guess at this point is that we're cloning an object somewhere and maintaining the sources when we shouldn't be. This is purely
        // a guess though.
        int maxSpecificity = 0;
        SourcesSet sources = pSeq1->sources();

//        printComplexSelector(pSeq1, "TRIMASDF SEQ1: ");
//        printSourcesSet(sources, "TRIMASDF SOURCES: ");

        for (SourcesSet::iterator sourcesSetIterator = sources.begin(), sourcesSetIteratorEnd = sources.end(); sourcesSetIterator != sourcesSetIteratorEnd; ++sourcesSetIterator) {
         	const Complex_Selector* const pCurrentSelector = *sourcesSetIterator;
          maxSpecificity = max(maxSpecificity, pCurrentSelector->specificity());
        }
        
//        cerr << "MAX SPECIFIITY: " << maxSpecificity << endl;

        bool isMoreSpecificOuter = false;

        for (ComplexSelectorDequeDeque::iterator resultIterator = result.begin(), resultIteratorEnd = result.end(); resultIterator != resultIteratorEnd; ++resultIterator) {
          ComplexSelectorDeque& seqs2 = *resultIterator;

//          printSelectors(seqs1, "SEQS1: ");
//          printSelectors(seqs2, "SEQS2: ");

          if (complexSelectorDequesEqual(seqs1, seqs2)) {
//            cerr << "CONTINUE" << endl;
            continue;
          }
          
          bool isMoreSpecificInner = false;
          
          for (ComplexSelectorDeque::iterator seqs2Iterator = seqs2.begin(), seqs2IteratorEnd = seqs2.end(); seqs2Iterator != seqs2IteratorEnd; ++seqs2Iterator) {
            Complex_Selector* pSeq2 = *seqs2Iterator;
            
//            cerr << "SEQ2 SPEC: " << pSeq2->specificity() << endl;
//            cerr << "IS SUPER: " << pSeq2->is_superselector_of(pSeq1) << endl;
            
            isMoreSpecificInner = pSeq2->specificity() >= maxSpecificity && pSeq2->is_superselector_of(pSeq1);

            if (isMoreSpecificInner) {
//              cerr << "FOUND MORE SPECIFIC" << endl;
              break;
            }
          }
          
          // If we found something more specific, we're done. Let the outer loop know and stop iterating.
					if (isMoreSpecificInner) {
            isMoreSpecificOuter = true;
            break;
          }
        }
        
        if (!isMoreSpecificOuter) {
//          cerr << "PUSHING" << endl;
          tempResult.push_back(pSeq1);
        }
      }

      result[resultIndex] = tempResult;

      resultIndex++;
    }
    
    
    trimmed = result;
  }
  
  
  
  /* This is the equivalent of ruby's Sequence.subweave.

    Here is the original subweave code for reference during porting.

      def subweave(seq1, seq2)
        return [seq2] if seq1.empty?
        return [seq1] if seq2.empty?

        seq1, seq2 = seq1.dup, seq2.dup
        return unless init = merge_initial_ops(seq1, seq2)
        return unless fin = merge_final_ops(seq1, seq2)
        seq1 = group_selectors(seq1)
        seq2 = group_selectors(seq2)
        lcs = Sass::Util.lcs(seq2, seq1) do |s1, s2|
          next s1 if s1 == s2
          next unless s1.first.is_a?(SimpleSequence) && s2.first.is_a?(SimpleSequence)
          next s2 if parent_superselector?(s1, s2)
          next s1 if parent_superselector?(s2, s1)
        end

        diff = [[init]]
        until lcs.empty?
          diff << chunks(seq1, seq2) {|s| parent_superselector?(s.first, lcs.first)} << [lcs.shift]
          seq1.shift
          seq2.shift
        end
        diff << chunks(seq1, seq2) {|s| s.empty?}
        diff += fin.map {|sel| sel.is_a?(Array) ? sel : [sel]}
        diff.reject! {|c| c.empty?}

        result = Sass::Util.paths(diff).map {|p| p.flatten}.reject {|p| path_has_two_subjects?(p)}

        result
      end
	*/
  /*
   TODO:
   - replace this code with the equivalent of the ruby code in Sequence.subweave
   - see if we need to split apart combinator and complex selector before calling this method
   */
	void subweave(Complex_Selector* pOne, Complex_Selector* pTwo, ComplexSelectorDeque& out, Context& ctx) {
    // Check for the simple cases
    if (pOne == NULL) {
    	out.push_back(pTwo ? pTwo->clone(ctx) : NULL);
      return;
    }
		if (pTwo == NULL) {
    	out.push_back(pOne ? pOne->clone(ctx) : NULL);
      return;
    }


    // Do the naive implementation. pOne = A B and pTwo = C D ...yields...  A B C D and C D A B
    // See https://gist.github.com/nex3/7609394 for details.
    Complex_Selector* pFirstPermutation = pOne->clone(ctx);
    pFirstPermutation->set_innermost(pTwo->clone(ctx), pFirstPermutation->innermost()->combinator()); // TODO: is this the correct combinator?
    out.push_back(pFirstPermutation);

    Complex_Selector* pSecondPermutation = pTwo->clone(ctx);
    pSecondPermutation->set_innermost(pOne->clone(ctx), pSecondPermutation->innermost()->combinator()); // TODO: is this the correct combinator?
    out.push_back(pSecondPermutation);
  }
  

  /* This is the equivalent of ruby's Sequence.weave.
   
   The following is the modified version of the ruby code that was more portable to C++. You
   should be able to drop it into ruby 3.2.19 and get the same results from ruby sass.

      def weave(path)
        # This function works by moving through the selector path left-to-right,
        # building all possible prefixes simultaneously. These prefixes are
        # `befores`, while the remaining parenthesized suffixes is `afters`.
        befores = [[]]
        afters = path.dup

        until afters.empty?
          current = afters.shift.dup
          last_current = [current.pop]

          tempResult = []

          for before in befores do
            sub = subweave(before, current)
            if sub.nil?
              next
            end

            for seqs in sub do
              tempResult.push(seqs + last_current)
            end
          end

          befores = tempResult

        end

        return befores
      end
 	*/
void weave(ComplexSelectorDeque& toWeave, Context& ctx, ComplexSelectorDeque& weaved /*out*/) {

    ComplexSelectorDeque befores;
  	befores.push_back(NULL); // this push is necessary for the befores iteration below to do anything. This matches the ruby code initializing befores to [[]].

    ComplexSelectorDeque afters;
    cloneComplexSelectorDeque(toWeave, afters, ctx);

    while (afters.size() > 0) {
			Complex_Selector* pCurrent = afters[0]->clone(ctx);
      afters.pop_front();

      Complex_Selector* pLastCurrent = pCurrent->innermost();
      if (pCurrent == pLastCurrent) {
        pCurrent = NULL;
      } else {
        // TODO: consider adding popComplexSelector and shiftComplexSelector to since this seems like general functionality that would be useful.
        Complex_Selector* pIter = pCurrent;
        while (pIter) {
          if (pIter->tail() && !pIter->tail()->tail()) {
            pIter->tail(NULL);
            break;
          }
          
        	pIter = pIter->tail();
        }
      }
          
      ComplexSelectorDeque collector; // TODO: figure out what to name this. A name with more context?

      for (ComplexSelectorDeque::iterator iterator = befores.begin(), endIterator = befores.end();
           iterator != endIterator; ++iterator) {
        
        Complex_Selector* pBefore = *iterator;
        
        ComplexSelectorDeque sub;
        subweave(pBefore, pCurrent, sub, ctx);
        
        if (sub.empty()) {
          continue;
        }

        for (ComplexSelectorDeque::iterator iterator = sub.begin(), endIterator = sub.end();
             iterator != endIterator; ++iterator) {
          
          Complex_Selector* pSequences = *iterator; // TODO: clone this?
          
          if (pSequences) {
          	pSequences->set_innermost(pLastCurrent->clone(ctx), pSequences->innermost()->combinator()); // TODO: is this the correct combinator?
         	} else {
						pSequences = pLastCurrent->clone(ctx);
          }
          
          collector.push_back(pSequences);
        }
      }
      
    	befores = collector;
    }

  	weaved = befores;
  }
  


  /* This is the equivalent of ruby's Sass::Util.paths.
   
   The following is the modified version of the ruby code that was more portable to C++. You
   should be able to drop it into ruby 3.2.19 and get the same results from ruby sass.

    # Return an array of all possible paths through the given arrays.
    #
    # @param arrs [Array<Array>]
    # @return [Array<Arrays>]
    #
    # @example
    #   paths([[1, 2], [3, 4], [5]]) #=>
    #     # [[1, 3, 5],
    #     #  [2, 3, 5],
    #     #  [1, 4, 5],
    #     #  [2, 4, 5]]
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
  void paths(ComplexSelectorDequeDeque& source, ComplexSelectorDequeDeque& out, Context& ctx) {
    To_String to_string;
    
    ComplexSelectorDequeDeque loopStart;

    for (ComplexSelectorDequeDeque::iterator arrsIterator = source.begin(), endIterator = source.end();
         arrsIterator != endIterator; ++arrsIterator) {
      
      ComplexSelectorDeque& arr = *arrsIterator;
      
    	ComplexSelectorDequeDeque permutations;

      for (ComplexSelectorDeque::iterator arrIterator = arr.begin(), endIterator = arr.end();
           arrIterator != endIterator; ++arrIterator) {
      	Complex_Selector* pE = (*arrIterator)->clone(ctx);
    
        if (loopStart.size() == 0) {
          // When the loopStart has nothing in it, we're on the first iteration. The new permutation
          // is just the current Complex_Selector*. Without this special case, we would never loop
          // over anything in the for loop in the below else clause.
          ComplexSelectorDeque newPermutation;
          newPermutation.push_back(pE);
          
          permutations.push_back(newPermutation);
        } else {
          for (ComplexSelectorDequeDeque::iterator loopStartIterator = loopStart.begin(), endIterator = loopStart.end();
               loopStartIterator != endIterator; ++loopStartIterator) {
            ComplexSelectorDeque& path = *loopStartIterator;
            
            ComplexSelectorDeque newPermutation;
            cloneComplexSelectorDeque(path, newPermutation, ctx);
            newPermutation.push_back(pE);
            
            permutations.push_back(newPermutation);
          }
        }
        
      }
      
      loopStart = permutations;
    }
    
    out = loopStart;
  }

  

  // complexSelectorDequeContains checks if an equivalent Complex_Selector to the one passed in is contained within the
  // passed in ComplexSelectorDeque. This is necessary because the deque contains pointers, and pointer comparison yields
  // strict object equivalency. We want to compare the selector's contents.
  //
  // TODO: move ComplexSelectorPointerComparator to ast.hpp next to the other one aimed at set usage? This is aimed at usage for std::find_if in complexSelectorDequeContains. One could be implemented in terms of the other for less code duplication.
  struct ComplexSelectorPointerComparator
  {
    bool operator()(Complex_Selector* const pOne)
    {
      return (!(*pOne < *pTwo) && !(*pTwo < *pOne));
    }
    Complex_Selector* pTwo;
  };
  bool complexSelectorDequeContainsImpl(const Complex_Selector& one, const Complex_Selector* pTwo) {
    return (!(one < *pTwo) && !(*pTwo < one));
  }
  bool complexSelectorDequeContains(ComplexSelectorDeque& deque, Complex_Selector* pComplexSelector) {
    ComplexSelectorPointerComparator comparator = { pComplexSelector };
    
    return std::find_if(
      deque.begin(),
      deque.end(),
      comparator
      ) != deque.end();
  }
  
  

  // This forward declaration is needed since extendComplexSelector calls extendCompoundSelector, which may recursively
  // call extendComplexSelector again.
  void extendComplexSelector(
    Complex_Selector* pComplexSelector,
    Context& ctx,
    ExtensionSubsetMap& subsetMap,
    set<Compound_Selector> seen,
    ComplexSelectorDeque& extendedSelectors);
  
  
  
  /* This is the equivalent of ruby's SimpleSequence.do_extend.
   
    // TODO: I think I have some modified ruby code to put here. Check.
  */
  /*
   ISSUES:
   - Previous TODO: Do we need to group the results by extender?
   - What does subject do in?: next unless unified = seq.members.last.unify(self_without_sel, subject?)
   - IMPROVEMENT: The search for uniqueness at the end is not ideal since it's has to loop over everything...
   - IMPROVEMENT: Check if the final search for uniqueness is doing anything that extendComplexSelector isn't already doing...
   */
  void extendCompoundSelector(
  	Compound_Selector* pSelector,
    Context& ctx,
    ExtensionSubsetMap& subsetMap,
    set<Compound_Selector> seen,
    ComplexSelectorDeque& extendedSelectors) {

    To_String to_string;

    SubsetMapEntries entries = subsetMap.get_v(pSelector->to_str_vec());
    
		for (SubsetMapEntries::iterator iterator = entries.begin(), endIterator = entries.end(); iterator != endIterator; ++iterator) {
      Complex_Selector* pExtComplexSelector = iterator->first;    // The selector up to where the @extend is (ie, the thing to merge)
      Compound_Selector* pExtCompoundSelector = iterator->second; // The stuff after the @extend
      
      // I know I said don't optimize yet, but I couldn't help moving this if check up until we find a reason not
      // to. In the ruby code, this was done at the end after a lot of work was already done. The only reason this
      // wouldn't be safe is if there are side effects in the skipped ruby code that the algorithm is relying on. In that
      // case, we wouldn't have those side effects without moving this lower in this function to match it's placement in
      // the ruby code. It's easy enough to change if this causes a problem.
      if (seen.find(*pExtCompoundSelector) != seen.end()) {
        continue;
      }
      
      // TODO: This can return a Compound_Selector with no elements. Should that just be returning NULL?
      Compound_Selector* pSelectorWithoutExtendSelectors = pSelector->minus(pExtCompoundSelector, ctx);


      Compound_Selector* pInnermostCompoundSelector = pExtComplexSelector->base();
      Compound_Selector* pUnifiedSelector = NULL;

			if (!pInnermostCompoundSelector) {
        pInnermostCompoundSelector = new (ctx.mem) Compound_Selector(pSelector->path(), pSelector->position());
      }

      if (pInnermostCompoundSelector->length() == 0) {
        pUnifiedSelector = pSelectorWithoutExtendSelectors;
      } else if (pSelectorWithoutExtendSelectors->length() == 0) {
      	pUnifiedSelector = pInnermostCompoundSelector;
      } else {
        pUnifiedSelector = pInnermostCompoundSelector->unify_with(pSelectorWithoutExtendSelectors, ctx);
      }
      
      if (!pUnifiedSelector || pUnifiedSelector->length() == 0) {
        continue;
      }
      
      
      // TODO: implement the parent directive match (if necessary based on test failures)
      // next if group.map {|e, _| check_directives_match!(e, parent_directives)}.none?
      

      // TODO: This seems a little fishy to me. See if it causes any problems. From the ruby, we should be able to just
      // get rid of the last Compound_Selector and replace it with this one. I think the reason this code is more
      // complex is that Complex_Selector contains a combinator, but in ruby combinators have already been filtered
      // out and aren't operated on.
      Complex_Selector* pNewSelector = pExtComplexSelector->clone(ctx);
      Complex_Selector* pNewInnerMost = new (ctx.mem) Complex_Selector(pSelector->path(), pSelector->position(), Complex_Selector::ANCESTOR_OF, pUnifiedSelector, NULL);
      Complex_Selector::Combinator combinator = pNewSelector->clear_innermost();
      pNewSelector->set_innermost(pNewInnerMost, combinator);


      // Set the sources on our new Complex_Selector to the sources of this simple sequence plus the thing we're extending.
//      printComplexSelector(pNewSelector, "ASDF SETTING ON: ");

      SourcesSet newSourcesSet = pSelector->sources();
//      printSourcesSet(newSourcesSet, "ASDF SOURCES THIS: ");
      newSourcesSet.insert(pExtComplexSelector->clone(ctx));
//      printSourcesSet(newSourcesSet, "ASDF NEW: ");
      pNewSelector->addSources(newSourcesSet, ctx);
      
//      SourcesSet newSet = pNewSelector->sources();
//      printSourcesSet(newSet, "ASDF NEW AFTER SET: ");
//      printSourcesSet(pSelector->sources(), "ASDF SOURCES THIS SHOULD BE SAME: ");


      ComplexSelectorDeque recurseExtendedSelectors;
      set<Compound_Selector> recurseSeen(seen);
      recurseSeen.insert(*pExtCompoundSelector);


			printComplexSelector(pNewSelector, "RECURSING DO EXTEND: ", true);
  		extendComplexSelector(pNewSelector, ctx, subsetMap, recurseSeen, recurseExtendedSelectors /*out*/);


      for (ComplexSelectorDeque::iterator iterator = recurseExtendedSelectors.begin(), endIterator = recurseExtendedSelectors.end();
           iterator != endIterator; ++iterator) {
        Complex_Selector* pSelector = *iterator;
        bool selectorAlreadyExists = complexSelectorDequeContains(extendedSelectors, pSelector);
        if (!selectorAlreadyExists) {
          extendedSelectors.push_back(pSelector);
        }
      }
    }
  }
 

  
  /* This is the equivalent of ruby's Sequence.do_extend.
   
   // TODO: I think I have some modified ruby code to put here. Check.
   */
  /*
   ISSUES:
   - check to automatically include combinators doesn't transfer over to libsass' data model where
     the combinator and compound selector are one unit
     next [[sseq_or_op]] unless sseq_or_op.is_a?(SimpleSequence)
   */
  void extendComplexSelector(
  	Complex_Selector* pComplexSelector,
    Context& ctx,
    ExtensionSubsetMap& subsetMap,
    set<Compound_Selector> seen,
    ComplexSelectorDeque& extendedSelectors) {

    To_String to_string;

    Complex_Selector* pCurrentComplexSelector = pComplexSelector->tail();
    
    ComplexSelectorDequeDeque extendedNotExpanded;
    
    while(pCurrentComplexSelector)
    {
      Compound_Selector* pCompoundSelector = pCurrentComplexSelector->head();

      ComplexSelectorDeque extended;
      extendCompoundSelector(pCompoundSelector, ctx, subsetMap, seen, extended /*out*/);
      printComplexSelectorDeque(extended, "EXTENDED: ");

      
      // Prepend the Compound_Selector based on the choices logic; choices seems to be extend but with an ruby Array instead of a Sequence
      // due to the member mapping: choices = extended.map {|seq| seq.members}
      Complex_Selector* pJustCurrentCompoundSelector = pCurrentComplexSelector->clone(ctx);
      pJustCurrentCompoundSelector->tail(NULL);

      bool isSuperselector = false;
      for (ComplexSelectorDeque::iterator iterator = extended.begin(), endIterator = extended.end();
           iterator != endIterator; ++iterator) {
        Complex_Selector* pExtensionSelector = *iterator;
      	if (pExtensionSelector->is_superselector_of(pJustCurrentCompoundSelector)) {
          isSuperselector = true;
          break;
        }
      }

      if (!isSuperselector) {
        extended.push_front(pJustCurrentCompoundSelector);
      }

      printComplexSelectorDeque(extended, "CHOICES UNSHIFTED: ");

      
      // Aggregate our current extensions
      extendedNotExpanded.push_back(extended);
    

      // Continue our iteration
    	pCurrentComplexSelector = pCurrentComplexSelector->tail();
    }


    printComplexSelectorDequeDeque(extendedNotExpanded, "EXTENDED NOT EXPANDED: ");
    
  
    // Ruby Equivalent: paths
    ComplexSelectorDequeDeque permutations;
    paths(extendedNotExpanded, permutations, ctx);
    printComplexSelectorDequeDeque(permutations, "PATHS: ");


    // Ruby Equivalent: weave
 		ComplexSelectorDequeDeque weaves;
    for (ComplexSelectorDequeDeque::iterator iterator = permutations.begin(), endIterator = permutations.end();
         iterator != endIterator; ++iterator) {
      ComplexSelectorDeque& toWeave = *iterator;
      
      ComplexSelectorDeque weaved;
			weave(toWeave, ctx, weaved);
      
      weaves.push_back(weaved);
    }
    
    printComplexSelectorDequeDeque(weaves, "WEAVES: ");
    
    
    // Ruby Equivalent: trim
    ComplexSelectorDequeDeque trimmed;
    trim(weaves, trimmed, ctx);
		printComplexSelectorDequeDeque(trimmed, "TRIMMED: ");

    
    // Ruby Equivalent: flatten
    for (ComplexSelectorDequeDeque::iterator iterator = trimmed.begin(), endIterator = trimmed.end();
         iterator != endIterator; ++iterator) {
      
      ComplexSelectorDeque& toCombine = *iterator;
      extendedSelectors.insert(extendedSelectors.end(), toCombine.begin(), toCombine.end());
    }
    

    printComplexSelectorDeque(extendedSelectors, ">>>>> EXTENDED: ");
  }



  /*
   This is the equivalent of ruby's CommaSequence.do_extend.
  */
    /*
     ISSUES:
     - Improvement: searching through deque with std::find is probably slow
     - Improvement: can we just use one deque?
     */
  Selector_List* extendSelectorList(Selector_List* pSelectorList, Context& ctx, ExtensionSubsetMap& subsetMap) {

    To_String to_string;

    ComplexSelectorDeque newSelectors;

    for (size_t index = 0, length = pSelectorList->length(); index < length; index++) {
      Complex_Selector* pSelector = (*pSelectorList)[index];

      ComplexSelectorDeque extendedSelectors;
      set<Compound_Selector> seen;
      extendComplexSelector(pSelector, ctx, subsetMap, seen, extendedSelectors /*out*/);
      
      if (!pSelector->has_placeholder()) {
        bool selectorAlreadyExists = complexSelectorDequeContains(extendedSelectors, pSelector);
        if (!selectorAlreadyExists) {
        	extendedSelectors.push_front(pSelector);
        }
      }
  
      newSelectors.insert(newSelectors.end(), extendedSelectors.begin(), extendedSelectors.end());
    }
    
    return createSelectorListFromDeque(newSelectors, ctx, pSelectorList);
  }

  
  // Extend a ruleset by extending the selectors and updating them on the ruleset. The block's rules don't need to change.
  void extendRuleset(Ruleset* pRuleset, Context& ctx, ExtensionSubsetMap& subsetMap) {
    To_String to_string;

    Selector_List* pNewSelectorList = extendSelectorList(static_cast<Selector_List*>(pRuleset->selector()), ctx, subsetMap);

    if (pNewSelectorList) {
      // re-parse in order to restructure expanded placeholder nodes correctly.
      //
      // TODO: I don't know if this is needed, but it was in the original C++ implementation, so I kept it. Try running the tests without re-parsing.
      pRuleset->selector(
        Parser::from_c_str(
          (pNewSelectorList->perform(&to_string) + ";").c_str(),
          ctx,
          pNewSelectorList->path(),
          pNewSelectorList->position()
        ).parse_selector_group()
      );
    }
  }
  
  

  Extend::Extend(Context& ctx, Extensions& extensions, ExtensionSubsetMap& ssm, Backtrace* bt)
  : ctx(ctx), extensions(extensions), subset_map(ssm), backtrace(bt)
  { }

  void Extend::operator()(Block* b)
  {
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      (*b)[i]->perform(this);
    }
  }

  void Extend::operator()(Ruleset* pRuleset)
  {
    // Deal with extensions in this Ruleset

    extendRuleset(pRuleset, ctx, subset_map);


    // Iterate into child blocks
    
    Block* b = pRuleset->block();

    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = (*b)[i];
      stm->perform(this);
    }
  }

  void Extend::operator()(Media_Block* m)
  {
    m->block()->perform(this);
  }

  void Extend::operator()(At_Rule* a)
  {
    if (a->block()) a->block()->perform(this);
  }
}
