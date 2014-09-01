#include "extend.hpp"
#include "context.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"
#include "backtrace.hpp"
#include "paths.hpp"
#include "parser.hpp"
#include <iostream>
#include <deque>

namespace Sass {
  
  typedef deque<Complex_Selector*> ComplexSelectorDeque;
	typedef deque<ComplexSelectorDeque> ComplexSelectorDequeDeque;

  
  Selector_List* createSelectorListFromDeque(ComplexSelectorDeque& deque, Context& ctx, Selector_List* pSelectorGroupTemplate) {
    Selector_List* pSelectorGroup = new (ctx.mem) Selector_List(pSelectorGroupTemplate->path(), pSelectorGroupTemplate->position(), pSelectorGroupTemplate->length());
    for (ComplexSelectorDeque::iterator iterator = deque.begin(), iteratorEnd = deque.end(); iterator != iteratorEnd; ++iterator) {
      *pSelectorGroup << *iterator;
    }
    return pSelectorGroup;
  }
  
  void fillDequeFromSelectorList(ComplexSelectorDeque& deque, Selector_List* pSelectorList) {
    for (size_t index = 0, length = pSelectorList->length(); index < length; index++) {
      deque.push_back((*pSelectorList)[index]);
    }
  }
  
  void printCompoundSelector(Compound_Selector* pCompoundSelector, const char* message=NULL, bool newline=false) {
		To_String to_string;
  	if (message) {
    	cerr << message;
    }

    cerr << "(" << (pCompoundSelector ? pCompoundSelector->perform(&to_string) : "NULL") << ")";

		if (newline) {
    	cerr << endl;
    }
  }
  
