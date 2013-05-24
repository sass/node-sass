/////////////////////////////////////////////
// Forward declarations for the AST visitors.
/////////////////////////////////////////////
namespace Sass {

  enum Binary_Operator {
    AND, OR,                   // logical connectives
    EQ, NEQ, GT, GTE, LT, LTE, // arithmetic relations
    ADD, SUB, MUL, DIV         // arithmetic functions
  };
  enum Textual_Type { NUMBER, PERCENTAGE, DIMENSION, HEX };

	class AST_Node;
	// statements
	class Block;
	class Ruleset;
	class Propset;
	class Media_Query;
	class At_Rule;
	class Declaration;
	class Assignment;
	class Import;
	class Warning;
	class Comment;
	class If;
	class For;
	class Each;
	class While;
	class Extend;
	class Definition;
	class Mixin_Call;
	// expressions
	class List;
	template <Binary_Operator op> class Binary_Expression;
	class Negation;
	class Function_Call;
	class Variable;
	template <Textual_Type T> class Textual;
	class Number;
	class Percentage;
	class Dimension;
	class Color;
	class Boolean;
	class String_Schema;
	class String;
	class String_Constant;
	class Media_Expression;
	// parameters and arguments
	class Parameter;
	class Parameters;
	class Argument;
	class Arguments;
	// selectors
	class Selector_Schema;
	class Selector_Reference;
	class Selector_Placeholder;
	class Type_Selector;
	class Selector_Qualifier;
	class Attribute_Selector;
	class Pseudo_Selector;
	class Negated_Selector;
	class Simple_Selector_Sequence;
	class Selector_Combination;
	class Selector_Group;

}