#ifndef SASS_AST_FWD_DECL_H
#define SASS_AST_FWD_DECL_H

#include <map>
#include <set>
#include <deque>
#include <vector>
#include <typeinfo>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "memory/SharedPtr.hpp"
#include "sass/functions.h"

/////////////////////////////////////////////
// Forward declarations for the AST visitors.
/////////////////////////////////////////////
namespace Sass {

  class AST_Node;

  class Has_Block;

  class Simple_Selector;

  class Parent_Reference;

  class PreValue;
  class Block;
  class Expression;
  class Statement;
  class Value;
  class Declaration;
  class Ruleset;
  class Bubble;
  class Trace;

  class Media_Block;
  class Supports_Block;
  class Directive;


  class Keyframe_Rule;
  class At_Root_Block;
  class Assignment;

  class Import;
  class Import_Stub;
  class Warning;

  class Error;
  class Debug;
  class Comment;

  class If;
  class For;
  class Each;
  class While;
  class Return;
  class Content;
  class Extension;
  class Definition;

  class List;
  class Map;
  class Function;

  class Mixin_Call;
  class Binary_Expression;
  class Unary_Expression;
  class Function_Call;
  class Custom_Warning;
  class Custom_Error;

  class Variable;
  class Number;
  class Color;
  class Color_RGBA;
  class Color_HSLA;
  class Boolean;
  class String;

  class String_Schema;
  class String_Constant;
  class String_Quoted;

  class Media_Query;
  class Media_Query_Expression;
  class Supports_Condition;
  class Supports_Operator;
  class Supports_Negation;
  class Supports_Declaration;
  class Supports_Interpolation;


  class Null;

  class At_Root_Query;
  class Parent_Selector;
  class Parameter;
  class Parameters;
  class Argument;
  class Arguments;
  class Selector;


  class Selector_Schema;
  class Placeholder_Selector;
  class Type_Selector;
  class Class_Selector;
  class Id_Selector;
  class Attribute_Selector;

  class Pseudo_Selector;
  class Wrapped_Selector;
  class Compound_Selector;
  class Complex_Selector;
  class Selector_List;


  // common classes
  class Context;
  class Expand;
  class Eval;

  // declare classes that are instances of memory nodes
  // #define IMPL_MEM_OBJ(type) using type##_Obj = SharedImpl<type>
  #define IMPL_MEM_OBJ(type) typedef SharedImpl<type> type##_Obj

  IMPL_MEM_OBJ(AST_Node);
  IMPL_MEM_OBJ(Statement);
  IMPL_MEM_OBJ(Block);
  IMPL_MEM_OBJ(Ruleset);
  IMPL_MEM_OBJ(Bubble);
  IMPL_MEM_OBJ(Trace);
  IMPL_MEM_OBJ(Media_Block);
  IMPL_MEM_OBJ(Supports_Block);
  IMPL_MEM_OBJ(Directive);
  IMPL_MEM_OBJ(Keyframe_Rule);
  IMPL_MEM_OBJ(At_Root_Block);
  IMPL_MEM_OBJ(Declaration);
  IMPL_MEM_OBJ(Assignment);
  IMPL_MEM_OBJ(Import);
  IMPL_MEM_OBJ(Import_Stub);
  IMPL_MEM_OBJ(Warning);
  IMPL_MEM_OBJ(Error);
  IMPL_MEM_OBJ(Debug);
  IMPL_MEM_OBJ(Comment);
  IMPL_MEM_OBJ(PreValue);
  IMPL_MEM_OBJ(Has_Block);
  IMPL_MEM_OBJ(If);
  IMPL_MEM_OBJ(For);
  IMPL_MEM_OBJ(Each);
  IMPL_MEM_OBJ(While);
  IMPL_MEM_OBJ(Return);
  IMPL_MEM_OBJ(Content);
  IMPL_MEM_OBJ(Extension);
  IMPL_MEM_OBJ(Definition);
  IMPL_MEM_OBJ(Mixin_Call);
  IMPL_MEM_OBJ(Value);
  IMPL_MEM_OBJ(Expression);
  IMPL_MEM_OBJ(List);
  IMPL_MEM_OBJ(Map);
  IMPL_MEM_OBJ(Function);
  IMPL_MEM_OBJ(Binary_Expression);
  IMPL_MEM_OBJ(Unary_Expression);
  IMPL_MEM_OBJ(Function_Call);
  IMPL_MEM_OBJ(Custom_Warning);
  IMPL_MEM_OBJ(Custom_Error);
  IMPL_MEM_OBJ(Variable);
  IMPL_MEM_OBJ(Number);
  IMPL_MEM_OBJ(Color);
  IMPL_MEM_OBJ(Color_RGBA);
  IMPL_MEM_OBJ(Color_HSLA);
  IMPL_MEM_OBJ(Boolean);
  IMPL_MEM_OBJ(String_Schema);
  IMPL_MEM_OBJ(String);
  IMPL_MEM_OBJ(String_Constant);
  IMPL_MEM_OBJ(String_Quoted);
  IMPL_MEM_OBJ(Media_Query);
  IMPL_MEM_OBJ(Media_Query_Expression);
  IMPL_MEM_OBJ(Supports_Condition);
  IMPL_MEM_OBJ(Supports_Operator);
  IMPL_MEM_OBJ(Supports_Negation);
  IMPL_MEM_OBJ(Supports_Declaration);
  IMPL_MEM_OBJ(Supports_Interpolation);
  IMPL_MEM_OBJ(At_Root_Query);
  IMPL_MEM_OBJ(Null);
  IMPL_MEM_OBJ(Parent_Selector);
  IMPL_MEM_OBJ(Parent_Reference);
  IMPL_MEM_OBJ(Parameter);
  IMPL_MEM_OBJ(Parameters);
  IMPL_MEM_OBJ(Argument);
  IMPL_MEM_OBJ(Arguments);
  IMPL_MEM_OBJ(Selector);
  IMPL_MEM_OBJ(Selector_Schema);
  IMPL_MEM_OBJ(Simple_Selector);
  IMPL_MEM_OBJ(Placeholder_Selector);
  IMPL_MEM_OBJ(Type_Selector);
  IMPL_MEM_OBJ(Class_Selector);
  IMPL_MEM_OBJ(Id_Selector);
  IMPL_MEM_OBJ(Attribute_Selector);
  IMPL_MEM_OBJ(Pseudo_Selector);
  IMPL_MEM_OBJ(Wrapped_Selector);
  IMPL_MEM_OBJ(Compound_Selector);
  IMPL_MEM_OBJ(Complex_Selector);
  IMPL_MEM_OBJ(Selector_List);

