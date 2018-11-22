#ifndef SASS_MEMORY_SHARED_PTR_H
#define SASS_MEMORY_SHARED_PTR_H

#include "sass/base.h"

#include <iostream>
#include <string>
#include <vector>

namespace Sass {

  class SharedPtr;

  ///////////////////////////////////////////////////////////////////////////////
  // Use macros for the allocation task, since overloading operator `new`
  // has been proven to be flaky under certain compilers (see comment below).
  ///////////////////////////////////////////////////////////////////////////////

  #ifdef DEBUG_SHARED_PTR

    #define SASS_MEMORY_NEW(Class, ...) \
      ((Class*)(new Class(__VA_ARGS__))->trace(__FILE__, __LINE__)) \

    #define SASS_MEMORY_COPY(obj) \
      ((obj)->copy(__FILE__, __LINE__)) \

    #define SASS_MEMORY_CLONE(obj) \
      ((obj)->clone(__FILE__, __LINE__)) \

  #else

    #define SASS_MEMORY_NEW(Class, ...) \
      new Class(__VA_ARGS__) \

    #define SASS_MEMORY_COPY(obj) \
      ((obj)->copy()) \

    #define SASS_MEMORY_CLONE(obj) \
      ((obj)->clone()) \

  #endif

  class SharedObj {
   public:
    SharedObj() : refcount(0), detached(false) {
      #ifdef DEBUG_SHARED_PTR
      if (taint) all.push_back(this);
      #endif
    }
    virtual ~SharedObj() {
      #ifdef DEBUG_SHARED_PTR
      all.clear();
      #endif
    }

    #ifdef DEBUG_SHARED_PTR
    static void dumpMemLeaks();
    SharedObj* trace(std::string file, size_t line) {
      this->file = file;
      this->line = line;
      return this;
    }
    std::string getDbgFile() { return file; }
    size_t getDbgLine() { return line; }
    void setDbg(bool dbg) { this->dbg = dbg; }
    size_t getRefCount() const { return refcount; }
    #endif

    static void setTaint(bool val) { taint = val; }

    virtual const std::string to_string() const = 0;
   protected:
    friend class SharedPtr;
    friend class Memory_Manager;
    size_t refcount;
    bool detached;
    static bool taint;
    #ifdef DEBUG_SHARED_PTR
    std::string file;
    size_t line;
    bool dbg = false;
    static std::vector<SharedObj*> all;
    #endif
  };

  class SharedPtr {
   public:
    SharedPtr() : node(nullptr) {}
    SharedPtr(SharedObj* ptr) : node(ptr) {
      incRefCount();
    }
    SharedPtr(const SharedPtr& obj) : SharedPtr(obj.node) {}
    ~SharedPtr() {
      decRefCount();
    }

    SharedPtr& operator=(SharedObj* other_node) {
      if (node != other_node) {
        decRefCount();
        node = other_node;
        incRefCount();
      } else if (node != nullptr) {
        node->detached = false;
      }
      return *this;
    }

    SharedPtr& operator=(const SharedPtr& obj) {
      return *this = obj.node;
    }

    // Prevents all SharedPtrs from freeing this node until it is assigned to another SharedPtr.
    SharedObj* detach() {
      if (node != nullptr) node->detached = true;
      return node;
    }

    SharedObj* obj() const { return node; }
    SharedObj* operator->() const { return node; }
    bool isNull() const { return node == nullptr; }
    operator bool() const { return node != nullptr; }

   protected:
    SharedObj* node;
    void decRefCount() {
      if (node == nullptr) return;
      --node->refcount;
      #ifdef DEBUG_SHARED_PTR
      if (node->dbg) std::cerr << "- " << node << " X " << node->refcount << " (" << this << ") " << "\n";
      #endif
      if (node->refcount == 0 && !node->detached) {
        #ifdef DEBUG_SHARED_PTR
        if (node->dbg) std::cerr << "DELETE NODE " << node << "\n";
        #endif
        delete node;
      }
    }
    void incRefCount() {
      if (node == nullptr) return;
      node->detached = false;
      ++node->refcount;
      #ifdef DEBUG_SHARED_PTR
      if (node->dbg) std::cerr << "+ " << node << " X " << node->refcount << " (" << this << ") " << "\n";
      #endif
    }
  };

  template <class T>
  class SharedImpl : private SharedPtr {
   public:
    SharedImpl() : SharedPtr(nullptr) {}

    template <class U>
    SharedImpl(U* node) :
      SharedPtr(static_cast<T*>(node)) {}

    template <class U>
    SharedImpl(const SharedImpl<U>& impl) :
      SharedImpl(impl.ptr()) {}

    template <class U>
    SharedImpl<T>& operator=(U *rhs) {
      return static_cast<SharedImpl<T>&>(
        SharedPtr::operator=(static_cast<T*>(rhs)));
    }

    template <class U>
    SharedImpl<T>& operator=(const SharedImpl<U>& rhs) {
      return static_cast<SharedImpl<T>&>(
        SharedPtr::operator=(static_cast<const SharedImpl<T>&>(rhs)));
    }

    operator const std::string() const {
      if (node) return node->to_string();
      return "[nullptr]";
    }

    using SharedPtr::isNull;
    using SharedPtr::operator bool;
    operator T*() const { return static_cast<T*>(this->obj()); }
    operator T&() const { return *static_cast<T*>(this->obj()); }
    T& operator* () const { return *static_cast<T*>(this->obj()); };
    T* operator-> () const { return static_cast<T*>(this->obj()); };
    T* ptr () const { return static_cast<T*>(this->obj()); };
    T* detach() { return static_cast<T*>(SharedPtr::detach()); }
  };

}

#endif
