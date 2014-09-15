#include "extend.hpp"
#include "context.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"
#include "backtrace.hpp"
#include "paths.hpp"
#include "parser.hpp"
#ifndef SASS_AST
#include "node.hpp"
#endif
#include "sass_util.hpp"
#include "debug.hpp"
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


	typedef vector<vector<int> > LCSTable;


  // ComplexSelectorOrCombinator = CSOC
  // Ruby sass' ComplexSelector equivalent has an array of CompoundSelectors and Combinators, which can be acted on
  // separately. Our data model has one class that contains a combinator and ComplexSelector. This class can be one
  // or the other, which will allow us to translate from our data model into something that can be operated on more
  // like Ruby sass' extend functionality.
  /*
   TODO:
   - consider adding c() and s() ease-of-use functions for accessing combinator() and selector()
   */
  class ComplexSelectorOrCombinator {
  public:
    inline bool isCombinator() const { return mIsCombinator; }
    inline bool isSelector() const { return !mIsCombinator; }
    
    Complex_Selector::Combinator combinator() const { return mCombinator; }

    Complex_Selector* selector() { return mpSelector; }
    const Complex_Selector* const selector() const { return mpSelector; }
    
    // Pass these by-value since they're basically just two pointers.
    static ComplexSelectorOrCombinator createCombinator(Complex_Selector* pSelector, Context& ctx);
    static ComplexSelectorOrCombinator createSelector(Complex_Selector* pSelector, Context& ctx);
    
    bool operator==(const ComplexSelectorOrCombinator& rhs) const;
		inline bool operator!=(const ComplexSelectorOrCombinator& rhs) const {return !(*this == rhs);}
    
    Context* mpContext;
    
    ComplexSelectorOrCombinator(const ComplexSelectorOrCombinator& other);
    
    inline ~ComplexSelectorOrCombinator() {} // TODO: is this needed?

  private:
    // private constructor; use the static methods createCombinator and createSelector to instantiate (this is just more expressive than a single constructor).
    // If we think it's safe, we could also get rid of mIsCombinator in favor of just checking if mpSelector is NULL. This is less explicit though, so I'm keeping
    // it the way it is now. A bool is a small price to pay.
    ComplexSelectorOrCombinator(bool isCombinator, Complex_Selector::Combinator combinator, Complex_Selector* pSelector, Context* pContext);
    
    // This private default constructor is necessary for LCS to work. It will get overwritten. Just give
    // it some innocuous values for now. We don't want other people calling it, so make it private. Use
    // friends to only allow the lcs code to instantiate one of these.
    ComplexSelectorOrCombinator() : mpContext(NULL), mIsCombinator(true), mCombinator(Complex_Selector::ANCESTOR_OF), mpSelector(NULL) {}
    template<typename DequeContentType, typename ComparatorType>
	  friend void lcsBacktrace(const LCSTable& c, const deque<DequeContentType>& x, const deque<DequeContentType>& y, int i, int j, const ComparatorType& comparator, deque<DequeContentType>& out);
    template<typename DequeContentType, typename ComparatorType>
	  friend void lcsTable(const deque<DequeContentType>& x, const deque<DequeContentType>& y, const ComparatorType& comparator, LCSTable& out);
    template<typename DequeContentType, typename ComparatorType>
  	friend void lcs(const deque<DequeContentType>& x, const deque<DequeContentType>& y, const ComparatorType& comparator, deque<DequeContentType>& out);
    
    bool mIsCombinator;
    Complex_Selector::Combinator mCombinator;
    Complex_Selector* mpSelector;
  };
  
  ComplexSelectorOrCombinator::ComplexSelectorOrCombinator(bool isCombinator, Complex_Selector::Combinator combinator, Complex_Selector* pSelector, Context* pContext) :
  	mpContext(pContext),
  	mIsCombinator(isCombinator),
    mCombinator(combinator),
    mpSelector(pSelector){
    
  }
  
  ComplexSelectorOrCombinator::ComplexSelectorOrCombinator(const ComplexSelectorOrCombinator& other) :
  	mpContext(other.mpContext),
    mIsCombinator(other.mIsCombinator),
    mCombinator(other.mCombinator),
    mpSelector(other.mpSelector)
  {
  	if (mpContext) {
	    mpSelector = mpSelector->clone(*mpContext);
    }
  }
  
  ComplexSelectorOrCombinator ComplexSelectorOrCombinator::createCombinator(Complex_Selector* pSelector, Context& ctx) {
    return ComplexSelectorOrCombinator(true /*isCombinator*/, pSelector->combinator(), NULL /*pSelector*/, &ctx);
  }

  ComplexSelectorOrCombinator ComplexSelectorOrCombinator::createSelector(Complex_Selector* pSelector, Context& ctx) {
    Complex_Selector* pClone = pSelector->clone(ctx);
    pClone->combinator(Complex_Selector::ANCESTOR_OF); // clear out the existing combinator if there is one
    pClone->tail(NULL); // clear out the tail pointer if there is one
   	return ComplexSelectorOrCombinator(false /*isCombinator*/, Complex_Selector::ANCESTOR_OF, pClone /*pSelector*/, &ctx);
  }
  
  bool ComplexSelectorOrCombinator::operator==(const ComplexSelectorOrCombinator& rhs) const {
  	if ((this->isCombinator() && !rhs.isCombinator()) || (this->isSelector() && !rhs.isSelector())) {
    	return false;
    }
    
    if (this->isCombinator()) {
    	return this->combinator() == rhs.combinator();
    } else {
    	return (!(*this->selector() < *rhs.selector()) && !(*rhs.selector() < *this->selector()));
    }
  }
  
  
  
  
  
  typedef deque<Complex_Selector*> ComplexSelectorDeque;
	typedef deque<ComplexSelectorDeque> ComplexSelectorDequeDeque;
  
  typedef pair<Complex_Selector*, Compound_Selector*> ExtensionPair;
  typedef vector<ExtensionPair> SubsetMapEntries;
  
  typedef deque<ComplexSelectorOrCombinator> CSOCDeque;
  typedef deque<CSOCDeque> CSOCDequeDeque;
  typedef deque<CSOCDequeDeque> CSOCDequeDequeDeque;
  
  
  void complexSelectorToCSOC(Complex_Selector* pToConvert, CSOCDeque& out, Context& ctx) {
    out.clear();
    
    while (pToConvert) {
      if (pToConvert->combinator() != Complex_Selector::ANCESTOR_OF) {
        out.push_back(ComplexSelectorOrCombinator::createCombinator(pToConvert, ctx));
      }
      
      out.push_back(ComplexSelectorOrCombinator::createSelector(pToConvert, ctx));

      pToConvert = pToConvert->tail();
    }
  }
  
  Complex_Selector* CSOCToComplexSelector(const CSOCDeque& toConvert, Context& ctx) {
    if (!toConvert.empty() && toConvert.front().isCombinator()) {
      throw "The first element in the selector must not be a combinator.";
    }

    if (!toConvert.empty() && toConvert.back().isCombinator()) {
      throw "The last element in the selector must not be a combinator.";
    }
    
    Complex_Selector* pCurrent = NULL;
    Complex_Selector* pTail = NULL; // We're looping backwards, so this is the element after pCurrent
    
    // I'm using indexing instead of STL iteration here due to the private constructor on the CSOC object
    for (int index = toConvert.size() - 1; index >= 0; index--) {
      const ComplexSelectorOrCombinator& pCurrentCSOC = toConvert[index];
      pCurrent = pCurrentCSOC.selector()->clone(ctx);
      
      if (pCurrentCSOC.isCombinator()) {
        throw "This should never happen. Either this algorithm is busted or there were two combinators in a row (which shouldn't happen).";
      }

      if (index > 0) {
        const ComplexSelectorOrCombinator& pPreviousCSOC = toConvert[index - 1];
        
        if (pPreviousCSOC.isCombinator()) {
          pCurrent->combinator(pPreviousCSOC.combinator());
          index--; // skip over this combinator in our iteration now that we've consumed it
        }
      }
      
      pCurrent->tail(pTail);

			pTail = pCurrent;

    }
  
    return pCurrent;
  }
  
  
  

  
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
  
