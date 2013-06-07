#include "cloner.hpp"
#include "memory_manager.hpp"

namespace Sass {

	// better to share objects when safe to do so
	AST_Node* Cloner::fallback(AST_Node* n)
	{
		return n;
	}

	AST_Node* Cloner::operator()(Block* b)
	{
		Block* bb = new (mem) Block(b->path(), b->line(), b->length());
		bb->is_root(b->is_root());
		for (size_t i = 0, L = bb->length(); i < L; ++i)
			(*bb)[i] << (*b)[i]->perform(this);
		return bb;
	}

	AST_Node* Cloner::operator()(Ruleset* r)
	{
		Ruleset* rr = new (mem) Ruleset(r->path(),
		                                r->line(),
		                                r->selector()->perform(this),
		                                r->block()->perform(this));
		return rr;
	}

	AST_Node* Cloner::operator()(Propset* p)
	{
		Propset* pp = new (mem) Propset(p->path(),
		                                p->line(),
		                                p->property_fragment()->perform(this),
		                                p->block()->perform(this));
		return pp;
	}

	AST_Node* Cloner::operator()(Media_Block* m)
	{
		Media_Block* mm = new (mem) Media_Block(m->path(),
		                                        m->line(),
		                                        m->media_queries()->perform(this),
		                                        m->block()->perform(this));
		return mm;
	}

	AST_Node* Cloner::operator()(At_Rule* a)
	{
		Selector* s = a->selector();
		Block*    b = a->block();
		At_Rule* aa = new (mem) At_Rule(a->path(),
		                                a->line(),
		                                a->keyword(),
		                                s ? s->perform(this) : 0,
		                                b ? b->perform(this) : 0);
		return aa;
	}

	AST_Node* Cloner::operator()(Declaration* d)
	{
		Declaration* dd = new (mem) Declaration(d->path(),
		                                        d->line(),
		                                        d->property()->perform(this),
		                                        d->value()->perform(this),
		                                        d->is_important());
		return dd;
	}

	AST_Node* Cloner::operator()(Assignment* a)
	{
		Assignment* aa = new (mem) Assignment(a->path(),
		                                      a->line(),
		                                      a->variable(),
		                                      a->value()->perform(this),
		                                      a->is_guarded());
		return aa;
	}

	AST_Node* Cloner::operator()(Import* i)
	{
		Import* ii = new (mem) Import(i->path(), i->line());
		ii->files() = i->files();
		for (size_t j = 0, S = i->urls().size(); j < S; ++j) {
			ii->urls()[j] = i->urls()[j]->perform(this);
		}
		return ii;
	}

	AST_Node* Cloner::operator()(Warning* w)
	{
		Warning* ww = new (mem) Warning(w->path(),
		                                w->line(),
		                                w->message()->perform(this));
		return ww;
	}

	AST_Node* Cloner::operator()(Comment* c)
	{
		Comment* cc = new (mem) Comment(w->path(),
		                                w->line(),
		                                w->text()->perform(this));
		return cc;
	}

	AST_Node* Cloner::operator()(If* i)
	{
		Statement* a = i->alternative();
		return new (mem) If(i->path(),
		                    i->line(),
		                    i->predicate()->perform(this),
		                    i->consequent()->perform(this),
		                    a ? a->perform(this) : 0);
	}

	AST_Node* Cloner::operator()(For* f)
	{
		return new (mem) For(f->path(),
		                     f->line(),
		                     f->variable(),
		                     f->lower_bound()->perform(this),
		                     f->upper_bound()->perform(this),
		                     f->block()->perform(this),
		                     f->is_inclusive());
	}

	AST_Node* Cloner::operator()(Each* e)
	{
		return new (mem) Each(e->path(),
		                      e->line(),
		                      e->variable(),
		                      e->list()->perform(this),
		                      e->block()->perform(this));
	}

	AST_Node* Cloner::operator()(While* w)
	{
		return new (mem) While(w->path(),
		                       w->line(),
		                       w->predicate()->perform(this),
		                       w->block()->perform(this));
	}

	AST_Node* Cloner::operator()(Return* r)
	{
		return new (mem) Return(r->path(),
		                        r->line(),
		                        r->value()->perform(this));
	}


}