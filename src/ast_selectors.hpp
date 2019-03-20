#ifndef SASS_AST_SEL_H
#define SASS_AST_SEL_H

// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"

#include <set>
#include <deque>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include "sass/base.h"
#include "ast_fwd_decl.hpp"

#include "util.hpp"
#include "units.hpp"
#include "context.hpp"
#include "position.hpp"
#include "constants.hpp"
#include "operation.hpp"
#include "position.hpp"
#include "inspect.hpp"
#include "source_map.hpp"
#include "environment.hpp"
#include "error_handling.hpp"
#include "ast_def_macros.hpp"
#include "ast_fwd_decl.hpp"
#include "source_map.hpp"
#include "fn_utils.hpp"

#include "sass.h"

namespace Sass {

  /////////////////////////////////////////
  // Abstract base class for CSS selectors.
  /////////////////////////////////////////
  class Selector : public Expression {
    // line break before list separator
    ADD_PROPERTY(bool, has_line_feed)
    // line break after list separator
    ADD_PROPERTY(bool, has_line_break)
    // maybe we have optional flag
    ADD_PROPERTY(bool, is_optional)
    // must not be a reference counted object
    // otherwise we create circular references
    ADD_PROPERTY(Media_Block*, media_block)
  protected:
    mutable size_t hash_;
  public:
    Selector(ParserState pstate);
    virtual ~Selector() = 0;
    size_t hash() const override = 0;
    virtual unsigned long specificity() const = 0;
    virtual int unification_order() const = 0;
    virtual void set_media_block(Media_Block* mb);
    virtual bool has_parent_ref() const;
    virtual bool has_real_parent_ref() const;
    // dispatch to correct handlers
    virtual bool operator<(const Selector& rhs) const = 0;
    virtual bool operator==(const Selector& rhs) const = 0;
    bool operator>(const Selector& rhs) const { return rhs < *this; };
    bool operator!=(const Selector& rhs) const { return !(rhs == *this); };
    ATTACH_VIRTUAL_AST_OPERATIONS(Selector);
  };
  inline Selector::~Selector() { }

  /////////////////////////////////////////////////////////////////////////
  // Interpolated selectors -- the interpolated String will be expanded and
  // re-parsed into a normal selector class.
  /////////////////////////////////////////////////////////////////////////
  class Selector_Schema final : public AST_Node {
    ADD_PROPERTY(String_Obj, contents)
    ADD_PROPERTY(bool, connect_parent);
    // must not be a reference counted object
    // otherwise we create circular references
    ADD_PROPERTY(Media_Block*, media_block)
    // store computed hash
    mutable size_t hash_;
  public:
    Selector_Schema(ParserState pstate, String_Obj c);
    bool has_parent_ref() const;
    bool has_real_parent_ref() const;
    bool operator<(const Selector& rhs) const;
    bool operator==(const Selector& rhs) const;
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
  class Simple_Selector : public Selector {
  public:
    enum Simple_Type {
      ID_SEL,
      TYPE_SEL,
      CLASS_SEL,
      PSEUDO_SEL,
      PARENT_SEL,
      WRAPPED_SEL,
      ATTRIBUTE_SEL,
      PLACEHOLDER_SEL,
    };
  public:
    HASH_CONSTREF(std::string, ns)
    HASH_CONSTREF(std::string, name)
    ADD_PROPERTY(Simple_Type, simple_type)
    HASH_PROPERTY(bool, has_ns)
  public:
    Simple_Selector(ParserState pstate, std::string n = "");
    virtual std::string ns_name() const;
    size_t hash() const override;
    bool empty() const;
    // namespace compare functions
    bool is_ns_eq(const Simple_Selector& r) const;
    // namespace query functions
    bool is_universal_ns() const;
    bool is_empty_ns() const;
    bool has_empty_ns() const;
    bool has_qualified_ns() const;
    // name query functions
    bool is_universal() const;
    virtual bool has_placeholder();

    virtual ~Simple_Selector() = 0;
    virtual Compound_Selector* unify_with(Compound_Selector*);

    virtual bool has_parent_ref() const override;
    virtual bool has_real_parent_ref() const override;
    virtual bool is_pseudo_element() const;
    virtual bool is_superselector_of(const Compound_Selector* sub) const;

