#ifndef SASS_AST_SEL_H
#define SASS_AST_SEL_H

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
    // ADD_PROPERTY(bool, has_reference)
    // line break before list separator
    ADD_PROPERTY(bool, has_line_feed)
    // line break after list separator
    ADD_PROPERTY(bool, has_line_break)
    // maybe we have optional flag
    ADD_PROPERTY(bool, is_optional)
    // parent block pointers

    // must not be a reference counted object
    // otherwise we create circular references
    ADD_PROPERTY(Media_Block_Ptr, media_block)
  protected:
    mutable size_t hash_;
  public:
    Selector(ParserState pstate)
    : Expression(pstate),
      has_line_feed_(false),
      has_line_break_(false),
      is_optional_(false),
      media_block_(0),
      hash_(0)
    { concrete_type(SELECTOR); }
    Selector(const Selector* ptr)
    : Expression(ptr),
      // has_reference_(ptr->has_reference_),
      has_line_feed_(ptr->has_line_feed_),
      has_line_break_(ptr->has_line_break_),
      is_optional_(ptr->is_optional_),
      media_block_(ptr->media_block_),
      hash_(ptr->hash_)
    { concrete_type(SELECTOR); }
    virtual ~Selector() = 0;
    size_t hash() const override = 0;
    virtual unsigned long specificity() const = 0;
    virtual int unification_order() const = 0;
    virtual void set_media_block(Media_Block_Ptr mb) {
      media_block(mb);
    }
    virtual bool has_parent_ref() const {
      return false;
    }
    virtual bool has_real_parent_ref() const {
      return false;
    }
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
    ADD_PROPERTY(Media_Block_Ptr, media_block)
    // store computed hash
    mutable size_t hash_;
  public:
    Selector_Schema(ParserState pstate, String_Obj c)
    : AST_Node(pstate),
      contents_(c),
      connect_parent_(true),
      media_block_(NULL),
      hash_(0)
    { }
    Selector_Schema(const Selector_Schema* ptr)
    : AST_Node(ptr),
      contents_(ptr->contents_),
      connect_parent_(ptr->connect_parent_),
      media_block_(ptr->media_block_),
      hash_(ptr->hash_)
    { }
    bool has_parent_ref() const;
    bool has_real_parent_ref() const;
    bool operator<(const Selector& rhs) const;
    bool operator==(const Selector& rhs) const;
    // selector schema is not yet a final selector, so we do not
    // have a specificity for it yet. We need to
    unsigned long specificity() const { return 0; }
    size_t hash() const override {
      if (hash_ == 0) {
        hash_combine(hash_, contents_->hash());
      }
      return hash_;
    }
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
    ADD_CONSTREF(std::string, ns)
    ADD_CONSTREF(std::string, name)
    ADD_PROPERTY(Simple_Type, simple_type)
    ADD_PROPERTY(bool, has_ns)
  public:
    Simple_Selector(ParserState pstate, std::string n = "")
    : Selector(pstate), ns_(""), name_(n), has_ns_(false)
    {
      size_t pos = n.find('|');
      // found some namespace
      if (pos != std::string::npos) {
        has_ns_ = true;
        ns_ = n.substr(0, pos);
        name_ = n.substr(pos + 1);
      }
    }
    Simple_Selector(const Simple_Selector* ptr)
    : Selector(ptr),
      ns_(ptr->ns_),
      name_(ptr->name_),
      has_ns_(ptr->has_ns_)
    { }
    std::string ns_name() const
    {
      std::string name("");
      if (has_ns_)
        name += ns_ + "|";
      return name + name_;
    }
    virtual size_t hash() const override
    {
      if (hash_ == 0) {
        hash_combine(hash_, std::hash<int>()(SELECTOR));
        hash_combine(hash_, std::hash<int>()(simple_type()));
        hash_combine(hash_, std::hash<std::string>()(ns()));
        hash_combine(hash_, std::hash<std::string>()(name()));
      }
      return hash_;
    }
    bool empty() const {
      return ns().empty() && name().empty();
    }
    // namespace compare functions
    bool is_ns_eq(const Simple_Selector& r) const;
    // namespace query functions
    bool is_universal_ns() const
    {
      return has_ns_ && ns_ == "*";
    }
    bool has_universal_ns() const
    {
      return !has_ns_ || ns_ == "*";
    }
    bool is_empty_ns() const
    {
      return !has_ns_ || ns_ == "";
    }
    bool has_empty_ns() const
    {
      return has_ns_ && ns_ == "";
    }
    bool has_qualified_ns() const
    {
      return has_ns_ && ns_ != "" && ns_ != "*";
    }
    // name query functions
    virtual bool is_universal() const
    {
      return name_ == "*";
    }

    virtual bool has_placeholder() {
      return false;
    }

    virtual ~Simple_Selector() = 0;
    virtual Compound_Selector_Ptr unify_with(Compound_Selector_Ptr);
    virtual bool is_pseudo_element() const { return false; }

    virtual bool is_superselector_of(Compound_Selector_Ptr_Const sub) const { return false; }

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
  // parent selectors can occur in selectors but also
  // inside strings in declarations (Compound_Selector).
  // only one simple parent selector means the first case.
  class Parent_Selector final : public Simple_Selector {
    ADD_PROPERTY(bool, real)
  public:
    Parent_Selector(ParserState pstate, bool r = true)
    : Simple_Selector(pstate, "&"), real_(r)
    { simple_type(PARENT_SEL); }
    Parent_Selector(const Parent_Selector* ptr)
    : Simple_Selector(ptr), real_(ptr->real_)
    { simple_type(PARENT_SEL); }
    bool is_real_parent_ref() const { return real(); };
    bool has_parent_ref() const override { return true; };
    bool has_real_parent_ref() const override { return is_real_parent_ref(); };
    unsigned long specificity() const override
    {
      return 0;
    }
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
    Placeholder_Selector(ParserState pstate, std::string n)
    : Simple_Selector(pstate, n)
    { simple_type(PLACEHOLDER_SEL); }
    Placeholder_Selector(const Placeholder_Selector* ptr)
    : Simple_Selector(ptr)
    { simple_type(PLACEHOLDER_SEL); }
    unsigned long specificity() const override
    {
      return Constants::Specificity_Base;
    }
    int unification_order() const override
    {
      return Constants::UnificationOrder_Placeholder;
    }
    bool has_placeholder() override {
      return true;
    }
    virtual ~Placeholder_Selector() {};
    bool operator<(const Simple_Selector& rhs) const final override;
    bool operator==(const Simple_Selector& rhs) const final override;
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
    Type_Selector(ParserState pstate, std::string n)
    : Simple_Selector(pstate, n)
    { simple_type(TYPE_SEL); }
    Type_Selector(const Type_Selector* ptr)
    : Simple_Selector(ptr)
    { simple_type(TYPE_SEL); }
    unsigned long specificity() const override
    {
      if (name() == "*") return 0;
      else               return Constants::Specificity_Element;
    }
    int unification_order() const override
    {
      return Constants::UnificationOrder_Element;
    }
    Simple_Selector_Ptr unify_with(Simple_Selector_Ptr);
    Compound_Selector_Ptr unify_with(Compound_Selector_Ptr) override;
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
    Class_Selector(ParserState pstate, std::string n)
    : Simple_Selector(pstate, n)
    { simple_type(CLASS_SEL); }
    Class_Selector(const Class_Selector* ptr)
    : Simple_Selector(ptr)
    { simple_type(CLASS_SEL); }
    unsigned long specificity() const override
    {
      return Constants::Specificity_Class;
    }
    int unification_order() const override
    {
      return Constants::UnificationOrder_Class;
    }
    Compound_Selector_Ptr unify_with(Compound_Selector_Ptr) override;
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
    Id_Selector(ParserState pstate, std::string n)
    : Simple_Selector(pstate, n)
    { simple_type(ID_SEL); }
    Id_Selector(const Id_Selector* ptr)
    : Simple_Selector(ptr)
    { simple_type(ID_SEL); }
    unsigned long specificity() const override
    {
      return Constants::Specificity_ID;
    }
    int unification_order() const override
    {
      return Constants::UnificationOrder_Id;
    }
    Compound_Selector_Ptr unify_with(Compound_Selector_Ptr) override;
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
    Attribute_Selector(ParserState pstate, std::string n, std::string m, String_Obj v, char o = 0)
    : Simple_Selector(pstate, n), matcher_(m), value_(v), modifier_(o)
    { simple_type(ATTRIBUTE_SEL); }
    Attribute_Selector(const Attribute_Selector* ptr)
    : Simple_Selector(ptr),
      matcher_(ptr->matcher_),
      value_(ptr->value_),
      modifier_(ptr->modifier_)
    { simple_type(ATTRIBUTE_SEL); }
    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_combine(hash_, Simple_Selector::hash());
        hash_combine(hash_, std::hash<std::string>()(matcher()));
        if (value_) hash_combine(hash_, value_->hash());
      }
      return hash_;
    }
    unsigned long specificity() const override
    {
      return Constants::Specificity_Attr;
    }
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
    Pseudo_Selector(ParserState pstate, std::string n, String_Obj expr = {})
    : Simple_Selector(pstate, n), expression_(expr)
    { simple_type(PSEUDO_SEL); }
    Pseudo_Selector(const Pseudo_Selector* ptr)
    : Simple_Selector(ptr), expression_(ptr->expression_)
    { simple_type(PSEUDO_SEL); }

    // A pseudo-element is made of two colons (::) followed by the name.
    // The `::` notation is introduced by the current document in order to
    // establish a discrimination between pseudo-classes and pseudo-elements.
    // For compatibility with existing style sheets, user agents must also
    // accept the previous one-colon notation for pseudo-elements introduced
    // in CSS levels 1 and 2 (namely, :first-line, :first-letter, :before and
    // :after). This compatibility is not allowed for the new pseudo-elements
    // introduced in this specification.
    bool is_pseudo_element() const override
    {
      return (name_[0] == ':' && name_[1] == ':')
             || is_pseudo_class_element(name_);
    }
    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_combine(hash_, Simple_Selector::hash());
        if (expression_) hash_combine(hash_, expression_->hash());
      }
      return hash_;
    }
    unsigned long specificity() const override
    {
      if (is_pseudo_element())
        return Constants::Specificity_Element;
      return Constants::Specificity_Pseudo;
    }
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
    Compound_Selector_Ptr unify_with(Compound_Selector_Ptr) override;
    ATTACH_AST_OPERATIONS(Pseudo_Selector)
    ATTACH_CRTP_PERFORM_METHODS()
  };

  /////////////////////////////////////////////////
  // Wrapped selector -- pseudo selector that takes a list of selectors as argument(s) e.g., :not(:first-of-type), :-moz-any(ol p.blah, ul, menu, dir)
  /////////////////////////////////////////////////
  class Wrapped_Selector final : public Simple_Selector {
    ADD_PROPERTY(Selector_List_Obj, selector)
  public:
    Wrapped_Selector(ParserState pstate, std::string n, Selector_List_Obj sel)
    : Simple_Selector(pstate, n), selector_(sel)
    { simple_type(WRAPPED_SEL); }
    Wrapped_Selector(const Wrapped_Selector* ptr)
    : Simple_Selector(ptr), selector_(ptr->selector_)
    { simple_type(WRAPPED_SEL); }
    using Simple_Selector::is_superselector_of;
    bool is_superselector_of(Wrapped_Selector_Ptr_Const sub) const;
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
    Compound_Selector(ParserState pstate, size_t s = 0)
    : Selector(pstate),
      Vectorized<Simple_Selector_Obj>(s),
      extended_(false),
      has_parent_reference_(false)
    { }
    Compound_Selector(const Compound_Selector* ptr)
    : Selector(ptr),
      Vectorized<Simple_Selector_Obj>(*ptr),
      extended_(ptr->extended_),
      has_parent_reference_(ptr->has_parent_reference_)
    { }
    bool contains_placeholder() {
      for (size_t i = 0, L = length(); i < L; ++i) {
        if ((*this)[i]->has_placeholder()) return true;
      }
      return false;
    };

    void append(Simple_Selector_Obj element) override;

    bool is_universal() const
    {
      return length() == 1 && (*this)[0]->is_universal();
    }

    Complex_Selector_Obj to_complex();
    Compound_Selector_Ptr unify_with(Compound_Selector_Ptr rhs);
    // virtual Placeholder_Selector_Ptr find_placeholder();
    bool has_parent_ref() const override;
    bool has_real_parent_ref() const override;
    Simple_Selector_Ptr base() const {
      if (length() == 0) return 0;
      // ToDo: why is this needed?
      if (Cast<Type_Selector>((*this)[0]))
        return (*this)[0];
      return 0;
    }
    bool is_superselector_of(Compound_Selector_Ptr_Const sub, std::string wrapped = "") const;
    bool is_superselector_of(Complex_Selector_Ptr_Const sub, std::string wrapped = "") const;
    bool is_superselector_of(Selector_List_Ptr_Const sub, std::string wrapped = "") const;
    size_t hash() const override
    {
      if (Selector::hash_ == 0) {
        hash_combine(Selector::hash_, std::hash<int>()(SELECTOR));
        if (length()) hash_combine(Selector::hash_, Vectorized::hash());
      }
      return Selector::hash_;
    }
    unsigned long specificity() const override
    {
      int sum = 0;
      for (size_t i = 0, L = length(); i < L; ++i)
      { sum += (*this)[i]->specificity(); }
      return sum;
    }
    int unification_order() const override
    {
      throw std::runtime_error("unification_order for Compound_Selector is undefined");
    }

    bool has_placeholder()
    {
      if (length() == 0) return false;
      if (Simple_Selector_Obj ss = elements().front()) {
        if (ss->has_placeholder()) return true;
      }
      return false;
    }

    bool is_empty_reference()
    {
      return length() == 1 &&
             Cast<Parent_Selector>((*this)[0]);
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

    Compound_Selector_Ptr minus(Compound_Selector_Ptr rhs);
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
                     String_Obj r = {})
    : Selector(pstate),
      combinator_(c),
      head_(h), tail_(t),
      reference_(r)
    {}
    Complex_Selector(const Complex_Selector* ptr)
    : Selector(ptr),
      combinator_(ptr->combinator_),
      head_(ptr->head_), tail_(ptr->tail_),
      reference_(ptr->reference_)
    {};
    bool empty() const {
      return (!tail() || tail()->empty())
        && (!head() || head()->empty())
        && combinator_ == ANCESTOR_OF;
    }
    bool has_parent_ref() const override;
    bool has_real_parent_ref() const override;

    Complex_Selector_Obj skip_empty_reference()
    {
      if ((!head_ || !head_->length() || head_->is_empty_reference()) &&
          combinator() == Combinator::ANCESTOR_OF)
      {
        if (!tail_) return {};
        tail_->has_line_feed_ = this->has_line_feed_;
        // tail_->has_line_break_ = this->has_line_break_;
        return tail_->skip_empty_reference();
      }
      return this;
    }

    // can still have a tail
    bool is_empty_ancestor() const
    {
      return (!head() || head()->length() == 0) &&
             combinator() == Combinator::ANCESTOR_OF;
    }

    Selector_List_Ptr tails(Selector_List_Ptr tails);

    // front returns the first real tail
    // skips over parent and empty ones
    Complex_Selector_Ptr_Const first() const;
    Complex_Selector_Ptr mutable_first();

    // last returns the last real tail
    Complex_Selector_Ptr_Const last() const;
    Complex_Selector_Ptr mutable_last();

    size_t length() const;
    Selector_List_Ptr resolve_parent_refs(SelectorStack& pstack, Backtraces& traces, bool implicit_parent = true);
    bool is_superselector_of(Compound_Selector_Ptr_Const sub, std::string wrapping = "") const;
    bool is_superselector_of(Complex_Selector_Ptr_Const sub, std::string wrapping = "") const;
    bool is_superselector_of(Selector_List_Ptr_Const sub, std::string wrapping = "") const;
    Selector_List_Ptr unify_with(Complex_Selector_Ptr rhs);
    Combinator clear_innermost();
    void append(Complex_Selector_Obj, Backtraces& traces);
    void set_innermost(Complex_Selector_Obj, Combinator);
    size_t hash() const override
    {
      if (hash_ == 0) {
        hash_combine(hash_, std::hash<int>()(SELECTOR));
        hash_combine(hash_, std::hash<int>()(combinator_));
        if (head_) hash_combine(hash_, head_->hash());
        if (tail_) hash_combine(hash_, tail_->hash());
      }
      return hash_;
    }
    unsigned long specificity() const override
    {
      int sum = 0;
      if (head()) sum += head()->specificity();
      if (tail()) sum += tail()->specificity();
      return sum;
    }
    int unification_order() const override
    {
      throw std::runtime_error("unification_order for Complex_Selector is undefined");
    }
    void set_media_block(Media_Block_Ptr mb) override {
      media_block(mb);
      if (tail_) tail_->set_media_block(mb);
      if (head_) head_->set_media_block(mb);
    }
    bool has_placeholder() {
      if (head_ && head_->has_placeholder()) return true;
      if (tail_ && tail_->has_placeholder()) return true;
      return false;
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

    const ComplexSelectorSet sources()
    {
      //s = Set.new
      //seq.map {|sseq_or_op| s.merge sseq_or_op.sources if sseq_or_op.is_a?(SimpleSequence)}
      //s

      ComplexSelectorSet srcs;

      Compound_Selector_Obj pHead = head();
      Complex_Selector_Obj  pTail = tail();

      if (pHead) {
        const ComplexSelectorSet& headSources = pHead->sources();
        srcs.insert(headSources.begin(), headSources.end());
      }

      if (pTail) {
        const ComplexSelectorSet& tailSources = pTail->sources();
        srcs.insert(tailSources.begin(), tailSources.end());
      }

      return srcs;
    }
    void addSources(ComplexSelectorSet& sources) {
      // members.map! {|m| m.is_a?(SimpleSequence) ? m.with_more_sources(sources) : m}
      Complex_Selector_Ptr pIter = this;
      while (pIter) {
        Compound_Selector_Ptr pHead = pIter->head();

        if (pHead) {
          pHead->mergeSources(sources);
        }

        pIter = pIter->tail();
      }
    }
    void clearSources() {
      Complex_Selector_Ptr pIter = this;
      while (pIter) {
        Compound_Selector_Ptr pHead = pIter->head();

        if (pHead) {
          pHead->clearSources();
        }

        pIter = pIter->tail();
      }
    }

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
    Selector_List(ParserState pstate, size_t s = 0)
    : Selector(pstate),
      Vectorized<Complex_Selector_Obj>(s),
      schema_({}),
      wspace_(0)
    { }
    Selector_List(const Selector_List* ptr)
    : Selector(ptr),
      Vectorized<Complex_Selector_Obj>(*ptr),
      schema_(ptr->schema_),
      wspace_(ptr->wspace_)
    { }
    std::string type() const override { return "list"; }
    // remove parent selector references
    // basically unwraps parsed selectors
    bool has_parent_ref() const override;
    bool has_real_parent_ref() const override;
    void remove_parent_selectors();
    Selector_List_Ptr resolve_parent_refs(SelectorStack& pstack, Backtraces& traces, bool implicit_parent = true);
    bool is_superselector_of(Compound_Selector_Ptr_Const sub, std::string wrapping = "") const;
    bool is_superselector_of(Complex_Selector_Ptr_Const sub, std::string wrapping = "") const;
    bool is_superselector_of(Selector_List_Ptr_Const sub, std::string wrapping = "") const;
    Selector_List_Ptr unify_with(Selector_List_Ptr);
    void populate_extends(Selector_List_Obj, Subset_Map&);
    Selector_List_Obj eval(Eval& eval);
    size_t hash() const override
    {
      if (Selector::hash_ == 0) {
        hash_combine(Selector::hash_, std::hash<int>()(SELECTOR));
        hash_combine(Selector::hash_, Vectorized::hash());
      }
      return Selector::hash_;
    }
    unsigned long specificity() const override
    {
      unsigned long sum = 0;
      unsigned long specificity;
      for (size_t i = 0, L = length(); i < L; ++i)
      {
        specificity = (*this)[i]->specificity();
        if (sum < specificity) sum = specificity;
      }
      return sum;
    }
    int unification_order() const override
    {
      throw std::runtime_error("unification_order for Selector_List is undefined");
    }
    void set_media_block(Media_Block_Ptr mb) override {
      media_block(mb);
      for (Complex_Selector_Obj cs : elements()) {
        cs->set_media_block(mb);
      }
    }
    bool has_placeholder() {
      for (Complex_Selector_Obj cs : elements()) {
        if (cs->has_placeholder()) return true;
      }
      return false;
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
