#include "ast.hpp"
#include "memory_manager.hpp"

namespace Sass {
  using namespace std;

  template <typename T>
  Memory_Manager<T>::Memory_Manager(size_t size)
  : nodes(vector<T*>())
  {
    size_t init = size;
    if (init < 8) init = 8;
    // reserve some space
    nodes.reserve(init);
  }

  template <typename T>
  Memory_Manager<T>::~Memory_Manager()
  {
    // release memory for all controlled nodes
    // avoid calling erase for every single node 
    for (size_t i = 0, S = nodes.size(); i < S; ++i) {
      deallocate(nodes[i]);
    }
    // just in case
    nodes.clear();
  }

  template <typename T>
  T* Memory_Manager<T>::operator()(T* np)
  {
    // add to pool
    nodes.push_back(np);
    // return resource
    return np;
  }

  template <typename T>
  bool Memory_Manager<T>::has(T* np)
  {
    // check if the pointer is controlled under our pool
    return find(nodes.begin(), nodes.end(), np) != nodes.end();
  }

  template <typename T>
  T* Memory_Manager<T>::allocate(size_t size)
  {
    return static_cast<T*>(operator new(size));
  }

  template <typename T>
  void Memory_Manager<T>::deallocate(T* np)
  {
    delete np;
  }

  template <typename T>
  void Memory_Manager<T>::remove(T* np)
  {
    // remove node from pool (no longer active)
    nodes.erase(find(nodes.begin(), nodes.end(), np));
    // you are now in control of the memory
  }

  template <typename T>
  void Memory_Manager<T>::destroy(T* np)
  {
    // remove from pool
    remove(np);
    // release memory
    delete np;
  }

  // compile implementation for AST_Node
  template class Memory_Manager<AST_Node>;

}