    bool operator<(const Selector& rhs) const final override;
    bool operator==(const Selector& rhs) const final override;
    virtual bool operator<(const Selector_List& rhs) const;
    virtual bool operator==(const Selector_List& rhs) const;
    virtual bool operator<(const Complex_Selector& rhs) const;
    virtual bool operator==(const Complex_Selector& rhs) const;
    virtual bool operator<(const Compound_Selector& rhs) const;
    virtual bool operator==(const Compound_Selector& rhs) const;
    virtual bool operator<(const Simple_Selector& rhs) const;
    virtual bool operator==(const Simple_Selector& rhs) const;

    ATTACH_VIRTUAL_AST_OPERATIONS(Simple_Selector);
    ATTACH_CRTP_PERFORM_METHODS();

  };
  inline Simple_Selector::~Simple_Selector() { }

  //////////////////////////////////
  // The Parent Selector Expression.
  //////////////////////////////////
  class Parent_Selector final : public Simple_Selector {
    // a real parent selector is given by the user
    // others are added implicitly to connect the
    // selector scopes automatically when rendered
    // a Parent_Reference is never seen in selectors
    // and is only used in values (e.g. `prop: #{&};`)
    ADD_PROPERTY(bool, real)
  public:
    Parent_Selector(ParserState pstate, bool r = true);

    virtual bool has_parent_ref() const override;
    virtual bool has_real_parent_ref() const override;

    virtual unsigned long specificity() const override;
    int unification_order() const override
    {
      throw std::runtime_error("unification_order for Parent_Selector is undefined");
    }
    std::string type() const override { return "selector"; }
    static std::string type_name() { return "selector"; }
    bool operator<(const Simple_Selector& rhs) const final override;
    bool operator==(const Simple_Selector& rhs) const final override;
    bool operator<(const Parent_Selector& rhs) const;
    bool operator==(const Parent_Selector& rhs) const;
    ATTACH_AST_OPERATIONS(Parent_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };


  /////////////////////////////////////////////////////////////////////////
  // Placeholder selectors (e.g., "%foo") for use in extend-only selectors.
  /////////////////////////////////////////////////////////////////////////
  class Placeholder_Selector final : public Simple_Selector {
  public:
    Placeholder_Selector(ParserState pstate, std::string n);