#ifdef DEBUG
  void printCombinator(Complex_Selector::Combinator combinator) {
    switch (combinator) {
      case Complex_Selector::ANCESTOR_OF: cerr << "\" \""; break;
      case Complex_Selector::PARENT_OF:   cerr << "\">\""; break;
      case Complex_Selector::PRECEDES:    cerr << "\"~\""; break;
      case Complex_Selector::ADJACENT_TO: cerr << "\"+\""; break;
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
    bool first = true;
    while (pIter) {
      if (pIter->combinator() != Complex_Selector::ANCESTOR_OF) {
        if (!first) {
          cerr << ", ";
        }
        first = false;
      	printCombinator(pIter->combinator());
      }

      if (!first) {
        cerr << ", ";
      }
      first = false;
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





  // Print a string representation of a Complex_Selector
  void printCSOCDeque(CSOCDeque& deque, const char* message=NULL, bool newline=true) {
		To_String to_string;

  	if (message) {
    	cerr << message;
    }

    cerr << "[";
    
    for (CSOCDeque::iterator iterator = deque.begin(), iteratorEnd = deque.end(); iterator != iteratorEnd; ++iterator) {
    	ComplexSelectorOrCombinator csoc = *iterator;
      
        if (iterator != deque.begin()) {
          cerr << ", ";
        }
      
      if (csoc.isCombinator()) {
				printCombinator(csoc.combinator());
      } else {
        cerr << csoc.selector()->head()->perform(&to_string);
      }
    }

    cerr << "]";

		if (newline) {
    	cerr << endl;
    }
  }

  void printCSOCDequeDeque(CSOCDequeDeque& dequeDeque, const char* message=NULL, bool newline=true) {
  	To_String to_string;

  	if (message) {
    	cerr << message;
    }
    
    cerr << "[";

    for (CSOCDequeDeque::iterator iterator = dequeDeque.begin(), iteratorEnd = dequeDeque.end(); iterator != iteratorEnd; ++iterator) {
      CSOCDeque& deque = *iterator;
      if (iterator != dequeDeque.begin()) {
      	cerr << ", ";
      }
      printCSOCDeque(deque, "", false /*newline*/);
    }

    cerr << "]";
    
    if (newline) {
      cerr << endl;
    }
  }
  
  // TODO: Can these prints be merged with templates and specialization
  void printCSOCDequeDequeDeque(CSOCDequeDequeDeque& dequeDequeDeque, const char* message=NULL, bool newline=true) {
  	To_String to_string;

  	if (message) {
    	cerr << message;
    }
    
    cerr << "[";

    for (CSOCDequeDequeDeque::iterator iterator = dequeDequeDeque.begin(), iteratorEnd = dequeDequeDeque.end(); iterator != iteratorEnd; ++iterator) {
      CSOCDequeDeque& dequeDeque = *iterator;
      if (iterator != dequeDequeDeque.begin()) {
      	cerr << ", ";
      }
      printCSOCDequeDeque(dequeDeque, "", false /*newline*/);
    }

    cerr << "]";
    
    if (newline) {
      cerr << endl;
    }
  }
#endif


  
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
  

  /*
   This is the equivalent of ruby's Sequence.trim.
   
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

#ifdef DEBUG
//        printComplexSelector(pSeq1, "TRIMASDF SEQ1: ");
//        printSourcesSet(sources, "TRIMASDF SOURCES: ");
#endif

        for (SourcesSet::iterator sourcesSetIterator = sources.begin(), sourcesSetIteratorEnd = sources.end(); sourcesSetIterator != sourcesSetIteratorEnd; ++sourcesSetIterator) {
         	const Complex_Selector* const pCurrentSelector = *sourcesSetIterator;
          maxSpecificity = max(maxSpecificity, pCurrentSelector->specificity());
        }
        
//        DEBUG_PRINTLN("MAX SPECIFIITY: " << maxSpecificity)

        bool isMoreSpecificOuter = false;

        for (ComplexSelectorDequeDeque::iterator resultIterator = result.begin(), resultIteratorEnd = result.end(); resultIterator != resultIteratorEnd; ++resultIterator) {
          ComplexSelectorDeque& seqs2 = *resultIterator;

#ifdef DEBUG
//          printSelectors(seqs1, "SEQS1: ");
//          printSelectors(seqs2, "SEQS2: ");
#endif

          if (complexSelectorDequesEqual(seqs1, seqs2)) {
//            DEBUG_PRINTLN("CONTINUE")
            continue;
          }
          
          bool isMoreSpecificInner = false;
          
          for (ComplexSelectorDeque::iterator seqs2Iterator = seqs2.begin(), seqs2IteratorEnd = seqs2.end(); seqs2Iterator != seqs2IteratorEnd; ++seqs2Iterator) {
            Complex_Selector* pSeq2 = *seqs2Iterator;
            
//            DEBUG_PRINTLN("SEQ2 SPEC: " << pSeq2->specificity())
//            DEBUG_PRINTLN("IS SUPER: " << pSeq2->is_superselector_of(pSeq1))
            
            isMoreSpecificInner = pSeq2->specificity() >= maxSpecificity && pSeq2->is_superselector_of(pSeq1);

            if (isMoreSpecificInner) {
//              DEBUG_PRINTLN("FOUND MORE SPECIFIC")
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
//          DEBUG_PRINTLN("PUSHING")
          tempResult.push_back(pSeq1);
        }
      }

      result[resultIndex] = tempResult;

      resultIndex++;
    }
    
    
    trimmed = result;
  }
  
  
  
  bool parentSuperselector(const CSOCDeque& one, const CSOCDeque& two, Context& ctx) {
  	// TODO: figure out a better way to create a Complex_Selector from scratch
    // TODO: There's got to be a better way. This got ugly quick...
    Position noPosition;
    Type_Selector fakeParent("", noPosition, "temp");
    Compound_Selector fakeHead("", noPosition, 1 /*size*/);
    fakeHead.elements().push_back(&fakeParent);
		Complex_Selector fakeParentContainer("", noPosition, Complex_Selector::ANCESTOR_OF, &fakeHead /*head*/, NULL /*tail*/);
    
    Complex_Selector* pOneWithFakeParent = CSOCToComplexSelector(one, ctx);
    pOneWithFakeParent->set_innermost(&fakeParentContainer, Complex_Selector::ANCESTOR_OF);
    Complex_Selector* pTwoWithFakeParent = CSOCToComplexSelector(two, ctx);
    pTwoWithFakeParent->set_innermost(&fakeParentContainer, Complex_Selector::ANCESTOR_OF);
    
    return pOneWithFakeParent->is_superselector_of(pTwoWithFakeParent);
  }
  
  
  bool parentSuperselector(const Node& one, const Node& two, Context& ctx) {
  	// TODO: figure out a better way to create a Complex_Selector from scratch
    // TODO: There's got to be a better way. This got ugly quick...
    Position noPosition;
    Type_Selector fakeParent("", noPosition, "temp");
    Compound_Selector fakeHead("", noPosition, 1 /*size*/);
    fakeHead.elements().push_back(&fakeParent);
		Complex_Selector fakeParentContainer("", noPosition, Complex_Selector::ANCESTOR_OF, &fakeHead /*head*/, NULL /*tail*/);
    
    Complex_Selector* pOneWithFakeParent = nodeToComplexSelector(one, ctx);
    pOneWithFakeParent->set_innermost(&fakeParentContainer, Complex_Selector::ANCESTOR_OF);
    Complex_Selector* pTwoWithFakeParent = nodeToComplexSelector(two, ctx);
    pTwoWithFakeParent->set_innermost(&fakeParentContainer, Complex_Selector::ANCESTOR_OF);
    
    return pOneWithFakeParent->is_superselector_of(pTwoWithFakeParent);
  }

  
  class ParentSuperselectorChunker {
  public:
  	ParentSuperselectorChunker(Node& lcs, Context& ctx) : mLcs(lcs), mCtx(ctx) {}
    Node& mLcs;
    Context& mCtx;

  	bool operator()(const Node& seq) const {
			// {|s| parent_superselector?(s.first, lcs.first)}
      return parentSuperselector(seq.collection()->front(), mLcs.collection()->front(), mCtx);
    }
  };
  
  class SubweaveEmptyChunker {
  public:
  	bool operator()(const Node& seq) const {
			// {|s| s.empty?}

      return seq.collection()->empty();
    }
  };
  
  /*
  # Takes initial subsequences of `seq1` and `seq2` and returns all
  # orderings of those subsequences. The initial subsequences are determined
  # by a block.
  #
  # Destructively removes the initial subsequences of `seq1` and `seq2`.
  #
  # For example, given `(A B C | D E)` and `(1 2 | 3 4 5)` (with `|`
  # denoting the boundary of the initial subsequence), this would return
  # `[(A B C 1 2), (1 2 A B C)]`. The sequences would then be `(D E)` and
  # `(3 4 5)`.
  #
  # @param seq1 [Array]
  # @param seq2 [Array]
  # @yield [a] Used to determine when to cut off the initial subsequences.
  #   Called repeatedly for each sequence until it returns true.
  # @yieldparam a [Array] A final subsequence of one input sequence after
  #   cutting off some initial subsequence.
  # @yieldreturn [Boolean] Whether or not to cut off the initial subsequence
  #   here.
  # @return [Array<Array>] All possible orderings of the initial subsequences.
  def chunks(seq1, seq2)
    chunk1 = []
    chunk1 << seq1.shift until yield seq1
    chunk2 = []
    chunk2 << seq2.shift until yield seq2
    return [] if chunk1.empty? && chunk2.empty?
    return [chunk2] if chunk1.empty?
    return [chunk1] if chunk2.empty?
    [chunk1 + chunk2, chunk2 + chunk1]
  end
  */
  template<typename ChunkerType>
  Node chunks(Node& seq1, Node& seq2, const ChunkerType& chunker) {
  	Node chunk1 = Node::createCollection();
    while (!chunker(seq1)) {
    	chunk1.collection()->push_back(seq1.collection()->front());
      seq1.collection()->pop_front();
    }
    
    Node chunk2 = Node::createCollection();
    while (!chunker(seq2)) {
    	chunk2.collection()->push_back(seq2.collection()->front());
      seq2.collection()->pop_front();
    }
    
    if (chunk1.collection()->empty() && chunk2.collection()->empty()) {
      DEBUG_PRINTLN("RETURNING BOTH EMPTY")
      return Node::createCollection();
    }
    
    if (chunk1.collection()->empty()) {
    	Node chunk2Wrapper = Node::createCollection();
    	chunk2Wrapper.collection()->push_back(chunk2);
      DEBUG_PRINTLN("RETURNING ONE EMPTY")
      return chunk2Wrapper;
    }
    
    if (chunk2.collection()->empty()) {
	    Node chunk1Wrapper = Node::createCollection();
      chunk1Wrapper.collection()->push_back(chunk1);
      DEBUG_PRINTLN("RETURNING TWO EMPTY")
      return chunk1Wrapper;
    }
    
    Node perms = Node::createCollection();
    
    Node firstPermutation = Node::createCollection();
    firstPermutation.collection()->insert(firstPermutation.collection()->end(), chunk1.collection()->begin(), chunk1.collection()->end());
    firstPermutation.collection()->insert(firstPermutation.collection()->end(), chunk2.collection()->begin(), chunk2.collection()->end());
    perms.collection()->push_back(firstPermutation);
    
    Node secondPermutation = Node::createCollection();
    secondPermutation.collection()->insert(secondPermutation.collection()->end(), chunk2.collection()->begin(), chunk2.collection()->end());
    secondPermutation.collection()->insert(secondPermutation.collection()->end(), chunk1.collection()->begin(), chunk1.collection()->end());
    perms.collection()->push_back(secondPermutation);
    
    DEBUG_PRINTLN("RETURNING PERM")
    
    return perms;
  }
  
  
  // Trying this out since I'm seeing weird behavior where the deque's are being emptied when calling into the templated version of chunks
  // TODO: use general version of chunks now that bug is fixed
  void chunksDeque(CSOCDequeDeque& seq1, CSOCDequeDeque& seq2, CSOCDequeDequeDeque& out, const SubweaveEmptyChunker& chunker) {
  	/*
#ifdef DEBUG
  	printCSOCDequeDeque(seq1, "ONE IN: ");
    printCSOCDequeDeque(seq2, "TWO IN: ");
#endif

  	CSOCDequeDeque chunk1;
    while (!chunker(seq1)) {
    	chunk1.push_back(seq1.front());
      seq1.pop_front();
    }
    
    CSOCDequeDeque chunk2;
    while (!chunker(seq2)) {
    	chunk2.push_back(seq2.front());
      seq2.pop_front();
    }
    
    if (chunk1.empty() && chunk2.empty()) {
      DEBUG_PRINTLN("RETURNING BOTH EMPTY")
      return;
    }
    
    if (chunk1.empty()) {
    	out.push_back(chunk2);
      DEBUG_PRINTLN("RETURNING ONE EMPTY")
      return;
    }
    
    if (chunk2.empty()) {
    	out.push_back(chunk1);
      DEBUG_PRINTLN("RETURNING TWO EMPTY")
      return;
    }
    
    CSOCDequeDeque firstPermutation;
    firstPermutation.insert(firstPermutation.end(), chunk1.begin(), chunk1.end());
    firstPermutation.insert(firstPermutation.end(), chunk2.begin(), chunk2.end());
    out.push_back(firstPermutation);
    
    CSOCDequeDeque secondPermutation;
    secondPermutation.insert(secondPermutation.end(), chunk2.begin(), chunk2.end());
    secondPermutation.insert(secondPermutation.end(), chunk1.begin(), chunk1.end());
    out.push_back(secondPermutation);
    
    DEBUG_PRINTLN("RETURNING PERM")
    */
  }
  
  
  template<typename CompareType>
  class DefaultLcsComparatorOld {
  public:
  	bool operator()(const CompareType& one, const CompareType& two, CompareType& out) const {
    	// TODO: Is this the correct C++ interpretation?
      // block ||= proc {|a, b| a == b && a}
      if (one == two) {
      	out = one;
        return true;
      }

      return false;
    }
  };
  
  class CSOCDequeLcsComparator {
  public:
  	CSOCDequeLcsComparator(Context& ctx) : mCtx(ctx) {}
    
    Context& mCtx;

  	bool operator()(const CSOCDeque& one, const CSOCDeque& two, CSOCDeque& out) const {
    	/*
      This code is based on the following block from ruby sass' subweave
				do |s1, s2|
          next s1 if s1 == s2
          next unless s1.first.is_a?(SimpleSequence) && s2.first.is_a?(SimpleSequence)
          next s2 if parent_superselector?(s1, s2)
          next s1 if parent_superselector?(s2, s1)
        end
      */

      if (one == two) {
      	out = one;
        return true;
      }

      if (!one.front().isSelector() || !two.front().isSelector()) {
      	return false;
      }
      
      if (parentSuperselector(one, two, mCtx)) {
      	out = two;
        return true;
      }
      
      if (parentSuperselector(two, one, mCtx)) {
      	out = one;
        return true;
      }

      return false;
    }
  };
  
  
  class LcsCollectionComparator {
  public:
  	LcsCollectionComparator(Context& ctx) : mCtx(ctx) {}
    
    Context& mCtx;

  	bool operator()(const Node& one, const Node& two, Node& out) const {
    	/*
      This code is based on the following block from ruby sass' subweave
				do |s1, s2|
          next s1 if s1 == s2
          next unless s1.first.is_a?(SimpleSequence) && s2.first.is_a?(SimpleSequence)
          next s2 if parent_superselector?(s1, s2)
          next s1 if parent_superselector?(s2, s1)
        end
      */

//      cerr << "S1:" << one << endl;
//      cerr << "S2:" << two << endl;
//
//      bool b1 = (one == two);
//      bool b2 = (one.collection()->front().isSelector() && two.collection()->front().isSelector());
//      bool b3 = (parentSuperselector(one, two, mCtx));
//      bool b4 = (parentSuperselector(two, one, mCtx));
//      cerr << b1 << " " << b2 << " " << b3 << " " << b4 << endl;
      
      if (one == two) {
      	out = one;
        return true;
      }
      
      if (!one.collection()->front().isSelector() || !two.collection()->front().isSelector()) {
      	return false;
      }
      
      if (parentSuperselector(one, two, mCtx)) {
      	out = two;
        return true;
      }
      
      if (parentSuperselector(two, one, mCtx)) {
      	out = one;
        return true;
      }

      return false;
    }
  };
  
  
  /*
  */
  template<typename DequeContentType, typename ComparatorType>
  void lcsBacktrace(const LCSTable& c, const deque<DequeContentType>& x, const deque<DequeContentType>& y, int i, int j, const ComparatorType& comparator, deque<DequeContentType>& out) {

  	if (i == 0 || j == 0) {
    	return;
    }

    DequeContentType compareOut;
    if (comparator(x[i], y[j], compareOut)) {
    	lcsBacktrace(c, x, y, i - 1, j - 1, comparator, out);
      out.push_back(compareOut);
    	return;
    }
    
    if (c[i][j - 1] > c[i - 1][j]) {
    	lcsBacktrace(c, x, y, i, j - 1, comparator, out);
      return;
    }
    
    lcsBacktrace(c, x, y, i - 1, j, comparator, out);
  }
  

  /*
  */
  template<typename DequeContentType, typename ComparatorType>
  void lcsTable(const deque<DequeContentType>& x, const deque<DequeContentType>& y, const ComparatorType& comparator, LCSTable& out) {

  	LCSTable c(x.size(), vector<int>(y.size()));
    
    // These shouldn't be necessary since the vector will be initialized to 0 already.
    // x.size.times {|i| c[i][0] = 0}
    // y.size.times {|j| c[0][j] = 0}

    for (int i = 1; i < x.size(); i++) {
    	for (int j = 1; j < y.size(); j++) {
        DequeContentType compareOut;

      	if (comparator(x[i], y[j], compareOut)) {
        	c[i][j] = c[i - 1][j - 1] + 1;
        } else {
        	c[i][j] = max(c[i][j - 1], c[i - 1][j]);
        }
      }
    }

    out = c;
  }

  
  /*
  */
  template<typename DequeContentType, typename ComparatorType>
  void lcs(const deque<DequeContentType>& x, const deque<DequeContentType>& y, const ComparatorType& comparator, deque<DequeContentType>& out) {
    
    deque<DequeContentType> newX(x);
    newX.push_front(DequeContentType());
    deque<DequeContentType> newY(y);
    newY.push_front(DequeContentType());

    LCSTable table;
    lcsTable(newX, newY, comparator, table);
    
    deque<DequeContentType> backtraceResult;
    lcsBacktrace(table, newX, newY, newX.size() - 1, newY.size() - 1, comparator, backtraceResult);
    
    out = backtraceResult;
  }
  
  
  Node groupSelectors(const Node& seq, Context& ctx) {
  	Node newSeq = Node::createCollection();
    
    Node tail = seq.clone(ctx);
    
    while (!tail.collection()->empty()) {
    	Node head = Node::createCollection();
      
      do {
      	head.collection()->push_back(tail.collection()->front());
        tail.collection()->pop_front();
      } while (!tail.collection()->empty() && (head.collection()->back().isCombinator() || tail.collection()->front().isCombinator()));
      
      newSeq.collection()->push_back(head);
    }
    
    return newSeq;
  }
  
  
  void getAndRemoveInitialOps(CSOCDeque& seq, CSOCDeque& ops) {
  	while (seq.size() > 0 && seq.front().isCombinator()) {
    	ops.push_back(seq.front());
      seq.pop_front();
    }
  }
  void getAndRemoveInitialOps(Node& seq, Node& ops) {
  	NodeDeque& seqCollection = *(seq.collection());
    NodeDeque& opsCollection = *(ops.collection());

  	while (seqCollection.size() > 0 && seqCollection.front().isCombinator()) {
    	opsCollection.push_back(seqCollection.front());
      seqCollection.pop_front();
    }
  }
  
  
  void getAndRemoveFinalOps(CSOCDeque& seq, CSOCDeque& ops) {
  	while (seq.size() > 0 && seq.back().isCombinator()) {
    	ops.push_back(seq.back()); // Purposefully reversed to match ruby code
      seq.pop_back();
    }
  }
  void getAndRemoveFinalOps(Node& seq, Node& ops) {
  	NodeDeque& seqCollection = *(seq.collection());
    NodeDeque& opsCollection = *(ops.collection());

  	while (seqCollection.size() > 0 && seqCollection.back().isCombinator()) {
    	opsCollection.push_back(seqCollection.back()); // Purposefully reversed to match ruby code
      seqCollection.pop_back();
    }
  }
  
  
  /*
      def merge_initial_ops(seq1, seq2)
        ops1, ops2 = [], []
        ops1 << seq1.shift while seq1.first.is_a?(String)
        ops2 << seq2.shift while seq2.first.is_a?(String)

        newline = false
        newline ||= !!ops1.shift if ops1.first == "\n"
        newline ||= !!ops2.shift if ops2.first == "\n"

        # If neither sequence is a subsequence of the other, they cannot be
        # merged successfully
        lcs = Sass::Util.lcs(ops1, ops2)
        return unless lcs == ops1 || lcs == ops2
        return (newline ? ["\n"] : []) + (ops1.size > ops2.size ? ops1 : ops2)
      end
  */
  Node mergeInitialOps(Node& seq1, Node& seq2, Context& ctx) {
  	Node ops1 = Node::createCollection();
    Node ops2 = Node::createCollection();
    
  	getAndRemoveInitialOps(seq1, ops1);
    getAndRemoveInitialOps(seq2, ops2);
    
		// TODO: Do we have this information available to us?
    // newline = false
    // newline ||= !!ops1.shift if ops1.first == "\n"
    // newline ||= !!ops2.shift if ops2.first == "\n"

		// If neither sequence is a subsequence of the other, they cannot be merged successfully
    DefaultLcsComparator lcsDefaultComparator;
    Node opsLcs = lcs(ops1, ops2, lcsDefaultComparator, ctx);
    
    if (!(opsLcs == ops1 || opsLcs == ops2)) {
    	return Node::createNil();
    }
    
    // TODO: more newline logic
    // return (newline ? ["\n"] : []) + (ops1.size > ops2.size ? ops1 : ops2)
    
    return (ops1.collection()->size() > ops2.collection()->size() ? ops1 : ops2);
  }

  bool mergeInitialOps(CSOCDeque& seq1, CSOCDeque& seq2, CSOCDeque& mergedOps) {
		CSOCDeque ops1;
    CSOCDeque ops2;

		getAndRemoveInitialOps(seq1, ops1);
    getAndRemoveInitialOps(seq2, ops2);

    CSOCDeque opsLcs;
    DefaultLcsComparatorOld<ComplexSelectorOrCombinator> defaultComparator;
    lcs<ComplexSelectorOrCombinator, DefaultLcsComparatorOld<ComplexSelectorOrCombinator> >(ops1, ops2, defaultComparator, opsLcs);
    
    if (!(opsLcs == ops1 || opsLcs == ops2)) {
    	return false;
    }
    
    // TODO: more newline logic
    // return (newline ? ["\n"] : []) + (ops1.size > ops2.size ? ops1 : ops2)
    
    mergedOps = (ops1.size() > ops2.size() ? ops1 : ops2);
    return true;
  }
  
  
  /*
      def merge_final_ops(seq1, seq2, res = [])


        # This code looks complicated, but it's actually just a bunch of special
        # cases for interactions between different combinators.
        op1, op2 = ops1.first, ops2.first
        if op1 && op2
          sel1 = seq1.pop
          sel2 = seq2.pop
          if op1 == '~' && op2 == '~'
            if sel1.superselector?(sel2)
              res.unshift sel2, '~'
            elsif sel2.superselector?(sel1)
              res.unshift sel1, '~'
            else
              merged = sel1.unify(sel2.members, sel2.subject?)
              res.unshift [
                [sel1, '~', sel2, '~'],
                [sel2, '~', sel1, '~'],
                ([merged, '~'] if merged)
              ].compact
            end
          elsif (op1 == '~' && op2 == '+') || (op1 == '+' && op2 == '~')
            if op1 == '~'
              tilde_sel, plus_sel = sel1, sel2
            else
              tilde_sel, plus_sel = sel2, sel1
            end

            if tilde_sel.superselector?(plus_sel)
              res.unshift plus_sel, '+'
            else
              merged = plus_sel.unify(tilde_sel.members, tilde_sel.subject?)
              res.unshift [
                [tilde_sel, '~', plus_sel, '+'],
                ([merged, '+'] if merged)
              ].compact
            end
          elsif op1 == '>' && %w[~ +].include?(op2)
            res.unshift sel2, op2
            seq1.push sel1, op1
          elsif op2 == '>' && %w[~ +].include?(op1)
            res.unshift sel1, op1
            seq2.push sel2, op2
          elsif op1 == op2
            return unless merged = sel1.unify(sel2.members, sel2.subject?)
            res.unshift merged, op1
          else
            # Unknown selector combinators can't be unified
            return
          end
          return merge_final_ops(seq1, seq2, res)
        elsif op1
          seq2.pop if op1 == '>' && seq2.last && seq2.last.superselector?(seq1.last)
          res.unshift seq1.pop, op1
          return merge_final_ops(seq1, seq2, res)
        else # op2
          seq1.pop if op2 == '>' && seq1.last && seq1.last.superselector?(seq2.last)
          res.unshift seq2.pop, op2
          return merge_final_ops(seq1, seq2, res)
        end
      end
  */
  Node mergeFinalOps(Node& seq1, Node& seq2, Context& ctx, Node& res) {
    
    Node ops1 = Node::createCollection();
    Node ops2 = Node::createCollection();
  
    getAndRemoveFinalOps(seq1, ops1);
    getAndRemoveFinalOps(seq2, ops2);
  
		// TODO: do we have newlines to remove?
    // ops1.reject! {|o| o == "\n"}
    // ops2.reject! {|o| o == "\n"}
    
		if (ops1.collection()->empty() && ops2.collection()->empty()) {
    	return res;
    }
  
		if (ops1.collection()->size() > 1 || ops2.collection()->size() > 1) {
    	DefaultLcsComparator lcsDefaultComparator;
    	Node opsLcs = lcs(ops1, ops2, lcsDefaultComparator, ctx);
      
      // If there are multiple operators, something hacky's going on. If one is a supersequence of the other, use that, otherwise give up.
      
      if (!(opsLcs == ops1 || opsLcs == ops2)) {
      	return Node::createNil();
      }
      
      if (ops1.collection()->size() > ops2.collection()->size()) {
      	res.collection()->insert(res.collection()->begin(), ops1.collection()->rbegin(), ops1.collection()->rend());
      } else {
      	res.collection()->insert(res.collection()->begin(), ops2.collection()->rbegin(), ops2.collection()->rend());
      }
      
      return res;
    }
    
    if (!ops1.collection()->empty() && !ops2.collection()->empty()) {

    	Node op1 = ops1.collection()->front();
     	Node op2 = ops2.collection()->front();
      
      Node sel1 = seq1.collection()->back();
      seq1.collection()->pop_back();
      
      Node sel2 = seq2.collection()->back();
      seq2.collection()->pop_back();

      if (op1.combinator() == Complex_Selector::PRECEDES && op2.combinator() == Complex_Selector::PRECEDES) {

      	if (sel1.selector()->is_superselector_of(sel2.selector())) {
        
        	res.collection()->push_front(op1 /*PRECEDES - could have been op2 as well*/);
          res.collection()->push_front(sel2);

        } else if (sel2.selector()->is_superselector_of(sel1.selector())) {

        	res.collection()->push_front(op1 /*PRECEDES - could have been op2 as well*/);
          res.collection()->push_front(sel1);

        } else {
        
//          merged = sel1.unify(sel2.members, sel2.subject?)
//          res.unshift [
//                       [sel1, '~', sel2, '~'],
//                       [sel2, '~', sel1, '~'],
//                       ([merged, '~'] if merged)
//                       ].compact
          
          Node newRes = Node::createCollection();
          
          Node firstPerm = Node::createCollection();
          firstPerm.collection()->push_back(sel1);
          firstPerm.collection()->push_back(Node::createCombinator(Complex_Selector::PRECEDES));
          firstPerm.collection()->push_back(sel2);
          firstPerm.collection()->push_back(Node::createCombinator(Complex_Selector::PRECEDES));
          newRes.collection()->push_back(firstPerm);

          Node secondPerm = Node::createCollection();
          secondPerm.collection()->push_back(sel2);
          secondPerm.collection()->push_back(Node::createCombinator(Complex_Selector::PRECEDES));
          secondPerm.collection()->push_back(sel1);
          secondPerm.collection()->push_back(Node::createCombinator(Complex_Selector::PRECEDES));
          newRes.collection()->push_back(secondPerm);

          Node merged = unify(sel1, sel2, ctx);
          if (merged.isCollection() && merged.collection()->size() > 0) {
            newRes.collection()->push_back(merged);
          }

          // TODO: Implement [].compact newRes
          
          res.collection()->push_front(newRes);

        }

      } else if (((op1.combinator() == Complex_Selector::PRECEDES && op2.combinator() == Complex_Selector::ADJACENT_TO)) || ((op1.combinator() == Complex_Selector::ADJACENT_TO && op2.combinator() == Complex_Selector::PRECEDES))) {
      
      		Node tildeSel = sel1;
          Node tildeOp = op1;
          Node plusSel = sel2;
          Node plusOp = op2;
      		if (op1.combinator() != Complex_Selector::PRECEDES) {
          	tildeSel = sel2;
            tildeOp = op2;
            plusSel = sel1;
            plusOp = op1;
          }
        
          if (tildeSel.selector()->is_superselector_of(plusSel.selector())) {

            res.collection()->push_front(plusOp);
            res.collection()->push_front(plusSel);

          } else {
          
            // TODO: does subject matter? Ruby: merged = plus_sel.unify(tilde_sel.members, tilde_sel.subject?)
            //Complex_Selector* pTildeSel = nodeToComplexSelector(tildeSel, ctx);
            Complex_Selector* pMerged = plusSel.selector()->clone(ctx);
            //pMerged->head(plusSel.selector()->head()->unify_with(pTildeSel->head(), ctx));
            // TODO: how to do this unification properly? Need example.
            
            Node newRes = Node::createCollection();
            
            Node firstPerm = Node::createCollection();
            firstPerm.collection()->push_back(tildeSel);
            firstPerm.collection()->push_back(Node::createCombinator(Complex_Selector::PRECEDES));
            firstPerm.collection()->push_back(plusSel);
            firstPerm.collection()->push_back(Node::createCombinator(Complex_Selector::ADJACENT_TO));
            newRes.collection()->push_back(firstPerm);
            
            if (pMerged) {
              Node mergedPerm = Node::createCollection();
              mergedPerm.collection()->push_back(complexSelectorToNode(pMerged, ctx));
              mergedPerm.collection()->push_back(Node::createCombinator(Complex_Selector::ADJACENT_TO));
              newRes.collection()->push_back(mergedPerm);
            }
            
            // TODO: Implement [].compact newRes
            
            res.collection()->push_front(newRes);
  
          }
      } else if (op1.combinator() == Complex_Selector::PARENT_OF && (op2.combinator() == Complex_Selector::PRECEDES || op2.combinator() == Complex_Selector::ADJACENT_TO)) {
      
      	res.collection()->push_front(op2);
        res.collection()->push_front(sel2);
        
        seq2.collection()->push_back(sel1);
        seq2.collection()->push_back(op1);

      } else if (op2.combinator() == Complex_Selector::PARENT_OF && (op1.combinator() == Complex_Selector::PRECEDES || op1.combinator() == Complex_Selector::ADJACENT_TO)) {

      	res.collection()->push_front(op1);
        res.collection()->push_front(sel1);
        
        seq2.collection()->push_back(sel2);
        seq2.collection()->push_back(op2);

      } else if (op1.combinator() == op2.combinator()) {

				// TODO: is this the right unification behavior? The ruby looks at all members, but sel2.selector() is just one thing...
        Compound_Selector* pMerged = sel1.selector()->head()->unify_with(sel2.selector()->head(), ctx);
        
        if (!pMerged) {
        	return Node::createNil();
        }
        
        Complex_Selector* pNewSelector = sel1.selector()->clone(ctx);
        pNewSelector->head(pMerged);
        
      	res.collection()->push_front(op1);
        res.collection()->push_front(Node::createSelector(pNewSelector, ctx));

      } else {
      	return Node::createNil();
      }

			return mergeFinalOps(seq1, seq2, ctx, res);
  
    } else if (!ops1.collection()->empty()) {

	    Node op1 = ops1.collection()->front();

    	if (op1.combinator() == Complex_Selector::PARENT_OF && !seq2.collection()->empty() && seq2.collection()->back().selector()->is_superselector_of(seq1.collection()->back().selector())) {
      	seq2.collection()->pop_back();
      }
      
      // TODO: consider unshift(NodeCollection, Node)
      res.collection()->push_front(op1);
      res.collection()->push_front(seq1.collection()->back());
      seq1.collection()->pop_back();

			return mergeFinalOps(seq1, seq2, ctx, res);

    } else { // !ops2.collection()->empty()

    	Node op2 = ops2.collection()->front();
      
      if (op2.combinator() == Complex_Selector::PARENT_OF && !seq1.collection()->empty() && seq1.collection()->back().selector()->is_superselector_of(seq2.collection()->back().selector())) {
      	seq1.collection()->pop_back();
      }
      
      res.collection()->push_front(op2);
      res.collection()->push_front(seq2.collection()->back());
      seq2.collection()->pop_back();

			return mergeFinalOps(seq1, seq2, ctx, res);

    }

  }
  
  
  /*
		This is the equivalent of ruby's Sequence.subweave.

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
    
    
    /*
    // Do the naive implementation. pOne = A B and pTwo = C D ...yields...  A B C D and C D A B
    // See https://gist.github.com/nex3/7609394 for details.
    Complex_Selector* pFirstPermutation = pOne->clone(ctx);
    pFirstPermutation->set_innermost(pTwo->clone(ctx), pFirstPermutation->innermost()->combinator()); // TODO: is this the correct combinator?
    out.push_back(pFirstPermutation);

    Complex_Selector* pSecondPermutation = pTwo->clone(ctx);
    pSecondPermutation->set_innermost(pOne->clone(ctx), pSecondPermutation->innermost()->combinator()); // TODO: is this the correct combinator?
    out.push_back(pSecondPermutation);

    
    return;*/
    

    
		// Convert to a data structure more equivalent to Ruby so we can perform these complex operations in the same manner.
    // Doing this clones the input, so this is equivalent to the ruby code's .dup
    Node seq1 = complexSelectorToNode(pOne, ctx);
    Node seq2 = complexSelectorToNode(pTwo, ctx);
    
#ifdef DEBUG
    DEBUG_PRINTLN("SUBWEAVE ONE: " << seq1)
    DEBUG_PRINTLN("SUBWEAVE TWO: " << seq2)
#endif

		Node init = mergeInitialOps(seq1, seq2, ctx);
    if (init.isNil()) {
    	return;
    }
    
#ifdef DEBUG
    DEBUG_PRINTLN("INIT: " << init)
#endif
    
    Node res = Node::createCollection();
		Node fin = mergeFinalOps(seq1, seq2, ctx, res);
    if (fin.isNil()) {
    	return;
    }
    
    DEBUG_PRINTLN("FIN: " << fin)


		// Moving this line up since fin isn't modified between now and when it happened before
    // fin.map {|sel| sel.is_a?(Array) ? sel : [sel]}

    for (NodeDeque::iterator finIter = fin.collection()->begin(), finEndIter = fin.collection()->end();
           finIter != finEndIter; ++finIter) {
      
      Node& childNode = *finIter;
      
      if (!childNode.isCollection()) {
      	Node wrapper = Node::createCollection();
        wrapper.collection()->push_back(childNode);
        childNode = wrapper;
      }

    }

		DEBUG_PRINTLN("FIN MAPPED: " << fin)



    Node groupSeq1 = groupSelectors(seq1, ctx);
    DEBUG_PRINTLN("SEQ1: " << groupSeq1)
    
    Node groupSeq2 = groupSelectors(seq2, ctx);
    DEBUG_PRINTLN("SEQ2: " << groupSeq2)


    LcsCollectionComparator collectionComparator(ctx);
    Node seqLcs = lcs(groupSeq2, groupSeq1, collectionComparator, ctx);
    
    DEBUG_PRINTLN("SEQLCS: " << seqLcs)


		Node initWrapper = Node::createCollection();
    initWrapper.collection()->push_back(init);
		Node diff = Node::createCollection();
    diff.collection()->push_back(initWrapper);

    DEBUG_PRINTLN("DIFF INIT: " << diff)
    
    
    while (!seqLcs.collection()->empty()) {
    	ParentSuperselectorChunker superselectorChunker(seqLcs, ctx); // TODO: rename this to parent super selector chunker
      Node chunksResult = chunks(groupSeq1, groupSeq2, superselectorChunker);
      diff.collection()->push_back(chunksResult);
      
      Node lcsWrapper = Node::createCollection();
      lcsWrapper.collection()->push_back(seqLcs.collection()->front());
      seqLcs.collection()->pop_front();
      diff.collection()->push_back(lcsWrapper);
    
			groupSeq1.collection()->pop_front();
      groupSeq2.collection()->pop_front();
    }
    
    DEBUG_PRINTLN("DIFF POST LCS: " << diff)
    
    
    DEBUG_PRINTLN("CHUNKS: ONE=" << groupSeq1 << " TWO=" << groupSeq2)
    

    SubweaveEmptyChunker emptyChunker;
    Node chunksResult = chunks(groupSeq1, groupSeq2, emptyChunker);
    diff.collection()->push_back(chunksResult);
    
    
    DEBUG_PRINTLN("DIFF POST CHUNKS: " << diff)
    

    diff.collection()->insert(diff.collection()->end(), fin.collection()->begin(), fin.collection()->end());
    
    DEBUG_PRINTLN("DIFF POST FIN MAPPED: " << diff)

    // JMA - filter out the empty nodes (use a new collection, since iterator erase() invalidates the old collection)
    Node diffFiltered = Node::createCollection();
    for (NodeDeque::iterator diffIter = diff.collection()->begin(), diffEndIter = diff.collection()->end();
           diffIter != diffEndIter; ++diffIter) {
    	Node& node = *diffIter;
      if (node.collection() && !node.collection()->empty()) {
        diffFiltered.collection()->push_back(node);
      }
    }
    diff = diffFiltered;
    
    DEBUG_PRINTLN("DIFF POST REJECT: " << diff)
    
    
		Node pathsResult = paths(diff, ctx);
    
    DEBUG_PRINTLN("PATHS: " << pathsResult)
    

		// We're flattening in place
    for (NodeDeque::iterator pathsIter = pathsResult.collection()->begin(), pathsEndIter = pathsResult.collection()->end();
			pathsIter != pathsEndIter; ++pathsIter) {

    	Node& child = *pathsIter;
      child = flatten(child, ctx);
    }
    
    DEBUG_PRINTLN("FLATTENED: " << pathsResult);
    
    
    /*
      TODO: implement
      rejected = mapped.reject {|p| path_has_two_subjects?(p)}
      $stderr.puts "REJECTED: #{rejected}"
     */
    DEBUG_PRINTLN("REJECTED: " << pathsResult)
    
  
    // Convert back to the data type the rest of the code expects.
    for (NodeDeque::iterator resultIter = pathsResult.collection()->begin(), resultEndIter = pathsResult.collection()->end();
			resultIter != resultEndIter; ++resultIter) {

    	Node& outNode = *resultIter;
      out.push_back(nodeToComplexSelector(outNode, ctx));

    }

  }
  

  /*
   This is the equivalent of ruby's Sequence.weave.
   
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
  


  /*
   This is the equivalent of ruby's Sass::Util.paths.
   
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
  
  void paths(CSOCDequeDequeDeque& source, CSOCDequeDequeDeque& out, Context& ctx) {
  	/*
    To_String to_string;
    
    CSOCDequeDequeDeque loopStart;

    for (CSOCDequeDequeDeque::iterator arrsIterator = source.begin(), endIterator = source.end();
         arrsIterator != endIterator; ++arrsIterator) {
      
      CSOCDequeDeque& arr = *arrsIterator;
      
    	CSOCDequeDequeDeque permutations;

      for (CSOCDequeDeque::iterator arrIterator = arr.begin(), endIterator = arr.end();
           arrIterator != endIterator; ++arrIterator) {
      	Complex_Selector* pE = (*arrIterator)->clone(ctx);
    
        if (loopStart.size() == 0) {
          // When the loopStart has nothing in it, we're on the first iteration. The new permutation
          // is just the current Complex_Selector*. Without this special case, we would never loop
          // over anything in the for loop in the below else clause.
          CSOCDequeDeque newPermutation;
          newPermutation.push_back(pE);
          
          permutations.push_back(newPermutation);
        } else {
          for (CSOCDequeDequeDeque::iterator loopStartIterator = loopStart.begin(), endIterator = loopStart.end();
               loopStartIterator != endIterator; ++loopStartIterator) {
            CSOCDequeDeque& path = *loopStartIterator;
            
            CSOCDequeDeque newPermutation;
            cloneComplexSelectorDeque(path, newPermutation, ctx);
            newPermutation.push_back(pE);
            
            permutations.push_back(newPermutation);
          }
        }
        
      }
      
      loopStart = permutations;
    }
    
    out = loopStart;
    */
  }
  
  // TODO: replace paths with this function
  /*
  template<typename ContentType>
  void pathsGeneral(deque<deque<ContentType> >& source, deque<deque<ContentType> >& out, Context& ctx) {
		typedef deque<ContentType> Deque;
    typedef deque<Deque> DequeDeque;

    DequeDeque loopStart;

    for (typename DequeDeque::iterator arrsIterator = source.begin(), endIterator = source.end();
         arrsIterator != endIterator; ++arrsIterator) {
      
      Deque& arr = *arrsIterator;
      
    	DequeDeque permutations;

      for (typename Deque::iterator arrIterator = arr.begin(), endIterator = arr.end();
           arrIterator != endIterator; ++arrIterator) {
      	Complex_Selector* pE = (*arrIterator)->clone(ctx);
    
        if (loopStart.size() == 0) {
          // When the loopStart has nothing in it, we're on the first iteration. The new permutation
          // is just the current Complex_Selector*. Without this special case, we would never loop
          // over anything in the for loop in the below else clause.
          Deque newPermutation;
          newPermutation.push_back(pE);
          
          permutations.push_back(newPermutation);
        } else {
          for (typename DequeDeque::iterator loopStartIterator = loopStart.begin(), endIterator = loopStart.end();
               loopStartIterator != endIterator; ++loopStartIterator) {
            Deque& path = *loopStartIterator;
            
            Deque newPermutation;
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
  */

  

  // complexSelectorDequeContains checks if an equivalent Complex_Selector to the one passed in is contained within the
  // passed in ComplexSelectorDeque. This is necessary because the deque contains pointers, and pointer comparison yields
  // strict object equivalency. We want to compare the selector's contents.
  //
  // TODO: move ComplexSelectorPointerComparator to ast.hpp next to the other one aimed at set usage? This is aimed at usage for std::find_if in complexSelectorDequeContains. One could be implemented in terms of the other for less code duplication. Can this be removed entirely now that I implemented operator== on the Complex_Selector class?
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
  
  
  
  /*
   This is the equivalent of ruby's SimpleSequence.do_extend.
   
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
    Complex_Selector::Combinator sourceCombinator,
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
      // JMA - copy the combinator from the selector we're extending (sourceCombinator)
      Complex_Selector* pNewSelector = pExtComplexSelector->clone(ctx);
      Complex_Selector* pNewInnerMost = new (ctx.mem) Complex_Selector(pSelector->path(), pSelector->position(), sourceCombinator, pUnifiedSelector, NULL);
      Complex_Selector::Combinator combinator = pNewSelector->clear_innermost();
      pNewSelector->set_innermost(pNewInnerMost, combinator);
      


      // Set the sources on our new Complex_Selector to the sources of this simple sequence plus the thing we're extending.
#ifdef DEBUG
//      printComplexSelector(pNewSelector, "ASDF SETTING ON: ");
#endif

      SourcesSet newSourcesSet = pSelector->sources();
#ifdef DEBUG
//      printSourcesSet(newSourcesSet, "ASDF SOURCES THIS: ");
#endif

      newSourcesSet.insert(pExtComplexSelector->clone(ctx));
#ifdef DEBUG
//      printSourcesSet(newSourcesSet, "ASDF NEW: ");
#endif

      pNewSelector->addSources(newSourcesSet, ctx);
#ifdef DEBUG
//      SourcesSet newSet = pNewSelector->sources();
//      printSourcesSet(newSet, "ASDF NEW AFTER SET: ");
//      printSourcesSet(pSelector->sources(), "ASDF SOURCES THIS SHOULD BE SAME: ");
#endif


      ComplexSelectorDeque recurseExtendedSelectors;
      set<Compound_Selector> recurseSeen(seen);
      recurseSeen.insert(*pExtCompoundSelector);


#ifdef DEBUG
			printComplexSelector(pNewSelector, "RECURSING DO EXTEND: ", true);
#endif
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
 

  
  /*
   This is the equivalent of ruby's Sequence.do_extend.
   
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
      extendCompoundSelector(pCompoundSelector, pCurrentComplexSelector->combinator(), ctx, subsetMap, seen, extended /*out*/);
