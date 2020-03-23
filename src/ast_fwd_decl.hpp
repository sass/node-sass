#ifndef SASS_AST_FWD_DECL_H
#define SASS_AST_FWD_DECL_H

// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"

#include "sass/functions.h"
#include "memory/SharedPtr.hpp"

/////////////////////////////////////////////
// Forward declarations for the AST visitors.
/////////////////////////////////////////////
namespace Sass {

  class AST_Node;

  class Has_Block;

  class SimpleSelector;

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

  class MediaRule;
  class CssMediaRule;
  class CssMediaQuery;

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
  class ExtendRule;
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
  class Null;

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
  
  class At_Root_Query;
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
  
  class SelectorComponent;
  class SelectorCombinator;
  class CompoundSelector;
  class ComplexSelector;
  class SelectorList;

  // common classes
  class Context;
  class Expand;
  class Eval;

  class Extension;

  // declare classes that are instances of memory nodes
  // Note: also add a mapping without underscore
  // ToDo: move to camelCase vars in the future
  #define IMPL_MEM_OBJ(type) \
    typedef SharedImpl<type> type##Obj; \
    typedef SharedImpl<type> type##_Obj; \

  IMPL_MEM_OBJ(AST_Node);
  IMPL_MEM_OBJ(Statement);
  IMPL_MEM_OBJ(Block);
  IMPL_MEM_OBJ(Ruleset);
  IMPL_MEM_OBJ(Bubble);
  IMPL_MEM_OBJ(Trace);
  IMPL_MEM_OBJ(MediaRule);
  IMPL_MEM_OBJ(CssMediaRule);
  IMPL_MEM_OBJ(CssMediaQuery);
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
  IMPL_MEM_OBJ(ExtendRule);
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
  IMPL_MEM_OBJ(Parent_Reference);
  IMPL_MEM_OBJ(Parameter);
  IMPL_MEM_OBJ(Parameters);
  IMPL_MEM_OBJ(Argument);
  IMPL_MEM_OBJ(Arguments);
  IMPL_MEM_OBJ(Selector);
  IMPL_MEM_OBJ(Selector_Schema);
  IMPL_MEM_OBJ(SimpleSelector);
  IMPL_MEM_OBJ(Placeholder_Selector);
  IMPL_MEM_OBJ(Type_Selector);
  IMPL_MEM_OBJ(Class_Selector);
  IMPL_MEM_OBJ(Id_Selector);
  IMPL_MEM_OBJ(Attribute_Selector);
  IMPL_MEM_OBJ(Pseudo_Selector);

  IMPL_MEM_OBJ(SelectorComponent);
  IMPL_MEM_OBJ(SelectorCombinator);
  IMPL_MEM_OBJ(CompoundSelector);
  IMPL_MEM_OBJ(ComplexSelector);
  IMPL_MEM_OBJ(SelectorList);

  // ###########################################################################
  // some often used typedefs
  // ###########################################################################

  typedef std::vector<Block*> BlockStack;
  typedef std::vector<Sass_Callee> CalleeStack;
  typedef std::vector<AST_Node_Obj> CallStack;
  typedef std::vector<CssMediaRuleObj> MediaStack;
  typedef std::vector<SelectorListObj> SelectorStack;
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
  DECLARE_BASE_CAST(SimpleSelector)
  DECLARE_BASE_CAST(SelectorComponent)

}

#endif