    int unification_order() const override
    {
      return Constants::UnificationOrder_Placeholder;
    }
    virtual ~Placeholder_Selector() {};
    virtual unsigned long specificity() const override;
    virtual bool has_placeholder() override;
    bool operator<(const Simple_Selector& rhs) const override;
    bool operator==(const Simple_Selector& rhs) const override;
    bool operator<(const Placeholder_Selector& rhs) const;
    bool operator==(const Placeholder_Selector& rhs) const;
    ATTACH_AST_OPERATIONS(Placeholder_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////////////////////////
  // Type selectors (and the universal selector) -- e.g., div, span, *.
  /////////////////////////////////////////////////////////////////////
  class Type_Selector final : public Simple_Selector {
  public:
    Type_Selector(ParserState pstate, std::string n);
    virtual unsigned long specificity() const override;
    int unification_order() const override
    {
      return Constants::UnificationOrder_Element;
    }
    Simple_Selector* unify_with(Simple_Selector*);
    Compound_Selector* unify_with(Compound_Selector*) override;
    bool operator<(const Simple_Selector& rhs) const final override;
    bool operator==(const Simple_Selector& rhs) const final override;
    bool operator<(const Type_Selector& rhs) const;
    bool operator==(const Type_Selector& rhs) const;
    ATTACH_AST_OPERATIONS(Type_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////
  // Class selectors  -- i.e., .foo.
  ////////////////////////////////////////////////
  class Class_Selector final : public Simple_Selector {
  public:
    Class_Selector(ParserState pstate, std::string n);
    virtual unsigned long specificity() const override;
    int unification_order() const override
    {
      return Constants::UnificationOrder_Class;
    }
    Compound_Selector* unify_with(Compound_Selector*) override;
    bool operator<(const Simple_Selector& rhs) const final override;
    bool operator==(const Simple_Selector& rhs) const final override;
    bool operator<(const Class_Selector& rhs) const;
    bool operator==(const Class_Selector& rhs) const;
    ATTACH_AST_OPERATIONS(Class_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////
  // ID selectors -- i.e., #foo.
  ////////////////////////////////////////////////
  class Id_Selector final : public Simple_Selector {
  public:
    Id_Selector(ParserState pstate, std::string n);
    virtual unsigned long specificity() const override;
    int unification_order() const override
    {
      return Constants::UnificationOrder_Id;
    }
    Compound_Selector* unify_with(Compound_Selector*) override;
    bool operator<(const Simple_Selector& rhs) const final override;
    bool operator==(const Simple_Selector& rhs) const final override;
    bool operator<(const Id_Selector& rhs) const;
    bool operator==(const Id_Selector& rhs) const;
    ATTACH_AST_OPERATIONS(Id_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////////////////////
  // Attribute selectors -- e.g., [src*=".jpg"], etc.
  ///////////////////////////////////////////////////
  class Attribute_Selector final : public Simple_Selector {
    ADD_CONSTREF(std::string, matcher)
    // this cannot be changed to obj atm!!!!!!????!!!!!!!
    ADD_PROPERTY(String_Obj, value) // might be interpolated
    ADD_PROPERTY(char, modifier);
  public:
    Attribute_Selector(ParserState pstate, std::string n, std::string m, String_Obj v, char o = 0);
    size_t hash() const override;
    virtual unsigned long specificity() const override;
    int unification_order() const override
    {
      return Constants::UnificationOrder_Attribute;
    }
    bool operator<(const Simple_Selector& rhs) const final override;
    bool operator==(const Simple_Selector& rhs) const final override;
    bool operator<(const Attribute_Selector& rhs) const;
    bool operator==(const Attribute_Selector& rhs) const;
    ATTACH_AST_OPERATIONS(Attribute_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  //////////////////////////////////////////////////////////////////
  // Pseudo selectors -- e.g., :first-child, :nth-of-type(...), etc.
  //////////////////////////////////////////////////////////////////
  /* '::' starts a pseudo-element, ':' a pseudo-class */
  /* Except :first-line, :first-letter, :before and :after */
  /* Note that pseudo-elements are restricted to one per selector */
  /* and occur only in the last simple_selector_sequence. */
  inline bool is_pseudo_class_element(const std::string& name)
  {
    return name == ":before"       ||
           name == ":after"        ||
           name == ":first-line"   ||
           name == ":first-letter";
  }

  // Pseudo Selector cannot have any namespace?
  class Pseudo_Selector final : public Simple_Selector {
    ADD_PROPERTY(String_Obj, expression)
  public:
    Pseudo_Selector(ParserState pstate, std::string n, String_Obj expr = {});
    virtual bool is_pseudo_element() const override;
    size_t hash() const override;
    virtual unsigned long specificity() const override;
    int unification_order() const override
    {
      if (is_pseudo_element())
        return Constants::UnificationOrder_PseudoElement;
      return Constants::UnificationOrder_PseudoClass;
    }
    bool operator<(const Simple_Selector& rhs) const final override;
    bool operator==(const Simple_Selector& rhs) const final override;
    bool operator<(const Pseudo_Selector& rhs) const;
    bool operator==(const Pseudo_Selector& rhs) const;
    Compound_Selector* unify_with(Compound_Selector*) override;
    ATTACH_AST_OPERATIONS(Pseudo_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////
  // Wrapped selector -- pseudo selector that takes a list of selectors as argument(s) e.g., :not(:first-of-type), :-moz-any(ol p.blah, ul, menu, dir)
  /////////////////////////////////////////////////
  class Wrapped_Selector final : public Simple_Selector {
    ADD_PROPERTY(Selector_List_Obj, selector)
  public:
    Wrapped_Selector(ParserState pstate, std::string n, Selector_List_Obj sel);
    using Simple_Selector::is_superselector_of;
    bool is_superselector_of(const Wrapped_Selector* sub) const;
    // Selectors inside the negation pseudo-class are counted like any
    // other, but the negation itself does not count as a pseudo-class.
    size_t hash() const override;
    bool has_parent_ref() const override;
    bool has_real_parent_ref() const override;
    unsigned long specificity() const override;
    int unification_order() const override
    {
      return Constants::UnificationOrder_Wrapped;
    }
    bool find ( bool (*f)(AST_Node_Obj) ) override;
    bool operator<(const Simple_Selector& rhs) const final override;
    bool operator==(const Simple_Selector& rhs) const final override;
    bool operator<(const Wrapped_Selector& rhs) const;
    bool operator==(const Wrapped_Selector& rhs) const;
    void cloneChildren() override;
    ATTACH_AST_OPERATIONS(Wrapped_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////////
  // Simple selector sequences. Maintains flags indicating whether it contains
  // any parent references or placeholders, to simplify expansion.
  ////////////////////////////////////////////////////////////////////////////
  class Compound_Selector final : public Selector, public Vectorized<Simple_Selector_Obj> {
  private:
    ComplexSelectorSet sources_;
    ADD_PROPERTY(bool, extended);
    ADD_PROPERTY(bool, has_parent_reference);
  protected:
    void adjust_after_pushing(Simple_Selector_Obj s) override
    {
      // if (s->has_reference())   has_reference(true);
      // if (s->has_placeholder()) has_placeholder(true);
    }
  public:
    Compound_Selector(ParserState pstate, size_t s = 0);
    bool contains_placeholder();
    void append(Simple_Selector_Obj element) override;
    bool is_universal() const;
    Complex_Selector_Obj to_complex();
    Compound_Selector* unify_with(Compound_Selector* rhs);
    // virtual Placeholder_Selector* find_placeholder();
    bool has_parent_ref() const override;
    bool has_real_parent_ref() const override;
    Simple_Selector* base() const;
    bool is_superselector_of(const Compound_Selector* sub, std::string wrapped = "") const;
    bool is_superselector_of(const Complex_Selector* sub, std::string wrapped = "") const;
    bool is_superselector_of(const Selector_List* sub, std::string wrapped = "") const;
    size_t hash() const override;
    virtual unsigned long specificity() const override;
    virtual bool has_placeholder();
    bool is_empty_reference();
    int unification_order() const override
    {
      throw std::runtime_error("unification_order for Compound_Selector is undefined");
    }
    bool find ( bool (*f)(AST_Node_Obj) ) override;

    bool operator<(const Selector& rhs) const override;
    bool operator==(const Selector& rhs) const override;
    bool operator<(const Selector_List& rhs) const;
    bool operator==(const Selector_List& rhs) const;
    bool operator<(const Complex_Selector& rhs) const;
    bool operator==(const Complex_Selector& rhs) const;
    bool operator<(const Compound_Selector& rhs) const;
    bool operator==(const Compound_Selector& rhs) const;
    bool operator<(const Simple_Selector& rhs) const;
    bool operator==(const Simple_Selector& rhs) const;

    ComplexSelectorSet& sources() { return sources_; }
    void clearSources() { sources_.clear(); }
    void mergeSources(ComplexSelectorSet& sources);

    Compound_Selector* minus(Compound_Selector* rhs);
    void cloneChildren() override;
    ATTACH_AST_OPERATIONS(Compound_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ////////////////////////////////////////////////////////////////////////////
  // General selectors -- i.e., simple sequences combined with one of the four
  // CSS selector combinators (">", "+", "~", and whitespace). Essentially a
  // linked list.
  ////////////////////////////////////////////////////////////////////////////
  class Complex_Selector final : public Selector {
  public:
    enum Combinator { ANCESTOR_OF, PARENT_OF, PRECEDES, ADJACENT_TO, REFERENCE };
  private:
    HASH_CONSTREF(Combinator, combinator)
    HASH_PROPERTY(Compound_Selector_Obj, head)
    HASH_PROPERTY(Complex_Selector_Obj, tail)
    HASH_PROPERTY(String_Obj, reference);
  public:
    bool contains_placeholder() {
      if (head() && head()->contains_placeholder()) return true;
      if (tail() && tail()->contains_placeholder()) return true;
      return false;
    };
    Complex_Selector(ParserState pstate,
                     Combinator c = ANCESTOR_OF,
                     Compound_Selector_Obj h = {},
                     Complex_Selector_Obj t = {},
                     String_Obj r = {});

    bool empty() const;

    bool has_parent_ref() const override;
    bool has_real_parent_ref() const override;
    Complex_Selector_Obj skip_empty_reference();

    // can still have a tail
    bool is_empty_ancestor() const;

    Selector_List* tails(Selector_List* tails);

    // front returns the first real tail
    // skips over parent and empty ones
    const Complex_Selector* first() const;
    Complex_Selector* mutable_first();

    // last returns the last real tail
    const Complex_Selector* last() const;
    Complex_Selector* mutable_last();

    size_t length() const;
    Selector_List* resolve_parent_refs(SelectorStack& pstack, Backtraces& traces, bool implicit_parent = true);
    bool is_superselector_of(const Compound_Selector* sub, std::string wrapping = "") const;
    bool is_superselector_of(const Complex_Selector* sub, std::string wrapping = "") const;
    bool is_superselector_of(const Selector_List* sub, std::string wrapping = "") const;
    Selector_List* unify_with(Complex_Selector* rhs);
    Combinator clear_innermost();
    void append(Complex_Selector_Obj, Backtraces& traces);
    void set_innermost(Complex_Selector_Obj, Combinator);

    size_t hash() const override;
    virtual unsigned long specificity() const override;
    virtual void set_media_block(Media_Block* mb) override;
    virtual bool has_placeholder();
    int unification_order() const override
    {
      throw std::runtime_error("unification_order for Complex_Selector is undefined");
    }
    bool find ( bool (*f)(AST_Node_Obj) ) override;

    bool operator<(const Selector& rhs) const override;
    bool operator==(const Selector& rhs) const override;
    bool operator<(const Selector_List& rhs) const;
    bool operator==(const Selector_List& rhs) const;
    bool operator<(const Complex_Selector& rhs) const;
    bool operator==(const Complex_Selector& rhs) const;
    bool operator<(const Compound_Selector& rhs) const;
    bool operator==(const Compound_Selector& rhs) const;
    bool operator<(const Simple_Selector& rhs) const;
    bool operator==(const Simple_Selector& rhs) const;

    const ComplexSelectorSet sources();
    void addSources(ComplexSelectorSet& sources);
    void clearSources();

    void cloneChildren() override;
    ATTACH_AST_OPERATIONS(Complex_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  ///////////////////////////////////
  // Comma-separated selector groups.
  ///////////////////////////////////
  class Selector_List final : public Selector, public Vectorized<Complex_Selector_Obj> {
    ADD_PROPERTY(Selector_Schema_Obj, schema)
    ADD_CONSTREF(std::vector<std::string>, wspace)
  protected:
    void adjust_after_pushing(Complex_Selector_Obj c) override;
  public:
    Selector_List(ParserState pstate, size_t s = 0);
    std::string type() const override { return "list"; }
    // remove parent selector references
    // basically unwraps parsed selectors
    bool has_parent_ref() const override;
    bool has_real_parent_ref() const override;
    void remove_parent_selectors();
    Selector_List* resolve_parent_refs(SelectorStack& pstack, Backtraces& traces, bool implicit_parent = true);
    bool is_superselector_of(const Compound_Selector* sub, std::string wrapping = "") const;
    bool is_superselector_of(const Complex_Selector* sub, std::string wrapping = "") const;
    bool is_superselector_of(const Selector_List* sub, std::string wrapping = "") const;
    Selector_List* unify_with(Selector_List*);
    void populate_extends(Selector_List_Obj, Subset_Map&);
    Selector_List_Obj eval(Eval& eval);

    size_t hash() const override;
    virtual unsigned long specificity() const override;
    virtual void set_media_block(Media_Block* mb) override;
    virtual bool has_placeholder();
    int unification_order() const override
    {
      throw std::runtime_error("unification_order for Selector_List is undefined");
    }
    bool find ( bool (*f)(AST_Node_Obj) ) override;
    bool operator<(const Selector& rhs) const override;
    bool operator==(const Selector& rhs) const override;
    bool operator<(const Selector_List& rhs) const;
    bool operator==(const Selector_List& rhs) const;
    bool operator<(const Complex_Selector& rhs) const;
    bool operator==(const Complex_Selector& rhs) const;
    bool operator<(const Compound_Selector& rhs) const;
    bool operator==(const Compound_Selector& rhs) const;
    bool operator<(const Simple_Selector& rhs) const;
    bool operator==(const Simple_Selector& rhs) const;
    // Selector Lists can be compared to comma lists
    bool operator<(const Expression& rhs) const override;
    bool operator==(const Expression& rhs) const override;
    void cloneChildren() override;
    ATTACH_AST_OPERATIONS(Selector_List)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  // compare function for sorting and probably other other uses
  struct cmp_complex_selector { inline bool operator() (const Complex_Selector_Obj l, const Complex_Selector_Obj r) { return (*l < *r); } };
  struct cmp_compound_selector { inline bool operator() (const Compound_Selector_Obj l, const Compound_Selector_Obj r) { return (*l < *r); } };
  struct cmp_simple_selector { inline bool operator() (const Simple_Selector_Obj l, const Simple_Selector_Obj r) { return (*l < *r); } };

}

#endif