  void printComplexSelector(Complex_Selector* pComplexSelector, const char* message=NULL, bool newline=false) {
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
  
  void printSelectors(const char* message, ComplexSelectorDeque& deque, Context& /*ctx*/, bool newline=false) {
  	To_String to_string;

  	cerr << message << "[";
    for (ComplexSelectorDeque::iterator iterator = deque.begin(), iteratorEnd = deque.end(); iterator != iteratorEnd; ++iterator) {
      Complex_Selector* pComplexSelector = *iterator;
      if (iterator != deque.begin()) {
      	cerr << ", ";
      }
      printComplexSelector(pComplexSelector);
    }
    cerr << "]";
    
    if (newline) {
      cerr << endl;
    }
  }
  
  void printComplexSelectorDequeDeque(const char* message, ComplexSelectorDequeDeque& dequeDeque, Context& ctx, bool newline=false) {
  	To_String to_string;
    
  	cerr << message << "[";
    for (ComplexSelectorDequeDeque::iterator iterator = dequeDeque.begin(), iteratorEnd = dequeDeque.end(); iterator != iteratorEnd; ++iterator) {
      ComplexSelectorDeque& deque = *iterator;
      if (iterator != dequeDeque.begin()) {
      	cerr << ", ";
      }
      printSelectors("", deque, ctx);
    }
    cerr << "]";
    
    if (newline) {
      cerr << endl;
    }
  }

  
  
  
  
  void cloneComplexSelectorDeque(ComplexSelectorDeque& source, ComplexSelectorDeque& dest, Context& ctx) {
    for (ComplexSelectorDeque::iterator iterator = source.begin(), iteratorEnd = source.end(); iterator != iteratorEnd; ++iterator) {
      Complex_Selector* pComplexSelector = *iterator;
			dest.push_back(pComplexSelector->clone(ctx));
    }
  }
  
  
  
  void cloneComplexSelectorDequeDeque(ComplexSelectorDequeDeque& source, ComplexSelectorDequeDeque& dest, Context& ctx) {
    for (ComplexSelectorDequeDeque::iterator iterator = source.begin(), iteratorEnd = source.end(); iterator != iteratorEnd; ++iterator) {

      ComplexSelectorDeque& toClone = *iterator;
      
      ComplexSelectorDeque cloned;
      cloneComplexSelectorDeque(toClone, cloned, ctx);

			dest.push_back(cloned);
    }
  }
  
  
  
  
  /*
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
          $stderr.puts "SEQS1: #{seqs1} #{i}"


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

          $stderr.puts "RESULT: #{result[i]}"
        end
        $stderr.puts "TRIM RESULT: #{result}"
        result
   */
  void trim(ComplexSelectorDequeDeque& toTrim, ComplexSelectorDequeDeque& trimmed, Context& ctx) {
    // return seqses if seqses.size > 100
    if (toTrim.size() > 100) {
    	trimmed = toTrim;
      return;
    }

    // result = seqses.dup
    ComplexSelectorDequeDeque result;
    cloneComplexSelectorDequeDeque(toTrim, result, ctx);
    
    
    // seqses.each_with_index do |seqs1, i|
    for (ComplexSelectorDequeDeque::iterator iterator = toTrim.begin(), iteratorEnd = toTrim.end(); iterator != iteratorEnd; ++iterator) {
    	ComplexSelectorDeque& seqs1 = *iterator;
      
      ComplexSelectorDeque tempResult;
      
      // for seq1 in seqs1 do
      for (ComplexSelectorDeque::iterator iterator = seqs1.begin(), iteratorEnd = seqs1.end(); iterator != iteratorEnd; ++iterator) {
       	Complex_Selector* pSeq1 = *iterator;
  
        
        // max_spec = 0
        // for seq in _sources(seq1) do
        //   max_spec = [max_spec, seq.specificity].max
        // end
        int maxSpecificity = 0;
        SourcesSet sources = pSeq1->sources();
        for (SourcesSet::iterator iterator = sources.begin(), iteratorEnd = sources.end(); iterator != iteratorEnd; ++iterator) {
         	const Complex_Selector* const pCurrentSelector = *iterator;
          maxSpecificity = max(maxSpecificity, pCurrentSelector->specificity());
        }

        bool isMoreSpecificOuter = false;
        for (ComplexSelectorDequeDeque::iterator iterator = result.begin(), iteratorEnd = result.end(); iterator != iteratorEnd; ++iterator) {
          ComplexSelectorDeque& seqs2 = *iterator;
          
          // TODO: not sure if this equality operator is doing a correct search of the deque's.
          if (seqs1 == seqs2) {
            continue;
          }
          
          bool isMoreSpecificInner = false;
          
          for (ComplexSelectorDeque::iterator iterator = seqs2.begin(), iteratorEnd = seqs2.end(); iterator != iteratorEnd; ++iterator) {
            Complex_Selector* pSeq2 = *iterator;
            
            // isMoreSpecificInner = _specificity(seq2) >= max_spec && _superselector?(seq2, seq1)
            isMoreSpecificInner = pSeq2->specificity() >= maxSpecificity && pSeq2->is_superselector_of(pSeq1);

            if (isMoreSpecificInner) {
              break;
            }
          }
          
          
					if (isMoreSpecificInner) {
            isMoreSpecificOuter = true;
            break;
          }
        }
        
        if (!isMoreSpecificOuter) {
          tempResult.push_back(pSeq1);
        }
      }
      
      // result[i] = tempResult
      *iterator = tempResult;
    }
    
    
    trimmed = result;
  }
  
  
  
/*
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
	void subweave(Complex_Selector* pOne, Complex_Selector* pTwo, ComplexSelectorDeque& out, Context& ctx) {
		cerr << "SUBWEAVE: ";
    printComplexSelector(pOne);
    printComplexSelector(pTwo, " ", true);

    if (pOne == NULL) {
    	out.push_back(pTwo ? pTwo->clone(ctx) : NULL);
      return;
    }
		if (pTwo == NULL) {
    	out.push_back(pOne ? pOne->clone(ctx) : NULL);
      return;
    }
    
    
    
    // Do the naive implementation
    Complex_Selector* pFirstPermutation = pOne->clone(ctx);
    pFirstPermutation->set_innermost(pTwo->clone(ctx), pFirstPermutation->innermost()->combinator()); // TODO: is this the correct combinator?
    out.push_back(pFirstPermutation);

    Complex_Selector* pSecondPermutation = pTwo->clone(ctx);
    pSecondPermutation->set_innermost(pOne->clone(ctx), pSecondPermutation->innermost()->combinator()); // TODO: is this the correct combinator?
    out.push_back(pSecondPermutation);
    

    //cerr << "subweave NOT YET IMPLEMENTED" << endl;
    //throw "subweave NOT YET IMPLEMENTED";
  }
  
  
  
  
  
  
void weave(ComplexSelectorDeque& toWeave, Context& ctx, ComplexSelectorDeque& weaved /*out*/) {
/*
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

//		printSelectors("WEAVE: ", toWeave, ctx, true);

  	// befores = [[]]
    ComplexSelectorDeque befores;
  	befores.push_back(NULL);

  	// afters = path.dup
    ComplexSelectorDeque afters;
    cloneComplexSelectorDeque(toWeave, afters, ctx);
  

  	//until afters.empty?
    while (afters.size() > 0) {
      //current = afters.shift.dup
			Complex_Selector* pCurrent = afters[0]->clone(ctx);
      afters.pop_front();
      
//      printComplexSelector(pCurrent, "CURRENT: ", true);

      
      //last_current = [current.pop]
      Complex_Selector* pLastCurrent = pCurrent->innermost();
      if (pCurrent == pLastCurrent) {
        pCurrent = NULL;
      } else {
        // TODO: consider adding popComplexSelector and shiftComplexSelector to make translating Ruby code easier.
        Complex_Selector* pIter = pCurrent;
        while (pIter) {
          if (pIter->tail() && !pIter->tail()->tail()) {
            pIter->tail(NULL);
            break;
          }
          
        	pIter = pIter->tail();
        }
      }
      
//      printComplexSelector(pCurrent, "CURRENT POST POP: ", true);
//      printComplexSelector(pLastCurrent, "LAST CURRENT: ", true);

    
    
      /*
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
      */

          
          
      ComplexSelectorDeque collector; // TODO: figure out what to name this
      
      // for before in befores do
      for (ComplexSelectorDeque::iterator iterator = befores.begin(), endIterator = befores.end();
           iterator != endIterator; ++iterator) {
        
        Complex_Selector* pBefore = *iterator;
        
        //        cerr << "> BEFORE IN BEFORES LOOP: BEFORE=";
        //        printComplexSelector(pBefore);
        //        printComplexSelector(pCurrent, " CURRENT=", true);
        
        ComplexSelectorDeque sub;
        subweave(pBefore, pCurrent, sub, ctx);
        
//        printSelectors("SUB: ", sub, ctx, true /*newline*/);
        
        if (sub.empty()) {
//          cerr << "CONTINUING DUE TO EMPTY" << endl;
          continue;
        }
      	
        // for seqs in sub do
        for (ComplexSelectorDeque::iterator iterator = sub.begin(), endIterator = sub.end();
             iterator != endIterator; ++iterator) {
          
          Complex_Selector* pSequences = *iterator; // TODO: clone this?
          
//          printComplexSelector(pSequences, "SEQS: ", true);
          
          if (pSequences) {
          	pSequences->set_innermost(pLastCurrent->clone(ctx), pSequences->innermost()->combinator()); // TODO: is this the correct combinator?
         	} else {
						pSequences = pLastCurrent->clone(ctx);
          }
          
//          printComplexSelector(pSequences, "PUSHING: ", true);
          
          collector.push_back(pSequences);
          
//          printSelectors("COLLECTOR: ", collector, ctx, true);
        }
      }
      
    	befores = collector;
    
    
    
    }
  