  // ###########################################################################
  // Implement compare, order and hashing operations for AST Nodes
  // ###########################################################################

  struct HashNodes {
    template <class T>
    size_t operator() (const T& ex) const {
      return ex.isNull() ? 0 : ex->hash();
    }
  };
  template <class T>
  bool OrderFunction(const T& lhs, const T& rhs) {
      return !lhs.isNull() && !rhs.isNull() && *lhs < *rhs;
  };
  struct OrderNodes {
    template <class T>
    bool operator() (const T& lhs, const T& rhs) const {
      return OrderFunction<T>(lhs, rhs);
    }
  };
  template <class T>
  bool CompareFunction(const T& lhs, const T& rhs) {
      // code around sass logic issue. 1px == 1 is true
      // but both items are still different keys in maps
      if (dynamic_cast<Number*>(lhs.ptr()))
        if (dynamic_cast<Number*>(rhs.ptr()))
          return lhs->hash() == rhs->hash();
      return !lhs.isNull() && !rhs.isNull() && *lhs == *rhs;
  }
  struct CompareNodes {
    template <class T>
    bool operator() (const T& lhs, const T& rhs) const {
      return CompareFunction<T>(lhs, rhs);
    }
  };

  struct HashPtr {
    template <class T>
    size_t operator() (const T *ref) const {
      return ref->hash();
    }
  };
  struct ComparePtrs {
    template <class T>
    bool operator() (const T *lhs, const T *rhs) const {
      return *lhs == *rhs;
    }
  };

  // ###########################################################################
  // some often used typedefs
  // ###########################################################################

  typedef std::unordered_map<
    Expression_Obj, // key
    Expression_Obj, // value
    HashNodes, // hasher
    CompareNodes // compare
  > ExpressionMap;
  typedef std::unordered_set<
    Expression_Obj, // value
    HashNodes, // hasher
    CompareNodes // compare
  > ExpressionSet;

  typedef std::string SubSetMapKey;
  typedef std::vector<std::string> SubSetMapKeys;

  typedef std::pair<Complex_Selector_Obj, Compound_Selector_Obj> SubSetMapPair;
  typedef std::pair<Compound_Selector_Obj, Complex_Selector_Obj> SubSetMapLookup;
  typedef std::vector<SubSetMapPair> SubSetMapPairs;
  typedef std::vector<SubSetMapLookup> SubSetMapLookups;

  typedef std::pair<Complex_Selector_Obj, SubSetMapPairs> SubSetMapResult;
  typedef std::vector<SubSetMapResult> SubSetMapResults;

  typedef std::set<Selector_Obj, OrderNodes> SelectorSet;

  typedef std::deque<Complex_Selector_Obj> ComplexSelectorDeque;
  typedef std::set<Simple_Selector_Obj, OrderNodes> SimpleSelectorSet;
  typedef std::set<Complex_Selector_Obj, OrderNodes> ComplexSelectorSet;
  typedef std::set<Compound_Selector_Obj, OrderNodes> CompoundSelectorSet;
  typedef std::unordered_set<Simple_Selector_Obj, HashNodes, CompareNodes> SimpleSelectorDict;

  typedef std::vector<Block*> BlockStack;
  typedef std::vector<Sass_Callee> CalleeStack;
  typedef std::vector<AST_Node_Obj> CallStack;
  typedef std::vector<Media_Block*> MediaStack;
  typedef std::vector<Selector_List_Obj> SelectorStack;
  typedef std::vector<Sass_Import_Entry> ImporterStack;

  // only to switch implementations for testing
  #define environment_map std::map

  // ###########################################################################
  // explicit type conversion functions
  // ###########################################################################

  template<class T>
  T* Cast(AST_Node* ptr);

  template<class T>
  const T* Cast(const AST_Node* ptr);

  // sometimes you know the class you want to cast to is final
  // in this case a simple typeid check is faster and safe to use

  #define DECLARE_BASE_CAST(T) \
  template<> T* Cast(AST_Node* ptr); \
  template<> const T* Cast(const AST_Node* ptr); \

  // ###########################################################################
  // implement specialization for final classes
  // ###########################################################################

  DECLARE_BASE_CAST(AST_Node)
  DECLARE_BASE_CAST(Expression)
  DECLARE_BASE_CAST(Statement)
  DECLARE_BASE_CAST(Has_Block)
  DECLARE_BASE_CAST(PreValue)
  DECLARE_BASE_CAST(Value)
  DECLARE_BASE_CAST(List)
  DECLARE_BASE_CAST(Color)
  DECLARE_BASE_CAST(String)
  DECLARE_BASE_CAST(String_Constant)
  DECLARE_BASE_CAST(Supports_Condition)
  DECLARE_BASE_CAST(Selector)
  DECLARE_BASE_CAST(Simple_Selector)

}

#endif
