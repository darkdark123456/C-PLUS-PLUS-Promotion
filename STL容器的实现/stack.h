#ifndef _STACK_H
#define _STACK_H
#include"deque.h"
template <typename T, typename Sequence = MENG::deque<T> >
class stack {
  friend bool operator== __STL_NULL_TMPL_ARGS (const stack&, const stack&){return x.c == y.c;}
  friend bool operator< __STL_NULL_TMPL_ARGS (const stack&, const stack&) {return x.c < y.c;}
public:
  typedef typename Sequence::value_type value_type;
  typedef typename Sequence::size_type size_type;
  typedef typename Sequence::reference reference;
  typedef typename Sequence::const_reference const_reference;
protected:
  Sequence c;
public:
  bool empty() const { return c.empty(); }
  size_type size() const { return c.size(); }
  reference top() { return c.back(); }
  const_reference top() const { return c.back(); }
  void push(const value_type& x) { c.push_back(x); }
  void pop() { c.pop_back(); }
};
#endif