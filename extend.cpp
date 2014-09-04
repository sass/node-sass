#include "extend.hpp"
#include "context.hpp"
#include "contextualize.hpp"
#include "to_string.hpp"
#include "backtrace.hpp"
#include "paths.hpp"
#include "parser.hpp"
#include "node.hpp"
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

  
  class SubweaveSuperselectorChunker {
  public:
  	SubweaveSuperselectorChunker(CSOCDequeDeque& other, Context& ctx) : mOther(other), mCtx(ctx) {}
    CSOCDequeDeque& mOther;
    Context& mCtx;

  	bool operator()(const CSOCDequeDeque& seq) const {
			// {|s| parent_superselector?(s.first, lcs.first)}
      return parentSuperselector(seq.front(), mOther.front(), mCtx);
    }
  };
  
  class SubweaveEmptyChunker {
  public:
  	bool operator()(CSOCDequeDeque& seq) const {
			// {|s| s.empty?}
      printCSOCDequeDeque(seq, "EMPTY CHECK: ");
      
      cerr << "ISEMPTY: " << seq.empty() << endl;
      
      cerr << "SIZE: " << seq.size() << endl;
      
      int computedSize = 0;
      for (CSOCDequeDeque::iterator iterator = seq.begin(), iteratorEnd = seq.end(); iterator != iteratorEnd; ++iterator) {
        computedSize++;
      }
      cerr << "COMP SIZE: " << computedSize << endl;

      return seq.empty();
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
  template<typename DequeContentType, typename ChunkerType>
  void chunks(deque<DequeContentType>& seq1, deque<DequeContentType>& seq2, deque<deque<DequeContentType> >& out, const ChunkerType& chunker) {
  	typedef deque<DequeContentType> Deque;

  	Deque chunk1;
    while (!chunker(seq1)) {
    	chunk1.push_back(seq1.front());
      seq1.pop_front();
    }
    
    Deque chunk2;
    while (!chunker(seq2)) {
    	chunk2.push_back(seq2.front());
      seq2.pop_front();
    }
    
    if (chunk1.empty() && chunk2.empty()) {
    	out.clear();
      cerr << "RETURNING BOTH EMPTY" << endl;
      return;
    }
    
    if (chunk1.empty()) {
    	out.push_back(chunk2);
      cerr << "RETURNING ONE EMPTY" << endl;
      return;
    }
    
    if (chunk2.empty()) {
    	out.push_back(chunk1);
      cerr << "RETURNING TWO EMPTY" << endl;
      return;
    }
    
    Deque firstPermutation;
    firstPermutation.insert(firstPermutation.end(), chunk1.begin(), chunk1.end());
    firstPermutation.insert(firstPermutation.end(), chunk2.begin(), chunk2.end());
    out.push_back(firstPermutation);
    
    Deque secondPermutation;
    secondPermutation.insert(secondPermutation.end(), chunk2.begin(), chunk2.end());
    secondPermutation.insert(secondPermutation.end(), chunk1.begin(), chunk1.end());
    out.push_back(secondPermutation);
    
    cerr << "RETURNING PERM" << endl;
  }
  
  
  // Trying this out since I'm seeing weird behavior where the deque's are being emptied when calling into the templated version of chunks
  // TODO: use general version of chunks now that bug is fixed
  void chunksDeque(CSOCDequeDeque& seq1, CSOCDequeDeque& seq2, CSOCDequeDequeDeque& out, const SubweaveEmptyChunker& chunker) {
  	printCSOCDequeDeque(seq1, "ONE IN: ");
    printCSOCDequeDeque(seq2, "TWO IN: ");

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
      cerr << "RETURNING BOTH EMPTY" << endl;
      return;
    }
    
    if (chunk1.empty()) {
    	out.push_back(chunk2);
      cerr << "RETURNING ONE EMPTY" << endl;
      return;
    }
    
    if (chunk2.empty()) {
    	out.push_back(chunk1);
      cerr << "RETURNING TWO EMPTY" << endl;
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
    
    cerr << "RETURNING PERM" << endl;
  }
  
  
  template<typename CompareType>
  class DefaultLcsComparator {
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
      
      if (one.front().isSelector() && two.front().isSelector()) {
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
  
  
  void groupSelectors(const CSOCDeque& in, CSOCDequeDeque& out) {
  	CSOCDeque tail(in);
    
    while (!tail.empty()) {
    	CSOCDeque head;
      
      do {
      	head.push_back(tail.front());
        tail.pop_front();
      } while (!tail.empty() && (head.back().isCombinator() || tail.front().isCombinator()));
      
      out.push_back(head);
    }
  }
  
  
  void getAndRemoveInitialOps(CSOCDeque& seq, CSOCDeque& ops) {
  	while (seq.size() > 0 && seq.front().isCombinator()) {
    	ops.push_back(seq.front());
      seq.pop_front();
    }
  }
  
  
  void getAndRemoveFinalOps(CSOCDeque& seq, CSOCDeque& ops) {
  	while (seq.size() > 0 && seq.back().isCombinator()) {
    	ops.push_back(seq.back()); // Purposefully reversed to match ruby code
      seq.pop_back();
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
  bool mergeInitialOps(CSOCDeque& seq1, CSOCDeque& seq2, CSOCDeque& mergedOps) {
		CSOCDeque ops1;
    CSOCDeque ops2;

		getAndRemoveInitialOps(seq1, ops1);
    getAndRemoveInitialOps(seq2, ops2);

		// TODO: Do we have this information available to us?
    // newline = false
    // newline ||= !!ops1.shift if ops1.first == "\n"
    // newline ||= !!ops2.shift if ops2.first == "\n"
    
    CSOCDeque opsLcs;
    DefaultLcsComparator<ComplexSelectorOrCombinator> defaultComparator;
    lcs<ComplexSelectorOrCombinator, DefaultLcsComparator<ComplexSelectorOrCombinator> >(ops1, ops2, defaultComparator, opsLcs);
    
    if (!(opsLcs == ops1 || opsLcs == ops2)) {
    	return false;
    }
    
    // TODO: more newline logic
    // return (newline ? ["\n"] : []) + (ops1.size > ops2.size ? ops1 : ops2)
    
    mergedOps = (ops1.size() > ops2.size() ? ops1 : ops2);
    return true;
  }
  
  
  bool mergeFinalOps(CSOCDeque& seq1, CSOCDeque& seq2, CSOCDeque& mergedOps, Context& ctx) {
		CSOCDeque ops1;
    CSOCDeque ops2;
    
    getAndRemoveFinalOps(seq1, ops1);
    getAndRemoveFinalOps(seq2, ops2);

		// TODO: do we have newlines to remove?
    // ops1.reject! {|o| o == "\n"}
    // ops2.reject! {|o| o == "\n"}

		if (ops1.empty() && ops2.empty()) {
    	return true;
    }

		if (ops1.size() > 1 || ops2.size() > 1) {
    	CSOCDeque opsLcs;
      DefaultLcsComparator<ComplexSelectorOrCombinator> defaultComparator;
      lcs<ComplexSelectorOrCombinator, DefaultLcsComparator<ComplexSelectorOrCombinator> >(ops1, ops2, defaultComparator, opsLcs);


      if (!(opsLcs == ops1 || opsLcs == ops2)) {
      	return false;
      }
      
      if (ops1.size() > ops2.size()) {
      	mergedOps.insert(mergedOps.begin(), ops1.begin(), ops1.end());
      } else {
      	mergedOps.insert(mergedOps.begin(), ops2.begin(), ops2.end());
      }
      
      reverse(mergedOps.begin(), mergedOps.end());
      
      return true;
    }
    
    if (!ops1.empty() && !ops2.empty()) {
    	ComplexSelectorOrCombinator op1 = ops1.front();
      ComplexSelectorOrCombinator op2 = ops2.front();
      
      ComplexSelectorOrCombinator sel1 = seq1.back();
      seq1.pop_back();
      
      ComplexSelectorOrCombinator sel2 = seq2.back();
      seq2.pop_back();
      
      if (op1.combinator() == Complex_Selector::PRECEDES && op2.combinator() == Complex_Selector::PRECEDES) {

      	if (sel1.selector()->is_superselector_of(sel2.selector())) {

        	mergedOps.push_front(op1 /*PRECEDES - could have been op2 as well*/);
          mergedOps.push_front(sel2);

        } else if (sel2.selector()->is_superselector_of(sel1.selector())) {

        	mergedOps.push_front(op1 /*PRECEDES - could have been op2 as well*/);
          mergedOps.push_front(sel1);

        } else {
        
        	throw "Not Yet Implemented. Should we allow chaining CSOCs or to create a CSOCDequeDeque?";

        			/*
              merged = sel1.unify(sel2.members, sel2.subject?)
              res.unshift [
                [sel1, '~', sel2, '~'],
                [sel2, '~', sel1, '~'],
                ([merged, '~'] if merged)
              ].compact
              */

        }

      } else if (((op1.combinator() == Complex_Selector::PRECEDES && op2.combinator() == Complex_Selector::ADJACENT_TO)) || ((op1.combinator() == Complex_Selector::ADJACENT_TO && op2.combinator() == Complex_Selector::PRECEDES))) {
      
      		ComplexSelectorOrCombinator tildeSel = sel1;
          ComplexSelectorOrCombinator tildeOp = op1;
          ComplexSelectorOrCombinator plusSel = sel2;
          ComplexSelectorOrCombinator plusOp = op2;
      		if (op1.combinator() != Complex_Selector::PRECEDES) {
          	tildeSel = sel2;
            tildeOp = op2;
            plusSel = sel1;
            plusOp = op1;
          }
        
          if (tildeSel.selector()->is_superselector_of(plusSel.selector())) {
          	mergedOps.push_front(plusOp);
            mergedOps.push_front(plusSel);
          } else {
          
          	throw "Not Yet Implemented. Should we allow chaining CSOCs or to create a CSOCDequeDeque?";
          
          	/*
              merged = plus_sel.unify(tilde_sel.members, tilde_sel.subject?)
              res.unshift [
                [tilde_sel, '~', plus_sel, '+'],
                ([merged, '+'] if merged)
              ].compact
            */
          }
      } else if (op1.combinator() == Complex_Selector::PARENT_OF && (op2.combinator() == Complex_Selector::PRECEDES || op2.combinator() == Complex_Selector::ADJACENT_TO)) {
      
      	mergedOps.push_front(op2);
        mergedOps.push_front(sel2);
        
        seq2.push_back(sel1);
        seq2.push_back(op1);

      } else if (op2.combinator() == Complex_Selector::PARENT_OF && (op1.combinator() == Complex_Selector::PRECEDES || op1.combinator() == Complex_Selector::ADJACENT_TO)) {

      	mergedOps.push_front(op1);
        mergedOps.push_front(sel1);
        
        seq2.push_back(sel2);
        seq2.push_back(op2);

      } else if (op1.combinator() == op2.combinator()) {

        Compound_Selector* pMerged = sel1.selector()->head()->unify_with(sel2.selector()->head(), ctx);
        
        if (!pMerged) {
        	return false;
        }
        
        Complex_Selector* pNewSelector = sel1.selector()->clone(ctx);
        pNewSelector->head(pMerged);
        
      	mergedOps.push_front(op1);
        mergedOps.push_front(ComplexSelectorOrCombinator::createSelector(pNewSelector, ctx));

      } else {
      	return false;
      }

			return mergeFinalOps(seq1, seq2, mergedOps, ctx);

    } else if (!ops1.empty()) {

	    ComplexSelectorOrCombinator op1 = ops1.front();

    	if (op1.combinator() == Complex_Selector::PARENT_OF && !seq2.empty() && seq2.back().selector()->is_superselector_of(seq1.back().selector())) {
      	seq2.pop_back();
      }

      mergedOps.push_front(op1);
      mergedOps.push_front(seq1.back());
      seq1.pop_back();

			return mergeFinalOps(seq1, seq2, mergedOps, ctx);

    } else { // !ops2.empty()

    	ComplexSelectorOrCombinator op2 = ops2.front();
      
      if (op2.combinator() == Complex_Selector::PARENT_OF && !seq1.empty() && seq1.back().selector()->is_superselector_of(seq2.back().selector())) {
      	seq1.pop_back();
      }
      
      mergedOps.push_front(op2);
      mergedOps.push_front(seq2.back());
      seq2.pop_back();

			return mergeFinalOps(seq1, seq2, mergedOps, ctx);

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
    
    
    
    /*

    // Do the naive implementation. pOne = A B and pTwo = C D ...yields...  A B C D and C D A B
    // See https://gist.github.com/nex3/7609394 for details.
    Complex_Selector* pFirstPermutation = pOne->clone(ctx);
    pFirstPermutation->set_innermost(pTwo->clone(ctx), pFirstPermutation->innermost()->combinator()); // TODO: is this the correct combinator?
    out.push_back(pFirstPermutation);

    Complex_Selector* pSecondPermutation = pTwo->clone(ctx);
    pSecondPermutation->set_innermost(pOne->clone(ctx), pSecondPermutation->innermost()->combinator()); // TODO: is this the correct combinator?
    out.push_back(pSecondPermutation);
    */
    printComplexSelector(pOne, "SUBWEAVE ONE: ");
    printComplexSelector(pTwo, "SUBWEAVE TWO: ");
    

		// Converting to CSOCDeque clones the inputs, so this is equivalent to the ruby code's .dup
    CSOCDeque seq1;
    complexSelectorToCSOC(pOne, seq1, ctx);
    
    CSOCDeque seq2;
    complexSelectorToCSOC(pTwo, seq2, ctx);
    
    
    printCSOCDeque(seq1, "SUBWEAVE ONE: ");
    printCSOCDeque(seq2, "SUBWEAVE TWO: ");
    
    
    CSOCDeque init;
    if (!mergeInitialOps(seq1, seq2, init)) {
    	return;
    }
    printCSOCDeque(init, "INIT: ");
    
    CSOCDequeDeque fin;
    /*
    if (!mergeFinalOps(seq1, seq2, fin, ctx)) {
    	return;
    }
    */
    /*
    This code may be useful for mapping fin:
    
    // fin.map {|sel| sel.is_a?(Array) ? sel : [sel]}
    for (CSOCDeque::iterator finIterator = fin.begin(), finEndIterator = fin.end();
           finIterator != finEndIterator; ++finIterator) {
      CSOCDeque wrapper;
      wrapper.push_back(*finIterator);
      diff.push_back(wrapper);
    }
    */
    cerr << "FIN: <NO EQUIVALENT>" << endl;
    printCSOCDequeDeque(fin, "FIN MAPPED: ");
    
    
    CSOCDequeDeque groupSeq1;
    groupSelectors(seq1, groupSeq1);
    printCSOCDequeDeque(groupSeq1, "GROUP1: ");
    
    CSOCDequeDeque groupSeq2;
    groupSelectors(seq2, groupSeq2);
    printCSOCDequeDeque(groupSeq2, "GROUP2: ");
    
    CSOCDequeDeque seqLcs;
    CSOCDequeLcsComparator dequeComparator(ctx);
    lcs<CSOCDeque, CSOCDequeLcsComparator>(groupSeq2, groupSeq1, dequeComparator, seqLcs);
    printCSOCDequeDeque(seqLcs, "SEQLCS: ");

    CSOCDequeDeque diffWrapper;
    diffWrapper.push_back(init);
    CSOCDequeDequeDeque diff;
    diff.push_back(diffWrapper);
    printCSOCDequeDequeDeque(diff, "DIFF INIT: ");
    
    while (!seqLcs.empty()) {
    	SubweaveSuperselectorChunker superselectorChunker(seqLcs, ctx); // TODO: rename this to parent super selector chunker
      
      chunks(groupSeq1, groupSeq2, diff, superselectorChunker);
      
      CSOCDequeDeque lcsContainer;
      lcsContainer.push_back(seqLcs.front());
      diff.push_back(lcsContainer);
      seqLcs.pop_front();
    
			groupSeq1.pop_front();
      groupSeq2.pop_front();
    }
    
    printCSOCDequeDequeDeque(diff, "DIFF POST LCS: ");
    
    cerr << "CHUNKS: ONE=";
    printCSOCDequeDeque(groupSeq1, "", false);
    cerr << " TWO=";
    printCSOCDequeDeque(groupSeq2, "", false);
    cerr << endl;
    
    SubweaveEmptyChunker emptyChunker;
    //chunks<CSOCDeque, SubweaveEmptyChunker>(groupSeq1, groupSeq2, diff, emptyChunker);
    chunksDeque(groupSeq1, groupSeq2, diff, emptyChunker);
    
    
    printCSOCDequeDequeDeque(diff, "DIFF POST CHUNKS: ");


		// TODO: finish implementing mergeFinalOps. Then the stuff in fin should be in an array already
    // diff += fin.map {|sel| sel.is_a?(Array) ? sel : [sel]}
    diff.push_back(fin);
    
    printCSOCDequeDequeDeque(diff, "DIFF POST FIN MAPPED: ");
    
    
    for (CSOCDequeDequeDeque::iterator diffIterator = diff.begin(), diffEndIterator = diff.end();
           diffIterator != diffEndIterator; ++diffIterator) {
    	CSOCDequeDeque& dequeDeque = *diffIterator;
      if (dequeDeque.empty()) {
      	diffIterator = diff.erase(diffIterator);
      }
    }
    
    printCSOCDequeDequeDeque(diff, "DIFF POST REJECT: ");
    
    

		CSOCDequeDequeDeque pathsTemp; // TODO: rename this
		//paths(diff, pathsTemp, ctx);

		printCSOCDequeDequeDeque(pathsTemp, "PATHS: ");

    
    CSOCDequeDeque flattened;
    for (CSOCDequeDequeDeque::iterator pathsIterator = pathsTemp.begin(), pathsEndIterator = pathsTemp.end();
           pathsIterator != pathsEndIterator; ++pathsIterator) {
    	CSOCDequeDeque& dequeDeque = *pathsIterator;

			CSOCDeque currentFlattened;
      
      for (CSOCDequeDeque::iterator dequeDequeIterator = dequeDeque.begin(), dequeDequeEndIterator = dequeDeque.end();
             dequeDequeIterator != dequeDequeEndIterator; ++dequeDequeIterator) {
        CSOCDeque& innermostFlatten = *dequeDequeIterator;
      	currentFlattened.insert(currentFlattened.end(), innermostFlatten.begin(), innermostFlatten.end());
      }
      
      flattened.push_back(currentFlattened);
    }
    
    printCSOCDequeDeque(flattened, "FLATTENED: ");


			/*
      	TODO: implement
        rejected = mapped.reject {|p| path_has_two_subjects?(p)}
        $stderr.puts "REJECTED: #{rejected}"

        rejected
       */
    printCSOCDequeDeque(flattened, "REJECTED: ");
       
   

    
    // Convert back to the data type the rest of the code expects.
    for (CSOCDequeDeque::iterator resultIterator = flattened.begin(), resultEndIterator = flattened.end();
           resultIterator != resultEndIterator; ++resultIterator) {
    	CSOCDeque& deque = *resultIterator;
      out.push_back(CSOCToComplexSelector(deque, ctx));
    }

    
    cerr << "END SUBWEAVE" << endl;
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