  	weaved = befores;
  }
  
  
  
  
  
  
  void paths(ComplexSelectorDequeDeque& source, ComplexSelectorDequeDeque& out, Context& ctx) {
    /*
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

    
    To_String to_string;
    
    ComplexSelectorDequeDeque loopStart;
    
//    cerr << "<<<<<<<<<<<<<<<<" << endl;
//    printSelectors("ARRS: ", source, ctx);

    // for arr in arrs do
    for (ComplexSelectorDequeDeque::iterator arrsIterator = source.begin(), endIterator = source.end();
         arrsIterator != endIterator; ++arrsIterator) {
      
      ComplexSelectorDeque& arr = *arrsIterator;
      
//      printComplexSelectorDequeDeque("LOOP START: ", loopStart, ctx, true /*newline*/);
      
    	ComplexSelectorDequeDeque permutations;

      // for e in arr do
      for (ComplexSelectorDeque::iterator arrIterator = arr.begin(), endIterator = arr.end();
           arrIterator != endIterator; ++arrIterator) {
      	Complex_Selector* pE = (*arrIterator)->clone(ctx);
        
//        printComplexSelector(pE, "E: ", true /*newline*/);
        
        // for path in loopStart do
        if (loopStart.size() == 0) {
//          cerr << "PATH: []" << endl;
          
          ComplexSelectorDeque newPermutation;
          newPermutation.push_back(pE);
          
//          printSelectors("CREATING: ", newPermutation, ctx, true /*newline*/);
          
          permutations.push_back(newPermutation);
        } else {
          for (ComplexSelectorDequeDeque::iterator loopStartIterator = loopStart.begin(), endIterator = loopStart.end();
               loopStartIterator != endIterator; ++loopStartIterator) {
            ComplexSelectorDeque& path = *loopStartIterator;
            
//            printSelectors("PATH: ", path, ctx, true /*newline*/);
            
            ComplexSelectorDeque newPermutation;
            cloneComplexSelectorDeque(path, newPermutation, ctx);
            newPermutation.push_back(pE);
            
//            printSelectors("CREATING: ", newPermutation, ctx, true /*newline*/);
            
            permutations.push_back(newPermutation);
            
          }
        }
        
      }
      
      /*
      Complex_Selector* pLoopingCurrentSelector = pOuterCurrentSelector;
      while(pLoopingCurrentSelector)
      {
        Complex_Selector* pCurrentNode = pLoopingCurrentSelector->clone(ctx);
        pCurrentNode->tail(NULL);

//        cerr << "E: " << pCurrentNode->perform(&to_string) << endl;

        // for path in loopStart do
        if (loopStart.size() == 0) {
//          cerr << "PATH: " << endl;
          // TODO: make this cleaner; try to figure out how to have one loop instead of this conditional
          permutations.push_back(pCurrentNode);
        } else {
          for (ComplexSelectorDeque::iterator iterator = loopStart.begin(), endIterator = loopStart.end();
               iterator != endIterator; ++iterator) {

            Complex_Selector* pNewPermutation = (*iterator)->clone(ctx);
            
//            cerr << "PATH: " << pNewPermutation->perform(&to_string) << endl;
            
            // TODO: is this the correct combinator? I don't want to replace what's there currently.
            pNewPermutation->set_innermost(pCurrentNode->clone(ctx), pNewPermutation->combinator());
            permutations.push_back(pNewPermutation);
          }
        }
        
        pLoopingCurrentSelector = pLoopingCurrentSelector->tail();
      }
      */
      
      loopStart = permutations;
    }
    
    out = loopStart;
    
