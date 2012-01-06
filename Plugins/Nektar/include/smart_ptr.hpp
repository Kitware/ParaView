/**
 * \file smart_ptr.hpp
 * \author Angus Leeming, a.leeming@ic.ac.uk
 * \date April 2002
 *
 * class nektar::scoped_ptr is a small extension of the boost::scoped_ptr idea.
 * See www.boost.org for more about boost.
 *
 * The original, boost smart_ptr.hpp file contains the following notice:
 *
 * (C) Copyright Greg Colvin and Beman Dawes 1998, 1999. Permission to copy,
 * use, modify, sell and distribute this software is granted provided this
 * copyright notice appears in all copies. This software is provided "as is"
 * without express or implied warranty, and with no claim as to its
 * suitability for any purpose.
 *
 * nektar::scoped_ptr can be used in identical fashion to the boost class,
 * but can also be used to invoke an arbitrary functor when it goes out of
 * scope.
 *
 * A simple example:
 *
 *   void example()
 *   {
 *      nektar::scoped_c_ptr<Foo> scoped_foo((Foo*)malloc(sizeof(Foo)));
 *      Foo * foo = scoped_foo.get();
 *      ...
 *      // scoped_foo calls free(foo) automatically as it goes out of scope
 *   }
 *
 * The additional power lies in being able to define arbitrary functors to
 * perform the destruction. Great with c-style structs that have no d-tor.
 */

#ifndef NEKTAR_SMART_PTR_HPP
#define NEKTAR_SMART_PTR_HPP

namespace nektar{
  class noncopyable {
  protected:
    noncopyable(){}
    ~noncopyable(){}
  private:
    noncopyable( const noncopyable& );
    const noncopyable& operator=( const noncopyable& );
  };

  template< typename T >
  struct DeletePtr {
    void operator()(T * x) { delete x; }
  };

  template< typename T, typename Destroyer=DeletePtr<T> >

  class scoped_ptr : noncopyable {
    T* ptr;

    void destroy_ptr() {
      if (ptr) {
  Destroyer destroy;
  destroy(ptr);
      }
    }

  public:
    typedef T element_type;

    explicit scoped_ptr( T* p=0 ) : ptr(p) {}
    ~scoped_ptr() { destroy_ptr(); }
    void reset( T* p=0 )  { if ( ptr != p ) { destroy_ptr(); ptr = p; } }
    T& operator*()  const { return *ptr; }
    T* operator->() const { return ptr; }
    T* get() const        { return ptr; }

    // This method makes sense only if ptr points to an array.
    // boost::scoped_ptr does not have an operator[]; rather they define a
    // boost::scoped_array.
    // This seems un-necessary to me, as a raw ptr can be accessed this way.
    T& operator[](size_t i) const { return ptr[i]; }
  };

  // and some shorthand helpers...
  template< typename T >
  struct DeleteArrayPtr {
    void operator()(T * x) { delete [] x; }
  };

  template< typename T >
  struct FreePtr {
    void operator()(T * x) { free(x); }
  };

  template< typename T >
  struct scoped_array_ptr : public scoped_ptr<T, DeleteArrayPtr<T> > {
    explicit scoped_array_ptr( T* p=0 )
      : scoped_ptr<T, DeleteArrayPtr<T> >(p) {}
  };

  template< typename T >
  struct scoped_c_ptr : public scoped_ptr<T, FreePtr<T> > {
    explicit scoped_c_ptr( T* p=0 )
      : scoped_ptr<T, FreePtr<T> >(p) {}
  };

} // namespace nektar

#endif // NEKTAR_SMART_PTR_HPP
