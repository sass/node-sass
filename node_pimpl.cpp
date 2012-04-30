namespace Sass {
  
  inline bool Node::has_children()   { return ip_->has_children; }
  inline bool Node::has_statements() { return ip_->has_statements; }
  inline bool Node::has_blocks()     { return ip_->has_blocks; }
  inline bool Node::has_expansions() { return ip_->has_expansions; }
  inline bool Node::has_backref()    { return ip_->has_backref; }
  inline bool Node::from_variable()  { return ip_->from_variable; }
  inline bool Node::eval_me()        { return ip_->eval_me; }
  inline bool Node::is_unquoted()    { return ip_->is_unquoted; }
  inline bool Node::is_numeric()     { return ip_->is_numeric(); }
  
  inline size_t Node::line_number()  { return ip_->line_number; }
  
}