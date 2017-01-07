#ifndef SASS_MEMORY_SHARED_PTR_H
#define SASS_MEMORY_SHARED_PTR_H

#include "sass/base.h"

#include <vector>

namespace Sass {

  class SharedPtr;

  ///////////////////////////////////////////////////////////////////////////////
  // Use macros for the allocation task, since overloading operator `new`
  // has been proven to be flaky under certain compilers (see comment below).
  ///////////////////////////////////////////////////////////////////////////////

  #ifdef DEBUG_SHARED_PTR

    #define SASS_MEMORY_NEW(Class, ...) \
      static_cast<Class##_Ptr>((new Class(__VA_ARGS__))->trace(__FILE__, __LINE__)) \

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

  #define SASS_MEMORY_CAST(Class, obj) \
    (dynamic_cast<Class##_Ptr>(&obj)) \

  #define SASS_MEMORY_CAST_PTR(Class, ptr) \
    (dynamic_cast<Class##_Ptr>(ptr)) \

  class SharedObj {
  protected:
  friend class SharedPtr;
  friend class Memory_Manager;
    #ifdef DEBUG_SHARED_PTR
      static std::vector<SharedObj*> all;
      std::string file;
      size_t line;
    #endif
    static bool taint;
    long refcounter;
    // long refcount;
    bool detached;
    #ifdef DEBUG_SHARED_PTR
      bool dbg;
    #endif
  public:
    #ifdef DEBUG_SHARED_PTR
      static void dumpMemLeaks();
      SharedObj* trace(std::string file, size_t line) {
        this->file = file;
        this->line = line;
        return this;
      }
    #endif
    SharedObj();
    #ifdef DEBUG_SHARED_PTR
      std::string getDbgFile() {
        return file;
      }
      size_t getDbgLine() {
        return line;
      }
      void setDbg(bool dbg) {
        this->dbg = dbg;
      }
    #endif
    static void setTaint(bool val) {
      taint = val;
    }
    virtual ~SharedObj();
    long getRefCount() {
      return refcounter;
    }
  };


  class SharedPtr {
  private:
    SharedObj* node;
  private:
    void decRefCount();
    void incRefCount();
  public:
    // the empty constructor
    SharedPtr()
    : node(NULL) {};
    // the create constructor
    SharedPtr(SharedObj* ptr);
    // copy assignment operator
    SharedPtr& operator=(const SharedPtr& rhs);
    // move assignment operator
    /* SharedPtr& operator=(SharedPtr&& rhs); */
    // the copy constructor
    SharedPtr(const SharedPtr& obj);
    // the move constructor
    /* SharedPtr(SharedPtr&& obj); */
    // destructor
    ~SharedPtr();
  public:
    SharedObj* obj () {
      return node;
    };
    SharedObj* obj () const {
      return node;
    };
    SharedObj* operator-> () {
      return node;
    };
    bool isNull () {
      return node == NULL;
    };
    bool isNull () const {
      return node == NULL;
    };
    SharedObj* detach() {
      node->detached = true;
      return node;
    };
    SharedObj* detach() const {
      if (node) {
        node->detached = true;
      }
      return node;
    };
    operator bool() {
      return node != NULL;
    };
    operator bool() const {
      return node != NULL;
    };

  };

  template < typename T >
  class SharedImpl : private SharedPtr {
  public:
    SharedImpl()
    : SharedPtr(NULL) {};
    SharedImpl(T* node)
    : SharedPtr(node) {};
    SharedImpl(T&& node)
    : SharedPtr(node) {};
    SharedImpl(const T& node)
    : SharedPtr(node) {};
    ~SharedImpl() {};
  public:
    T* operator& () {
      return static_cast<T*>(this->obj());
    };
    T* operator& () const {
      return static_cast<T*>(this->obj());
    };
    T& operator* () {
      return *static_cast<T*>(this->obj());
    };
    T& operator* () const {
      return *static_cast<T*>(this->obj());
    };
    T* operator-> () {
      return static_cast<T*>(this->obj());
    };
    T* operator-> () const {
      return static_cast<T*>(this->obj());
    };
    T* ptr () {
      return static_cast<T*>(this->obj());
    };
    T* detach() {
      if (this->obj() == NULL) return NULL;
      return static_cast<T*>(SharedPtr::detach());
    }
    bool isNull() {
      return this->obj() == NULL;
    }
    operator bool() {
      return this->obj() != NULL;
    };
    operator bool() const {
      return this->obj() != NULL;
    };
  };

}

#endif