#define ATTACH_OPERATIONS()\
virtual void perform(Operation<void>* op) { (*op)(this); }\
virtual AST_Node* perform(Operation<AST_Node*>* op) { return (*op)(this); }\
virtual string perform(Operation<string>* op) { return (*op)(this); }

#define ADD_PROPERTY(type, name)\
private:\
  type name##_;\
public:\
  type name() const        { return name##_; }\
  type name(type name##__) { return name##_ = name##__; }\
private:
