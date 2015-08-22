#include "ast.hpp"
#include "memory_manager.hpp"

namespace Sass {

  template <typename T>
  Memory_Manager<T>::Memory_Manager(size_t size)
  : nodes(std::vector<T*>())
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
  T* Memory_Manager<T>::add(T* np)
  {
    void* heap = (char*)np - 1;
    *((char*)((void*)heap)) = 1;
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
    // need additional memory for status header
    void* heap = malloc(sizeof(char) + size);
    // init internal status to zero
    *(static_cast<char*>(heap)) = 0;
    // the object lives on char further
    void* object = static_cast<char*>(heap) + 1;
    // add the memory under our management
    nodes.push_back(static_cast<T*>(object));
    // cast object to its final type
    return static_cast<T*>(object);
  }

  template <typename T>
  void Memory_Manager<T>::deallocate(T* np)
  {
    void* object = static_cast<void*>(np);
    char* heap = static_cast<char*>(object) - 1;
    if (heap[0]) np->~T();
    free(heap);
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
    deallocate(np);
  }

  // compile implementation for AST_Node
  template class Memory_Manager<AST_Node>;

}