//    cerr << "<<<<<<<<<<<<<<<<" << endl;
  }

  
  
  
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
  
  
  
  
  // TODO: make these member methods to avoid passing around ctx, subsetMap, and to avoid this forward declaration
  void extendComplexSelector(
    Complex_Selector* pComplexSelector,
    Context& ctx,
    ExtensionSubsetMap& subsetMap,
    set<Compound_Selector> seen,
    ComplexSelectorDeque& extendedSelectors);
  
  
  
  
  void extendCompoundSelector(
  	Compound_Selector* pSelector,
    Context& ctx,
    ExtensionSubsetMap& subsetMap,
    set<Compound_Selector> seen,
    ComplexSelectorDeque& extendedSelectors) {
  /*
   Selector_List* Extend::extend_compound(Compound_Selector* sel, set<Compound_Selector>& seen) {

   To_String to_string;
   // cerr << "EXTEND_COMPOUND: " << sel->perform(&to_string) << endl;
   Selector_List* results = new (ctx.mem) Selector_List(sel->path(), sel->position());
   
   // TODO: Do we need to group the results by extender?
   vector<pair<Complex_Selector*, Compound_Selector*> > entries = subset_map.get_v(sel->to_str_vec());
   
   for (size_t i = 0, S = entries.size(); i < S; ++i)
   {
   if (seen.count(*entries[i].second)) continue;
   // cerr << "COMPOUND: " << sel->perform(&to_string) << " KEYS TO " << entries[i].first->perform(&to_string) << " AND " << entries[i].second->perform(&to_string) << endl;
   Compound_Selector* diff = sel->minus(entries[i].second, ctx);
   Compound_Selector* last = entries[i].first->base();
   if (!last) last = new (ctx.mem) Compound_Selector(sel->path(), sel->position());
   // cerr << sel->perform(&to_string) << " - " << entries[i].second->perform(&to_string) << " = " << diff->perform(&to_string) << endl;
   // cerr << "LAST: " << last->perform(&to_string) << endl;
   Compound_Selector* unif;
   if (last->length() == 0) unif = diff;
   else if (diff->length() == 0) unif = last;
   else unif = last->unify_with(diff, ctx);
   // if (unif) cerr << "UNIFIED: " << unif->perform(&to_string) << endl;
   if (!unif || unif->length() == 0) continue;
   Complex_Selector* cplx = entries[i].first->clone(ctx);
   // cerr << "cplx: " << cplx->perform(&to_string) << endl;
   Complex_Selector* new_innermost = new (ctx.mem) Complex_Selector(sel->path(), sel->position(), Complex_Selector::ANCESTOR_OF, unif, 0);
   // cerr << "new_innermost: " << new_innermost->perform(&to_string) << endl;
   cplx->set_innermost(new_innermost, cplx->clear_innermost());
   // cerr << "new cplx: " << cplx->perform(&to_string) << endl;
   *results << cplx;
   set<Compound_Selector> seen2 = seen;
   seen2.insert(*entries[i].second);
   Selector_List* ex2 = extend_complex(cplx, seen2);
   *results += ex2;
   // cerr << "RECURSIVELY CALLING EXTEND_COMPLEX ON " << cplx->perform(&to_string) << endl;
   // vector<Selector_List*> ex2 = extend_complex(cplx, seen2);
   // for (size_t j = 0, T = ex2.size(); j < T; ++j)
   // {
   //   *results += ex2[i];
   // }
   }
   
   // cerr << "RESULTS: " << results->perform(&to_string) << endl;
   return results;
   */
  /*
      def do_extend(extends, parent_directives, seen = Set.new)
        print "\n%%%%%%%%%%%%% SIMPLE SEQ DO EXTEND #{members}\n"

        Sass::Util.group_by_to_a(extends.get(members.to_set)) {|ex, _| ex.extender}.map do |seq, group|
          sels = group.map {|_, s| s}.flatten
          # If A {@extend B} and C {...},
          # seq is A, sels is B, and self is C

          self_without_sel = Sass::Util.array_minus(self.members, sels)
          group.each {|e, _| e.result = :failed_to_unify unless e.result == :succeeded}
          next unless unified = seq.members.last.unify(self_without_sel, subject?)
          group.each {|e, _| e.result = :succeeded}
          next if group.map {|e, _| check_directives_match!(e, parent_directives)}.none?
          new_seq = Sequence.new(seq.members[0...-1] + [unified])
          new_seq.add_sources!(sources + [seq])
          [sels, new_seq]
        end.compact.map do |sels, seq|
          if seen.include?(sels)
            []
          else
            # print "SIMPLESEQ CALLING DO EXTEND: #{seq}\n"
            seq.do_extend(extends, parent_directives, seen + [sels])
          end
        end.flatten.uniq
      end
  */
    /*
     ISSUES:
     - Previous TODO: Do we need to group the results by extender?
     - What does subject do in?: next unless unified = seq.members.last.unify(self_without_sel, subject?)
     - If sibling extend fails, check if creation of new selecor is clone correct selector; might be extra parent nodes
     - The search for uniqueness at the end is not ideal since it's has to loop over everything...
     - Check if the final search for uniqueness is doing anything that extendComplexSelector isn't already doing...
     */
    To_String to_string;
//    printCompoundSelector(pSelector, "%%%%%%%%%%%%% SIMPLE SEQ DO EXTEND: ", true);


    typedef pair<Complex_Selector*, Compound_Selector*> ExtensionPair;
    typedef vector<ExtensionPair> SubsetMapEntries;
    SubsetMapEntries entries = subsetMap.get_v(pSelector->to_str_vec());
    
		for (SubsetMapEntries::iterator iterator = entries.begin(), endIterator = entries.end(); iterator != endIterator; ++iterator) {
      Complex_Selector* pExtComplexSelector = iterator->first;    // The selector up to where the @extend is (ie, the thing to merge)
      Compound_Selector* pExtCompoundSelector = iterator->second; // The stuff after the @extend
      
      if (seen.find(*pExtCompoundSelector) != seen.end()) {
//      	printCompoundSelector(pExtCompoundSelector, "CONTINUING DUE TO SEEN: ", true);
        continue;
      }
      
      // TODO: This can return a Compound_Selector with no elements. Should that just be returning NULL?
      Compound_Selector* pSelectorWithoutExtendSelectors = pSelector->minus(pExtCompoundSelector, ctx);
//      cerr << "MINUS: " << pSelectorWithoutExtendSelectors->perform(&to_string) << endl;




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
      


      // This seems a little fishy to me. See if it causes any problems. From the ruby, we should be able to just
      // get rid of the last Compound_Selector and replace it with this one. I think the reason this code is more
      // complex is that Complex_Selector contains a combinator, but in ruby combinators have already been filtered
      // out and aren't operated on.
      Complex_Selector* pNewSelector = pExtComplexSelector->clone(ctx);
      Complex_Selector* pNewInnerMost = new (ctx.mem) Complex_Selector(pSelector->path(), pSelector->position(), Complex_Selector::ANCESTOR_OF, pUnifiedSelector, NULL);
      Complex_Selector::Combinator combinator = pNewSelector->clear_innermost();
      pNewSelector->set_innermost(pNewInnerMost, combinator);

      
      
      


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
    
//    printSelectors("SIMP SEQ RETURN:: ", extendedSelectors, ctx, true /*newline*/);
  }
 
  
  void extendComplexSelector(
  	Complex_Selector* pComplexSelector,
    Context& ctx,
    ExtensionSubsetMap& subsetMap,
    set<Compound_Selector> seen,
    ComplexSelectorDeque& extendedSelectors) {
  /*
      def do_extend(extends, parent_directives, seen = Set.new)

        extended_not_expanded = members.map do |sseq_or_op|

          next [[sseq_or_op]] unless sseq_or_op.is_a?(SimpleSequence)

          # print ">>>>> CALLING EXTEND ON: #{sseq_or_op}\n"
          extended = sseq_or_op.do_extend(extends, parent_directives, seen)

          choices = extended.map {|seq| seq.members}

          choices.unshift([sseq_or_op]) unless extended.any? {|seq| seq.superselector?(sseq_or_op)}

          choices
        end

        weaves = Sass::Util.paths(extended_not_expanded).map {|path| weave(path)}

        result = Sass::Util.flatten(trim(weaves), 1).map {|p| Sequence.new(p)}

        result
      end
  */
    /*
     ISSUES:
     - check for operator doesn't transfer over to libsass' object model
     */
    To_String to_string;
//    cerr << "************ SEQ DO EXTEND: " << pComplexSelector->perform(&to_string) << endl;

    Complex_Selector* pCurrentComplexSelector = pComplexSelector->tail();
    
    ComplexSelectorDequeDeque extendedNotExpanded;
    
    while(pCurrentComplexSelector)
    {
      Compound_Selector* pCompoundSelector = pCurrentComplexSelector->head();

      ComplexSelectorDeque extended;
      extendCompoundSelector(pCompoundSelector, ctx, subsetMap, seen, extended /*out*/);
      
      printSelectors("EXTENDED: ", extended, ctx, true /*newline*/);
      
      

      /*
      Complex_Selector* pChoices = NULL;
      
      if (extended.size() > 0) {
        pChoices = extended[0];

        for (ComplexSelectorDeque::iterator iterator = extended.begin() + 1, endIterator = extended.end();
             iterator != endIterator; ++iterator) {
          // TODO: We could keep track of innermost so we don't traverse the whole thing each time
          // TODO: Are we getting the combinator from the right place
          Complex_Selector* pCurrentSelector = *iterator;
          Complex_Selector* pCurrentInnermost = pChoices->innermost();
          pCurrentInnermost->set_innermost(pCurrentSelector, pCurrentInnermost->combinator());
        }
      }
      */
      
//      printComplexSelector(pChoices, ">>>>> CHOICES BEFORE: ", true);
  
      
      
      
      // TODO: this is just here for debugging
      Complex_Selector* pTemp = pCurrentComplexSelector->clone(ctx);
      pTemp->tail(NULL);
//      printComplexSelector(pTemp, ">>>>> INSERT: ", true);

      
      
      Complex_Selector* pJustCurrentCompoundSelector = pCurrentComplexSelector->clone(ctx);
      pJustCurrentCompoundSelector->tail(NULL);
      
      
      
      // Until the rest is implemented, just include the extended selectors
      // Prepend the Compound_Selector based on the choices logic; choices seems to be extend but with an Array instead of a Sequence

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
        // Get just the first piece

        
        // Add in pJustCurrentCompoundSelector to the front of pChoices
//        pJustCurrentCompoundSelector->tail(pChoices);
//        pChoices = pJustCurrentCompoundSelector;
        
        // TODO: this could be done in a simpler way
        
        extended.push_front(pJustCurrentCompoundSelector);
      }
      

      printSelectors("CHOICES UNSHIFTED: ", extended, ctx, true /*newline*/);


      
//      if (pChoices) {
//      	extended.push_back(pChoices);
//      }
      
      extendedNotExpanded.push_back(extended);
      
    	pCurrentComplexSelector = pCurrentComplexSelector->tail();
    }

    printComplexSelectorDequeDeque("EXTENDED NOT EXPANDED: ", extendedNotExpanded, ctx, true /*newline*/);
    
  
    
    ComplexSelectorDequeDeque permutations;
    paths(extendedNotExpanded, permutations, ctx);
    
    
    printComplexSelectorDequeDeque("PATHS: ", permutations, ctx, true /*newline*/);

  
    
    
//    printSelectors("PERMS: ", permutations, ctx);
//    cerr << endl;
//    printComplexSelectorDequeDeque("PREWEAVE: ", permutations, ctx, true /-*newline*-/);
 

 
 		ComplexSelectorDequeDeque weaves;
    for (ComplexSelectorDequeDeque::iterator iterator = permutations.begin(), endIterator = permutations.end();
         iterator != endIterator; ++iterator) {
      ComplexSelectorDeque& toWeave = *iterator;
      
      ComplexSelectorDeque weaved;
			weave(toWeave, ctx, weaved);
      
      weaves.push_back(weaved);
    }
    
    printComplexSelectorDequeDeque("WEAVES: ", weaves, ctx, true /*newline*/);
    
    
    
    
    ComplexSelectorDequeDeque trimmed;
    trim(weaves, trimmed, ctx);

    
		printComplexSelectorDequeDeque("TRIMMED: ", trimmed, ctx, true /*newline*/);

    
    for (ComplexSelectorDequeDeque::iterator iterator = trimmed.begin(), endIterator = trimmed.end();
         iterator != endIterator; ++iterator) {
      
      ComplexSelectorDeque& toCombine = *iterator;
      extendedSelectors.insert(extendedSelectors.end(), toCombine.begin(), toCombine.end());
      
    }
    
    printSelectors("FLATTENED: ", extendedSelectors, ctx, true);
    printSelectors(">>>>> EXTENDED: ", extendedSelectors, ctx, true);


    // TODO: implement the rest of this...
    /*
     result = Sass::Util.flatten(trim(weaves), 1).map {|p| Sequence.new(p)}
     
     result
     end
     */
  }

  
  Selector_List* extendSelectorList(Selector_List* pSelectorList, Context& ctx, ExtensionSubsetMap& subsetMap) {
  /*
  def do_extend(extends, parent_directives)

    result = CommaSequence.new(members.map do |seq|

        extended = seq.do_extend(extends, parent_directives)

        # First Law of Extend: the result of extending a selector should
        # always contain the base selector.
        #
        # See https://github.com/nex3/sass/issues/324.

        # unshift = put seq on front of array unless it has a placeholder or is already in extended
        # CAN YOU SAFELY GET RID OF PLACEHOLDERS HERE? - maybe because it will already be in the list?
        extended.unshift seq unless seq.has_placeholder? || extended.include?(seq)
        extended

        extended
      end.flatten)

      result
  end
  */
    /*
     ISSUES:
     - Improvement: searching through deque with std::find is probably slow
     - Improvement: can we just use one deque?
     */
    To_String to_string;

//    cerr << "COMMA SEQ DO EXTEND " << pSelectorList->perform(&to_string) << endl;

    ComplexSelectorDeque newSelectors;

    for (size_t index = 0, length = pSelectorList->length(); index < length; index++) {
      Complex_Selector* pSelector = (*pSelectorList)[index];

      ComplexSelectorDeque extendedSelectors;
      set<Compound_Selector> seen;
      extendComplexSelector(pSelector, ctx, subsetMap, seen, extendedSelectors /*out*/);
      
			printSelectors("<<<<< EXTENDED: ", extendedSelectors, ctx, true /*newline*/);
      
      if (!pSelector->has_placeholder()) {
        bool selectorAlreadyExists = complexSelectorDequeContains(extendedSelectors, pSelector);
        if (!selectorAlreadyExists) {
        	extendedSelectors.push_front(pSelector);
        }
      }
      
      printSelectors("<<<<< EXTENDED UNSHIFTED: ", extendedSelectors, ctx, true /*newline*/);
  
      newSelectors.insert(newSelectors.end(), extendedSelectors.begin(), extendedSelectors.end());
      
//      printSelectors("FINAL SELECTORS: ", newSelectors, ctx, true /*newline*/);
    }
    
    return createSelectorListFromDeque(newSelectors, ctx, pSelectorList);

  }

  
  void extendRuleset(Ruleset* pRuleset, Context& ctx, ExtensionSubsetMap& subsetMap) {
    To_String to_string;

    Selector_List* pNewSelectorList = extendSelectorList(static_cast<Selector_List*>(pRuleset->selector()), ctx, subsetMap);

    if (pNewSelectorList) {
      // re-parse in order to restructure expanded placeholder nodes correctly
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
  
  
  
  
  
  
  // Return the extension Compound_Selector* or NULL if no extension was found
  // For instance, @extend .bar would return a Compound_Selector* to .bar
  Compound_Selector* getComplexSelectorExtension(Complex_Selector* pComplexSelector, Extensions& extensions) {
    // Iterate the complex selector checking if any of the pieces are in the extensions collection

    Compound_Selector* pHead = pComplexSelector->head();
    Complex_Selector* pTail = pComplexSelector->tail();

    while(pTail)
    {
      pHead = pTail->head();
      pTail = pTail->tail();
      if (pHead &&
          !pHead->is_empty_reference() &&
          extensions.count(*pHead) > 0)
      {
        return pHead;
      }
    }
    
    return NULL;
  }
  
  Complex_Selector* generateExtension(Complex_Selector* pExtender, Complex_Selector* pExtendee, Compound_Selector* pExtensionCompoundSelector, Context& ctx) {
    // Clone the selector so we can modify it in place and use it as our new selector
    Complex_Selector* pNewSelector = pExtendee->clone(ctx);
    
    // Find the node in the linked list before the one we want to replace
    Complex_Selector* pIterSelector = pNewSelector;
    Complex_Selector* pSelectorBeforeExtendPoint = NULL;
    while(pIterSelector)
    {
      if (pIterSelector->tail() && pIterSelector->tail()->head() == pExtensionCompoundSelector) {
        pSelectorBeforeExtendPoint = pIterSelector;
        break;
      }
      pIterSelector = pIterSelector->tail();
    }
    
    // Insert our extender's selector into the current selector
    // TODO: look into whether this library is memory leaking all over the place. Where does delete get called?
    Complex_Selector* pSelectorAfterExtendPoint = pSelectorBeforeExtendPoint->tail()->tail();
    pExtender->innermost()->tail(pSelectorAfterExtendPoint);
    pSelectorBeforeExtendPoint->tail(pExtender);
    
    return pNewSelector;
  }
  
  void extendRulesetOld(Ruleset* pRuleset, Context& ctx, Extensions& extensions) {
    To_String to_string;

    // Get the start selector list
    Selector_List* pOriginalSelectorGroup = static_cast<Selector_List*>(pRuleset->selector());
    cerr << "CURR GROUP: " << pOriginalSelectorGroup->perform(&to_string) << endl;
    
    // Create a collection of selectors we've already processed
    ComplexSelectorDeque newGroup;
    
    // Create a collection of selectors we still need to process. As extensions are created, they'll
    // get added to this list so that chained extends are processed in the correct order.
    ComplexSelectorDeque selectorsToProcess;
    fillDequeFromSelectorList(selectorsToProcess, pOriginalSelectorGroup);
    
    bool extendedSomething = false;
    
    while (selectorsToProcess.size() > 0) {
      
      // Get the current selector to try and extend
      Complex_Selector* pCurrentSelector = selectorsToProcess[0];
      cerr << "CUR SEL: " << pCurrentSelector->perform(&to_string) << endl;
      
      // Remove it from the process queue since we'll be done after we finish this loop
      selectorsToProcess.pop_front();
      
      // Always add the current selector
      newGroup.push_back(pCurrentSelector);
      
      // Check if the current selector has an extension
      Compound_Selector* pExtensionCompoundSelector = getComplexSelectorExtension(pCurrentSelector, extensions);
      
      if (pExtensionCompoundSelector != NULL) {

        cerr << "HAS EXT: " << pCurrentSelector->perform(&to_string) << endl;
        cerr << "EXT FOUND: " << pExtensionCompoundSelector->perform(&to_string) << endl;

        // We are going to extend something!
        extendedSomething = true;
        
        // Iterate over the extensions in reverse to make adding them to our selectorsToProcess list in the correct order easier.
        for (Extensions::iterator iterator = extensions.upper_bound(*pExtensionCompoundSelector),
             endIterator = extensions.lower_bound(*pExtensionCompoundSelector);
             iterator != endIterator;) {
          
          --iterator;
          
          Compound_Selector extCompoundSelector = iterator->first;
          Complex_Selector* pExtComplexSelector = iterator->second->clone(ctx); // Clone this so we get a new copy to insert into
          
          cerr << "COMPOUND: " << extCompoundSelector.perform(&to_string) << endl;
          cerr << "COMPLEX: " << pExtComplexSelector->perform(&to_string) << endl;
          
          // We can't extend ourself
          // TODO: figure out a better way to do this
          if (pCurrentSelector->perform(&to_string) == pExtComplexSelector->perform(&to_string)) {
            continue;
          }
          
          // Generate our new extension selector
          Complex_Selector* pNewSelector = generateExtension(pExtComplexSelector, pCurrentSelector, pExtensionCompoundSelector, ctx);
          
          // Add the new selector to our list of selectors to process
          selectorsToProcess.push_front(pNewSelector);
        }
      }
      
      // TODO: remove these once we're done debugging this method
      Selector_List* pTempDone = createSelectorListFromDeque(newGroup, ctx, pOriginalSelectorGroup);
      Selector_List* pTempToProcess = createSelectorListFromDeque(selectorsToProcess, ctx, pOriginalSelectorGroup);
      cerr << "DONE PROCESSING GROUP AFTER EXT ITER: " << pTempDone->perform(&to_string) << endl;
      cerr << "TO PROCESS GROUP AFTER EXT ITER: " << pTempToProcess->perform(&to_string) << endl;
    }
    
    
    // Only update the rule set's selector (incurring the cost of a parse) if we actually
    // added something new.
    
    if (extendedSomething) {
      Selector_List* pNewSelectorGroup = createSelectorListFromDeque(newGroup, ctx, pOriginalSelectorGroup);
      cerr << "NEW GROUP: " << pNewSelectorGroup->perform(&to_string) << endl;
      
      // re-parse in order to restructure expanded placeholder nodes correctly
      pRuleset->selector(
        Parser::from_c_str(
          (pNewSelectorGroup->perform(&to_string) + ";").c_str(),
          ctx,
          pNewSelectorGroup->path(),
          pNewSelectorGroup->position()
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

    //extendRulesetOld(pRuleset, ctx, extensions);
    extendRuleset(pRuleset, ctx, subset_map);
    
    // Iterate into child blocks
    
    Block* b = pRuleset->block();

    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = (*b)[i];
      stm->perform(this);
    }
    

    return;


    // Define these so the old code continues to compile. I want to keep the old code around for reference, but
    // it should be deleted before we go live with these changes.
    Ruleset* r = pRuleset;

    
    // To_String to_string;
    // ng = new (ctx.mem) Selector_List(sg->path(), sg->position(), sg->length());
    // // for each selector in the group
    // for (size_t i = 0, L = sg->length(); i < L; ++i) {
    //   Complex_Selector* sel = (*sg)[i];
    //   *ng << sel;
    //   // if it's supposed to be extended
    //   Compound_Selector* sel_base = sel->base();
    //   if (sel_base && extensions.count(*sel_base)) {
    //     // extend it wrt each of its extenders
    //     for (multimap<Compound_Selector, Complex_Selector*>::iterator extender = extensions.lower_bound(*sel_base), E = extensions.upper_bound(*sel_base);
    //          extender != E;
    //          ++extender) {
    //       *ng += generate_extension(sel, extender->second);
    //       extended = true;
    //     }
    //   }
    // }
    // if (extended) r->selector(ng);
    To_String to_string;
    Selector_List* sg = static_cast<Selector_List*>(r->selector());
    Selector_List* all_subbed = new (ctx.mem) Selector_List(r->selector()->path(), r->selector()->position());
    for (size_t i = 0, L = sg->length(); i < L; ++i) {
      Complex_Selector* cplx = (*sg)[i];
      bool extended = true;
      Selector_List* ng = 0;
      while (cplx) {
        Selector_Placeholder* sp = cplx->find_placeholder();
        if (!sp) {
            // After the loop over this rule set's selectors completes, the selectors in
            // all_subbed will become the new selectors for this rule set. If we get here, there's no
            // placeholder selector in the current complex selector. We don't want to lose
            // this selector, so append it to ng, so that it will get added to all_subbed.
            ng = new (ctx.mem) Selector_List(sg->path(), sg->position());
            *ng << cplx;
            break;
        }
        Compound_Selector* placeholder = new (ctx.mem) Compound_Selector(cplx->path(), cplx->position(), 1);
        *placeholder << sp;
        // if the current placeholder can be subbed
        if (extensions.count(*placeholder)) {
          ng = new (ctx.mem) Selector_List(sg->path(), sg->position());
          // perform each substitution and accumulate
          for (multimap<Compound_Selector, Complex_Selector*>::iterator extender = extensions.lower_bound(*placeholder), E = extensions.upper_bound(*placeholder);
               extender != E;
               ++extender) {
            Contextualize do_sub(ctx, 0, 0, backtrace, placeholder, extender->second);
            Complex_Selector* subbed = static_cast<Complex_Selector*>(cplx->perform(&do_sub));
            *ng << subbed;
            // cplx = subbed;
          }
        }
        else extended = false;
        // if at any point we fail to sub a placeholder, then break and skip this entire complex selector
        // else {
          cplx = 0;
        // }
      }
      // if we make it through the loop and `extended` is still true, then
      // we've subbed all placeholders in the current complex selector -- add
      // it to the result
      if (extended && ng) *all_subbed += ng;
    }


    if (all_subbed->length()) {
      // re-parse in order to restructure expanded placeholder nodes correctly
      r->selector(
        Parser::from_c_str(
          (all_subbed->perform(&to_string) + ";").c_str(),
          ctx,
          all_subbed->path(),
          all_subbed->position()
        ).parse_selector_group()
      );
    }

    // let's try the new stuff here; eventually it should replace the preceding
    set<Compound_Selector> seen;
    // Selector_List* new_list = new (ctx.mem) Selector_List(sg->path(), sg->position());
    bool extended = false;
    sg = static_cast<Selector_List*>(r->selector());
    Selector_List* ng = new (ctx.mem) Selector_List(sg->path(), sg->position(), sg->length());
    // for each complex selector in the list
    for (size_t i = 0, L = sg->length(); i < L; ++i)
    {
      // get rid of the useless backref that's at the front of the selector
      (*sg)[i] = (*sg)[i]->tail();
      if (!(*sg)[i]->has_placeholder()) *ng << (*sg)[i];
      // /* *new_list += */ extend_complex((*sg)[i], seen);
      // cerr << "checking [ " << (*sg)[i]->perform(&to_string) << " ]" << endl;
      Selector_List* extended_sels = extend_complex((*sg)[i], seen);
      // cerr << "extended by [ " << extended_sels->perform(&to_string) << " ]" << endl;
      if (extended_sels->length() > 0)
      {
        // cerr << "EXTENDED SELS: " << extended_sels->perform(&to_string) << endl;
        extended = true;
        for (size_t j = 0, M = extended_sels->length(); j < M; ++j)
        {
          // cerr << "GENERATING EXTENSION FOR " << (*sg)[i]->perform(&to_string) << " AND " << (*extended_sels)[j]->perform(&to_string) << endl;
          // cerr << "length of extender [ " << (*extended_sels)[j]->perform(&to_string) << " ] is " << (*extended_sels)[j]->length() << endl;
          // cerr << "extender's tail is [ " << (*extended_sels)[j]->tail()->perform(&to_string) << " ]" << endl;
          Selector_List* fully_extended = generate_extension((*sg)[i], (*extended_sels)[j]->tail()); // TODO: figure out why the extenders each have an extra node at the beginning
          // cerr << "combining extensions into [ " << fully_extended->perform(&to_string) << " ]" << endl;
          *ng += fully_extended;
        }
      }
    }

    // if (extended) cerr << "FINAL SELECTOR: " << ng->perform(&to_string) << endl;
    if (extended) r->selector(ng);

    // If there are still placeholders after the preceding, filter them out.
    if (r->selector()->has_placeholder())
    {
      Selector_List* current = static_cast<Selector_List*>(r->selector());
      Selector_List* final = new (ctx.mem) Selector_List(sg->path(), sg->position());
      for (size_t i = 0, L = current->length(); i < L; ++i)
      {
        if (!(*current)[i]->has_placeholder()) *final << (*current)[i];
      }
      r->selector(final);
    }
    r->block()->perform(this);
  }

  void Extend::operator()(Media_Block* m)
  {
    m->block()->perform(this);
  }

  void Extend::operator()(At_Rule* a)
  {
    if (a->block()) a->block()->perform(this);
  }

  Selector_List* Extend::generate_extension(Complex_Selector* extendee, Complex_Selector* extender)
  {
    To_String to_string;
    Selector_List* new_group = new (ctx.mem) Selector_List(extendee->path(), extendee->position());
    if (extendee->perform(&to_string) == extender->perform(&to_string)) return new_group;
    Complex_Selector* extendee_context = extendee->context(ctx);
    Complex_Selector* extender_context = extender->context(ctx);
    if (extendee_context && extender_context) {
      // cerr << "extender and extendee have a context" << endl;
      // cerr << extender_context->length() << endl;
      Complex_Selector* base = new (ctx.mem) Complex_Selector(new_group->path(), new_group->position(), Complex_Selector::ANCESTOR_OF, extender->base(), 0);
      extendee_context->innermost()->tail(extender);
      *new_group << extendee_context;
      // make another one so we don't erroneously share tails
      extendee_context = extendee->context(ctx);
      extendee_context->innermost()->tail(base);
      extender_context->innermost()->tail(extendee_context);
      *new_group << extender_context;
    }
    else if (extendee_context) {
      // cerr << "extendee has a context" << endl;
      extendee_context->innermost()->tail(extender);
      *new_group << extendee_context;
    }
    else {
      // cerr << "extender has a context" << endl;
      *new_group << extender;
    }
    return new_group;
  }

  Selector_List* Extend::extend_complex(Complex_Selector* sel, set<Compound_Selector>& seen)
  {
    To_String to_string;
    // cerr << "EXTENDING COMPLEX: " << sel->perform(&to_string) << endl;
    // vector<Selector_List*> choices; // 
    Selector_List* extended = new (ctx.mem) Selector_List(sel->path(), sel->position());

    Compound_Selector* h = sel->head();
    Complex_Selector* t = sel->tail();
    if (h && !h->is_empty_reference())
    {
      // Selector_List* extended = extend_compound(h, seen);
      *extended += extend_compound(h, seen);
      // bool found = false;
      // for (size_t i = 0, L = extended->length(); i < L; ++i)
      // {
      //   if ((*extended)[i]->is_superselector_of(h))
      //   { found = true; break; }
      // }
      // if (!found)
      // {
      //   *extended << new (ctx.mem) Complex_Selector(sel->path(), sel->position(), Complex_Selector::ANCESTOR_OF, h, 0);
      // }
      // choices.push_back(extended);
    }
    while(t)
    {
      h = t->head();
      t = t->tail();
      if (h && !h->is_empty_reference())
      {
        // Selector_List* extended = extend_compound(h, seen);
        *extended += extend_compound(h, seen);
        // bool found = false;
        // for (size_t i = 0, L = extended->length(); i < L; ++i)
        // {
        //   if ((*extended)[i]->is_superselector_of(h))
        //   { found = true; break; }
        // }
        // if (!found)
        // {
        //   *extended << new (ctx.mem) Complex_Selector(sel->path(), sel->position(), Complex_Selector::ANCESTOR_OF, h, 0);
        // }
        // choices.push_back(extended);
      }
    }
    // cerr << "EXTENSIONS: " << extended->perform(&to_string) << endl;
    return extended;
    // cerr << "CHOICES:" << endl;
    // for (size_t i = 0, L = choices.size(); i < L; ++i)
    // {
    //   cerr << choices[i]->perform(&to_string) << endl;
    // }

    // vector<vector<Complex_Selector*> > cs;
    // for (size_t i = 0, S = choices.size(); i < S; ++i)
    // {
    //   cs.push_back(choices[i]->elements());
    // }
    // vector<vector<Complex_Selector*> > ps = paths(cs);
    // cerr << "PATHS:" << endl;
    // for (size_t i = 0, S = ps.size(); i < S; ++i)
    // {
    //   for (size_t j = 0, T = ps[i].size(); j < T; ++j)
    //   {
    //     cerr << ps[i][j]->perform(&to_string) << ", ";
    //   }
    //   cerr << endl;
    // }
    // vector<Selector_List*> new_choices;
    // for (size_t i = 0, S = ps.size(); i < S; ++i)
    // {
    //   Selector_List* new_list = new (ctx.mem) Selector_List(sel->path(), sel->position());
    //   for (size_t j = 0, T = ps[i].size(); j < T; ++j)
    //   {
    //     *new_list << ps[i][j];
    //   }
    //   new_choices.push_back(new_list);
    // }
    // return new_choices;
  }

  Selector_List* Extend::extend_compound(Compound_Selector* sel, set<Compound_Selector>& seen)
  {
    To_String to_string;
    // cerr << "EXTEND_COMPOUND: " << sel->perform(&to_string) << endl;
    Selector_List* results = new (ctx.mem) Selector_List(sel->path(), sel->position());

    // TODO: Do we need to group the results by extender?
    vector<pair<Complex_Selector*, Compound_Selector*> > entries = subset_map.get_v(sel->to_str_vec());

    for (size_t i = 0, S = entries.size(); i < S; ++i)
    {
      if (seen.count(*entries[i].second)) continue;
      // cerr << "COMPOUND: " << sel->perform(&to_string) << " KEYS TO " << entries[i].first->perform(&to_string) << " AND " << entries[i].second->perform(&to_string) << endl;
      Compound_Selector* diff = sel->minus(entries[i].second, ctx);
      Compound_Selector* last = entries[i].first->base();
      if (!last) last = new (ctx.mem) Compound_Selector(sel->path(), sel->position());
      // cerr << sel->perform(&to_string) << " - " << entries[i].second->perform(&to_string) << " = " << diff->perform(&to_string) << endl;
      // cerr << "LAST: " << last->perform(&to_string) << endl;
      Compound_Selector* unif;
      if (last->length() == 0) unif = diff;
      else if (diff->length() == 0) unif = last;
      else unif = last->unify_with(diff, ctx);
      // if (unif) cerr << "UNIFIED: " << unif->perform(&to_string) << endl;
      if (!unif || unif->length() == 0) continue;
      Complex_Selector* cplx = entries[i].first->clone(ctx);
      // cerr << "cplx: " << cplx->perform(&to_string) << endl;
      Complex_Selector* new_innermost = new (ctx.mem) Complex_Selector(sel->path(), sel->position(), Complex_Selector::ANCESTOR_OF, unif, 0);
      // cerr << "new_innermost: " << new_innermost->perform(&to_string) << endl;
      cplx->set_innermost(new_innermost, cplx->clear_innermost());
      // cerr << "new cplx: " << cplx->perform(&to_string) << endl;
      *results << cplx;
      set<Compound_Selector> seen2 = seen;
      seen2.insert(*entries[i].second);
      Selector_List* ex2 = extend_complex(cplx, seen2);
      *results += ex2;
      // cerr << "RECURSIVELY CALLING EXTEND_COMPLEX ON " << cplx->perform(&to_string) << endl;
      // vector<Selector_List*> ex2 = extend_complex(cplx, seen2);
      // for (size_t j = 0, T = ex2.size(); j < T; ++j)
      // {
      //   *results += ex2[i];
      // }
    }

    // cerr << "RESULTS: " << results->perform(&to_string) << endl;
    return results;
  }

}
