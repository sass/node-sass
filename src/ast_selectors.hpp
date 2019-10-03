#ifndef SASS_AST_SEL_H
#define SASS_AST_SEL_H

// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"
#include "ast.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Some helper functions
  /////////////////////////////////////////////////////////////////////////

  bool compoundIsSuperselector(
    const CompoundSelectorObj& compound1,
    const CompoundSelectorObj& compound2,
    const std::vector<SelectorComponentObj>& parents);

  bool complexIsParentSuperselector(
    const std::vector<SelectorComponentObj>& complex1,
    const std::vector<SelectorComponentObj>& complex2);

    std::vector<std::vector<SelectorComponentObj>> weave(
    const std::vector<std::vector<SelectorComponentObj>>& complexes);

  std::vector<std::vector<SelectorComponentObj>> weaveParents(
    std::vector<SelectorComponentObj> parents1,
    std::vector<SelectorComponentObj> parents2);

  std::vector<SimpleSelectorObj> unifyCompound(
    const std::vector<SimpleSelectorObj>& compound1,
    const std::vector<SimpleSelectorObj>& compound2);

  std::vector<std::vector<SelectorComponentObj>> unifyComplex(
    const std::vector<std::vector<SelectorComponentObj>>& complexes);

  /////////////////////////////////////////
  // Abstract base class for CSS selectors.
  /////////////////////////////////////////
  class Selector : public Expression {
  protected:
    mutable size_t hash_;
  public:
    Selector(ParserState pstate);
    virtual ~Selector() = 0;
    size_t hash() const override = 0;
    virtual bool has_real_parent_ref() const;
    // you should reset this to null on containers
    virtual unsigned long specificity() const = 0;
    // by default we return the regular specificity
    // you must override this for all containers
    virtual size_t maxSpecificity() const { return specificity(); }
    virtual size_t minSpecificity() const { return specificity(); }
    // dispatch to correct handlers
    ATTACH_VIRTUAL_CMP_OPERATIONS(Selector)
    ATTACH_VIRTUAL_AST_OPERATIONS(Selector)
  };
  inline Selector::~Selector() { }

  /////////////////////////////////////////////////////////////////////////
  // Interpolated selectors -- the interpolated String will be expanded and
  // re-parsed into a normal selector class.
  /////////////////////////////////////////////////////////////////////////
  class Selector_Schema final : public AST_Node {
    ADD_PROPERTY(String_Schema_Obj, contents)
    ADD_PROPERTY(bool, connect_parent);
    // store computed hash
    mutable size_t hash_;
  public:
    Selector_Schema(ParserState pstate, String_Obj c);

    bool has_real_parent_ref() const;
    // selector schema is not yet a final selector, so we do not
    // have a specificity for it yet. We need to
    virtual unsigned long specificity() const;
    size_t hash() const override;
    ATTACH_AST_OPERATIONS(Selector_Schema)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////
  // Abstract base class for simple selectors.
  ////////////////////////////////////////////
  class SimpleSelector : public Selector {
  public:
    enum Simple_Type {
      ID_SEL,
      TYPE_SEL,
      CLASS_SEL,
      PSEUDO_SEL,
      ATTRIBUTE_SEL,
      PLACEHOLDER_SEL,
    };
  public:
    HASH_CONSTREF(std::string, ns)
    HASH_CONSTREF(std::string, name)
    ADD_PROPERTY(Simple_Type, simple_type)
    HASH_PROPERTY(bool, has_ns)
  public:
    SimpleSelector(ParserState pstate, std::string n = "");
    virtual std::string ns_name() const;
    size_t hash() const override;
    virtual bool empty() const;
    // namespace compare functions
    bool is_ns_eq(const SimpleSelector& r) const;
    // namespace query functions
    bool is_universal_ns() const;
    bool is_empty_ns() const;
    bool has_empty_ns() const;
    bool has_qualified_ns() const;
    // name query functions
    bool is_universal() const;
    virtual bool has_placeholder();

    virtual ~SimpleSelector() = 0;
    virtual CompoundSelector* unifyWith(CompoundSelector*);

    /* helper function for syntax sugar */
    virtual Id_Selector* getIdSelector() { return NULL; }
    virtual Type_Selector* getTypeSelector() { return NULL; }
    virtual Pseudo_Selector* getPseudoSelector() { return NULL; }

    ComplexSelectorObj wrapInComplex();
    CompoundSelectorObj wrapInCompound();

    virtual bool isInvisible() const { return false; }
    virtual bool is_pseudo_element() const;
    virtual bool has_real_parent_ref() const override;

    bool operator==(const Selector& rhs) const final override;

    virtual bool operator==(const SelectorList& rhs) const;
    virtual bool operator==(const ComplexSelector& rhs) const;
    virtual bool operator==(const CompoundSelector& rhs) const;

    ATTACH_VIRTUAL_CMP_OPERATIONS(SimpleSelector);
    ATTACH_VIRTUAL_AST_OPERATIONS(SimpleSelector);
    ATTACH_CRTP_PERFORM_METHODS();

  };
  inline SimpleSelector::~SimpleSelector() { }

  /////////////////////////////////////////////////////////////////////////
  // Placeholder selectors (e.g., "%foo") for use in extend-only selectors.
  /////////////////////////////////////////////////////////////////////////
  class Placeholder_Selector final : public SimpleSelector {
  public:
    Placeholder_Selector(ParserState pstate, std::string n);
    bool isInvisible() const override { return true; }
    virtual unsigned long specificity() const override;
    virtual bool has_placeholder() override;
    bool operator==(const SimpleSelector& rhs) const override;
    ATTACH_CMP_OPERATIONS(Placeholder_Selector)
    ATTACH_AST_OPERATIONS(Placeholder_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////////////////////////
  // Type selectors (and the universal selector) -- e.g., div, span, *.
  /////////////////////////////////////////////////////////////////////
  class Type_Selector final : public SimpleSelector {
  public:
    Type_Selector(ParserState pstate, std::string n);
    virtual unsigned long specificity() const override;
    SimpleSelector* unifyWith(const SimpleSelector*);
    CompoundSelector* unifyWith(CompoundSelector*) override;
    Type_Selector* getTypeSelector() override { return this; }
    bool operator==(const SimpleSelector& rhs) const final override;
    ATTACH_CMP_OPERATIONS(Type_Selector)
    ATTACH_AST_OPERATIONS(Type_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////
  // Class selectors  -- i.e., .foo.
  ////////////////////////////////////////////////
  class Class_Selector final : public SimpleSelector {
  public:
    Class_Selector(ParserState pstate, std::string n);
    virtual unsigned long specificity() const override;
    bool operator==(const SimpleSelector& rhs) const final override;
    ATTACH_CMP_OPERATIONS(Class_Selector)
    ATTACH_AST_OPERATIONS(Class_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////
  // ID selectors -- i.e., #foo.
  ////////////////////////////////////////////////
  class Id_Selector final : public SimpleSelector {
  public:
    Id_Selector(ParserState pstate, std::string n);
    virtual unsigned long specificity() const override;
    CompoundSelector* unifyWith(CompoundSelector*) override;
    Id_Selector* getIdSelector() final override { return this; }
    bool operator==(const SimpleSelector& rhs) const final override;
    ATTACH_CMP_OPERATIONS(Id_Selector)
    ATTACH_AST_OPERATIONS(Id_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////////////
  // Attribute selectors -- e.g., [src*=".jpg"], etc.
  ///////////////////////////////////////////////////
  class Attribute_Selector final : public SimpleSelector {
    ADD_CONSTREF(std::string, matcher)
    // this cannot be changed to obj atm!!!!!!????!!!!!!!
    ADD_PROPERTY(String_Obj, value) // might be interpolated
    ADD_PROPERTY(char, modifier);
  public:
    Attribute_Selector(ParserState pstate, std::string n, std::string m, String_Obj v, char o = 0);
    size_t hash() const override;
    virtual unsigned long specificity() const override;
    bool operator==(const SimpleSelector& rhs) const final override;
    ATTACH_CMP_OPERATIONS(Attribute_Selector)
    ATTACH_AST_OPERATIONS(Attribute_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////////////////////////////////////////
  // Pseudo selectors -- e.g., :first-child, :nth-of-type(...), etc.
  //////////////////////////////////////////////////////////////////
  // Pseudo Selector cannot have any namespace?
  class Pseudo_Selector final : public SimpleSelector {
    ADD_PROPERTY(std::string, normalized)
    ADD_PROPERTY(String_Obj, argument)
    ADD_PROPERTY(SelectorListObj, selector)
    ADD_PROPERTY(bool, isSyntacticClass)
    ADD_PROPERTY(bool, isClass)
  public:
    Pseudo_Selector(ParserState pstate, std::string n, bool element = false);
    virtual bool is_pseudo_element() const override;
    size_t hash() const override;

    bool empty() const override;

    bool has_real_parent_ref() const override;

    // Whether this is a pseudo-element selector.
    // This is `true` if and only if [isClass] is `false`.
    bool isElement() const { return !isClass(); }

    // Whether this is syntactically a pseudo-element selector.
    // This is `true` if and only if [isSyntacticClass] is `false`.
    bool isSyntacticElement() const { return !isSyntacticClass(); }

    virtual unsigned long specificity() const override;
    Pseudo_Selector_Obj withSelector(SelectorListObj selector);

    CompoundSelector* unifyWith(CompoundSelector*) override;
    Pseudo_Selector* getPseudoSelector() final override { return this; }
    bool operator==(const SimpleSelector& rhs) const final override;
    ATTACH_CMP_OPERATIONS(Pseudo_Selector)
    ATTACH_AST_OPERATIONS(Pseudo_Selector)
    void cloneChildren() override;
    ATTACH_CRTP_PERFORM_METHODS()
  };


  ////////////////////////////////////////////////////////////////////////////
  // Complex Selectors are the most important class of selectors.
  // A Selector List consists of Complex Selectors (separated by comma)
  // Complex Selectors are itself a list of Compounds and Combinators
  // Between each item there is an implicit ancestor of combinator
  ////////////////////////////////////////////////////////////////////////////
  class ComplexSelector final : public Selector, public Vectorized<SelectorComponentObj> {
    ADD_PROPERTY(bool, chroots)
    // line break before list separator
    ADD_PROPERTY(bool, hasPreLineFeed)
  public:
    ComplexSelector(ParserState pstate);

    // Returns true if the first components
    // is a compound selector and fullfills
    // a few other criteria.
    bool isInvisible() const;

    size_t hash() const override;
    void cloneChildren() override;
    bool has_placeholder() const;
    bool has_real_parent_ref() const override;

    SelectorList* resolve_parent_refs(SelectorStack pstack, Backtraces& traces, bool implicit_parent = true);
    virtual unsigned long specificity() const override;

    SelectorList* unifyWith(ComplexSelector* rhs);

    bool isSuperselectorOf(const ComplexSelector* sub) const;

    SelectorListObj wrapInList();

    size_t maxSpecificity() const override;
    size_t minSpecificity() const override;

    bool operator==(const Selector& rhs) const override;
    bool operator==(const SelectorList& rhs) const;
    bool operator==(const CompoundSelector& rhs) const;
    bool operator==(const SimpleSelector& rhs) const;

    ATTACH_CMP_OPERATIONS(ComplexSelector)
    ATTACH_AST_OPERATIONS(ComplexSelector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////////
  // Base class for complex selector components
  ////////////////////////////////////////////////////////////////////////////
  class SelectorComponent : public Selector {
    // line break after list separator
    ADD_PROPERTY(bool, hasPostLineBreak)
  public:
    SelectorComponent(ParserState pstate, bool postLineBreak = false);
    size_t hash() const override = 0;
    void cloneChildren() override;


    // By default we consider instances not empty
    virtual bool empty() const { return false; }

    virtual bool has_placeholder() const = 0;
    bool has_real_parent_ref() const override = 0;

    ComplexSelector* wrapInComplex();

    size_t maxSpecificity() const override { return 0; }
    size_t minSpecificity() const override { return 0; }

    virtual bool isCompound() const { return false; };
    virtual bool isCombinator() const { return false; };

    /* helper function for syntax sugar */
    virtual CompoundSelector* getCompound() { return NULL; }
    virtual SelectorCombinator* getCombinator() { return NULL; }
    virtual const CompoundSelector* getCompound() const { return NULL; }
    virtual const SelectorCombinator* getCombinator() const { return NULL; }

    virtual unsigned long specificity() const override;
    bool operator==(const Selector& rhs) const override = 0;
    ATTACH_VIRTUAL_CMP_OPERATIONS(SelectorComponent);
    ATTACH_VIRTUAL_AST_OPERATIONS(SelectorComponent);
  };

  ////////////////////////////////////////////////////////////////////////////
  // A specific combinator between compound selectors
  ////////////////////////////////////////////////////////////////////////////
  class SelectorCombinator final : public SelectorComponent {
  public:

    // Enumerate all possible selector combinators. There is some
    // discrepancy with dart-sass. Opted to name them as in CSS33
    enum Combinator { CHILD /* > */, GENERAL /* ~ */, ADJACENT /* + */};

  private:

    // Store the type of this combinator
    HASH_CONSTREF(Combinator, combinator)

  public:
    SelectorCombinator(ParserState pstate, Combinator combinator, bool postLineBreak = false);

    bool has_real_parent_ref() const override { return false; }
    bool has_placeholder() const override { return false; }

    /* helper function for syntax sugar */
    SelectorCombinator* getCombinator() final override { return this; }
    const SelectorCombinator* getCombinator() const final override { return this; }

    // Query type of combinator
    bool isCombinator() const override { return true; };

    // Matches the right-hand selector if it's a direct child of the left-
    // hand selector in the DOM tree. Dart-sass also calls this `child`
    // https://developer.mozilla.org/en-US/docs/Web/CSS/Child_combinator
    bool isChildCombinator() const { return combinator_ == CHILD; } // >

    // Matches the right-hand selector if it comes after the left-hand
    // selector in the DOM tree. Dart-sass class this `followingSibling`
    // https://developer.mozilla.org/en-US/docs/Web/CSS/General_sibling_combinator
    bool isGeneralCombinator() const { return combinator_ == GENERAL; } // ~

    // Matches the right-hand selector if it's immediately adjacent to the
    // left-hand selector in the DOM tree. Dart-sass calls this `nextSibling`
    // https://developer.mozilla.org/en-US/docs/Web/CSS/Adjacent_sibling_combinator
    bool isAdjacentCombinator() const { return combinator_ == ADJACENT; } // +

    size_t maxSpecificity() const override { return 0; }
    size_t minSpecificity() const override { return 0; }

    size_t hash() const override {
      return std::hash<int>()(combinator_);
    }
    void cloneChildren() override;
    virtual unsigned long specificity() const override;
    bool operator==(const Selector& rhs) const override;
    bool operator==(const SelectorComponent& rhs) const override;

    ATTACH_CMP_OPERATIONS(SelectorCombinator)
    ATTACH_AST_OPERATIONS(SelectorCombinator)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////////
  // A compound selector consists of multiple simple selectors
  ////////////////////////////////////////////////////////////////////////////
  class CompoundSelector final : public SelectorComponent, public Vectorized<SimpleSelectorObj> {
    ADD_PROPERTY(bool, hasRealParent)
    ADD_PROPERTY(bool, extended)
  public:
    CompoundSelector(ParserState pstate, bool postLineBreak = false);

    // Returns true if this compound selector
    // fullfills various criteria.
    bool isInvisible() const;

    bool empty() const override {
      return Vectorized::empty();
    }

    size_t hash() const override;
    CompoundSelector* unifyWith(CompoundSelector* rhs);

    /* helper function for syntax sugar */
    CompoundSelector* getCompound() final override { return this; }
    const CompoundSelector* getCompound() const final override { return this; }

    bool isSuperselectorOf(const CompoundSelector* sub, std::string wrapped = "") const;

    void cloneChildren() override;
    bool has_real_parent_ref() const override;
    bool has_placeholder() const override;
    std::vector<ComplexSelectorObj> resolve_parent_refs(SelectorStack pstack, Backtraces& traces, bool implicit_parent = true);

    virtual bool isCompound() const override { return true; };
    virtual unsigned long specificity() const override;

    size_t maxSpecificity() const override;
    size_t minSpecificity() const override;

    bool operator==(const Selector& rhs) const override;

    bool operator==(const SelectorComponent& rhs) const override;

    bool operator==(const SelectorList& rhs) const;
    bool operator==(const ComplexSelector& rhs) const;
    bool operator==(const SimpleSelector& rhs) const;

    ATTACH_CMP_OPERATIONS(CompoundSelector)
    ATTACH_AST_OPERATIONS(CompoundSelector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////
  // Comma-separated selector groups.
  ///////////////////////////////////
  class SelectorList final : public Selector, public Vectorized<ComplexSelectorObj> {
  private:
    // maybe we have optional flag
    // ToDo: should be at ExtendRule?
    ADD_PROPERTY(bool, is_optional)
  public:
    SelectorList(ParserState pstate, size_t s = 0);
    std::string type() const override { return "list"; }
    size_t hash() const override;

    SelectorList* unifyWith(SelectorList*);

    // Returns true if all complex selectors
    // can have real parents, meaning every
    // first component does allow for it
    bool isInvisible() const;

    void cloneChildren() override;
    bool has_real_parent_ref() const override;
    SelectorList* resolve_parent_refs(SelectorStack pstack, Backtraces& traces, bool implicit_parent = true);
    virtual unsigned long specificity() const override;

    bool isSuperselectorOf(const SelectorList* sub) const;

    size_t maxSpecificity() const override;
    size_t minSpecificity() const override;

    bool operator==(const Selector& rhs) const override;
    bool operator==(const ComplexSelector& rhs) const;
    bool operator==(const CompoundSelector& rhs) const;
    bool operator==(const SimpleSelector& rhs) const;
    // Selector Lists can be compared to comma lists
    bool operator==(const Expression& rhs) const override;

    ATTACH_CMP_OPERATIONS(SelectorList)
    ATTACH_AST_OPERATIONS(SelectorList)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////
  // The Sass `@extend` directive.
  ////////////////////////////////
  class ExtendRule final : public Statement {
    ADD_PROPERTY(bool, isOptional)
    // This should be a simple selector only!
    ADD_PROPERTY(SelectorListObj, selector)
    ADD_PROPERTY(Selector_Schema_Obj, schema)
  public:
    ExtendRule(ParserState pstate, SelectorListObj s);
    ExtendRule(ParserState pstate, Selector_Schema_Obj s);
    ATTACH_AST_OPERATIONS(ExtendRule)
    ATTACH_CRTP_PERFORM_METHODS()
  };

}

#endif