#ifdef DEBUG
      printComplexSelectorDeque(extended, "EXTENDED: ");
#endif

      
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

#ifdef DEBUG
      printComplexSelectorDeque(extended, "CHOICES UNSHIFTED: ");
#endif
      
      // Aggregate our current extensions
      extendedNotExpanded.push_back(extended);
    

      // Continue our iteration
    	pCurrentComplexSelector = pCurrentComplexSelector->tail();
    }

#ifdef DEBUG
    printComplexSelectorDequeDeque(extendedNotExpanded, "EXTENDED NOT EXPANDED: ");
#endif
  
    // Ruby Equivalent: paths
    ComplexSelectorDequeDeque permutations;
    paths(extendedNotExpanded, permutations, ctx);
#ifdef DEBUG
    printComplexSelectorDequeDeque(permutations, "PATHS: ");
#endif

    // Ruby Equivalent: weave
 		ComplexSelectorDequeDeque weaves;
    for (ComplexSelectorDequeDeque::iterator iterator = permutations.begin(), endIterator = permutations.end();
         iterator != endIterator; ++iterator) {
      ComplexSelectorDeque& toWeave = *iterator;
      
      ComplexSelectorDeque weaved;
			weave(toWeave, ctx, weaved);
      
      weaves.push_back(weaved);
    }

#ifdef DEBUG
    printComplexSelectorDequeDeque(weaves, "WEAVES: ");
#endif
    
    // Ruby Equivalent: trim
    ComplexSelectorDequeDeque trimmed;
    trim(weaves, trimmed, ctx);
#ifdef DEBUG
		printComplexSelectorDequeDeque(trimmed, "TRIMMED: ");
#endif

    
    // Ruby Equivalent: flatten
    for (ComplexSelectorDequeDeque::iterator iterator = trimmed.begin(), endIterator = trimmed.end();
         iterator != endIterator; ++iterator) {
      
      ComplexSelectorDeque& toCombine = *iterator;
      extendedSelectors.insert(extendedSelectors.end(), toCombine.begin(), toCombine.end());
    }
    
#ifdef DEBUG
    printComplexSelectorDeque(extendedSelectors, ">>>>> EXTENDED: ");
#endif
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
  
  

  Extend::Extend(Context& ctx, ExtensionSubsetMap& ssm)
  : ctx(ctx), subset_map(ssm)
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
