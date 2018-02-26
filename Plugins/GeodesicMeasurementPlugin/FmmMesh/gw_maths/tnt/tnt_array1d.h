/*
*
* Template Numerical Toolkit (TNT)
*
* Mathematical and Computational Sciences Division
* National Institute of Technology,
* Gaithersburg, MD USA
*
*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*
*/



#ifndef TNT_ARRAY1D_H
#define TNT_ARRAY1D_H

#include <cstdlib>
#include <iostream>

#ifdef TNT_BOUNDS_CHECK
#include <assert.h>
#endif

namespace TNT
{

/**
    Tempplated one-dimensional, numerical array which
    looks like a conventional C array.
    Elements are accessed via the familiar A[i] notation.

    <p>
    Array assignment is by reference (i.e. shallow assignment).
    That is, B=A implies that the A and B point to the
    same array, so modifications to the elements of A
    will be reflected in B. If an independent copy
    is required, then B = A.copy() can be used.  Note
    that this facilitates returning arrays from functions
    without relying on compiler optimizations to eliminate
    extensive data copying.

    <p>
    The indexing and layout of this array object makes
    it compatible with C and C++ algorithms that utilize
    the familiar C[i] notation.  This includes numerous
    textbooks, such as Numercial Recipes, and various
    public domain codes.

    <p>
    This class employs its own garbage collection via
    the use of reference counts.  That is, whenever
    an internal array storage no longer has any references
    to it, it is destroyed.
*/
template <class T>
class Array1D
{


  private:
    T* v_;
    int n_;
    int *ref_count_;

    void initialize_(int n);
    void copy_(T* p, const T*  q, int len) const;
    void set_(const T& val);
    void destroy_();
    inline const T* begin_() const;
    inline T* begin_();

  public:

    typedef         T   value_type;

             Array1D();
    explicit Array1D(int n);
             Array1D(int n,  T *a);
             Array1D(int n, const T &a);
    inline   Array1D(const Array1D &A);
    inline   Array1D & operator=(const T &a);
    inline   Array1D & operator=(const Array1D &A);
    inline   Array1D & ref(const Array1D &A);
             Array1D copy() const;
             Array1D & inject(const Array1D & A);
    inline   T& operator[](int i);
    inline   const T& operator[](int i) const;
    inline      int dim1() const;
    inline   int dim() const;
    inline   int ref_count() const;
               ~Array1D();


};

/**
    Null constructor.  Creates a 0-length (NULL) array.
    (Reference count is also zero.)
*/

template <class T>
Array1D<T>::Array1D() : v_(0), n_(0), ref_count_(0)
{
    ref_count_ = new int;
    *ref_count_ = 1;
}

/**
    Copy constructor. Array data is NOT copied, but shared.
    Thus, in Array1D B(A), subsequent changes to A will
    be reflected in B.  For an indepent copy of A, use
    Array1D B(A.copy()), or B = A.copy(), instead.
*/
template <class T>
Array1D<T>::Array1D(const Array1D<T> &A) : v_(A.v_),
    n_(A.n_), ref_count_(A.ref_count_)
{
    (*ref_count_)++;
}



/**
    Create a new array (vector) of length <b>n</b>,
    WITHOUT initializing array elements.
    To create an initialized array of constants, see Array1D(n,value).

    <p>
    This version avoids the O(n) initialization overhead and
    is used just before manual assignment.

    @param n the dimension (length) of the new matrix.
*/
template <class T>
Array1D<T>::Array1D(int n) : v_(0), n_(n), ref_count_(0)
{
    initialize_(n);
    ref_count_ = new int;
    *ref_count_ = 1;
}



/**
    Create a new array of length <b>n</b>,  initializing array elements to
    constant specified by argument.  Most often used to
    create an array of zeros, as in A(n, 0.0).

    @param n the dimension (length) of the new matrix.
    @param val the constant value to set all elements of the new array to.
*/
template <class T>
Array1D<T>::Array1D(int n, const T &val) : v_(0), n_(n) ,
    ref_count_(0)
{
    initialize_(n);
    set_(val);
    ref_count_ = new int;
    *ref_count_ = 1;

}

/**
    Create a new n-length array,  as a view of an existing one-dimensional
    C array.  (Note that the storage for this pre-existing array will
    never be destroyed by the Aray1DRef class.)

    @param n the dimension (length) of the new matrix.
    @param a the one dimensional C array to use as data storage for
        the array.
*/
template <class T>
Array1D<T>::Array1D(int n, T *a) : v_(a), n_(n) ,
    ref_count_(0)
{
    ref_count_ = new int;
    *ref_count_ = 2;        /* this avoid destroying original data. */

}

/**
    A[i] indexes the ith element of A.  The first element is
    A[0]. If TNT_BOUNDS_CHECK is defined, then the index is
    checked that it falls within the array bounds.
*/
template <class T>
inline T& Array1D<T>::operator[](int i)
{
#ifdef TNT_BOUNDS_CHECK
    assert(i>= 0);
    assert(i < n_);
#endif
    return v_[i];
}

/**
    A[i] indexes the ith element of A.  The first element is
    A[0]. If TNT_BOUNDS_CHECK is defined, then the index is
    checked that it fall within the array bounds.
*/
template <class T>
inline const T& Array1D<T>::operator[](int i) const
{
#ifdef TNT_BOUNDS_CHECK
    assert(i>= 0);
    assert(i < n_);
#endif
    return v_[i];
}

/**
    Assign all elemnts of A to a constant scalar.
*/
template <class T>
Array1D<T> & Array1D<T>::operator=(const T &a)
{
    set_(a);
    return *this;
}
/**
    Create a new of existing matrix.  Used in B = A.copy()
    or in the construction of B, e.g. Array1D B(A.copy()),
    to create a new array that does not share data.

*/
template <class T>
Array1D<T> Array1D<T>::copy() const
{
    Array1D A( n_);
    copy_(A.begin_(), begin_(), n_);

    return A;
}


/**
    Copy the elements to from one array to another, in place.
    That is B.inject(A), both A and B must conform (i.e. have
    identical row and column dimensions).

    This differs from B = A.copy() in that references to B
    before this assignment are also affected.  That is, if
    we have
    <pre>
    Array1D A(n);
    Array1D C(n);
    Array1D B(C);        // elements of B and C are shared.

</pre>
    then B.inject(A) affects both and C, while B=A.copy() creates
    a new array B which shares no data with C or A.

    @param A the array from which elements will be copied
    @return an instance of the modified array. That is, in B.inject(A),
    it returns B.  If A and B are not conformat, no modifications to
    B are made.

*/
template <class T>
Array1D<T> & Array1D<T>::inject(const Array1D &A)
{
    if (A.n_ == n_)
        copy_(begin_(), A.begin_(), n_);

    return *this;
}





/**
    Create a reference (shallow assignment) to another existing array.
    In B.ref(A), B and A shared the same data and subsequent changes
    to the array elements of one will be reflected in the other.
    <p>
    This is what operator= calls, and B=A and B.ref(A) are equivalent
    operations.

    @return The new referenced array: in B.ref(A), it returns B.
*/
template <class T>
Array1D<T> & Array1D<T>::ref(const Array1D<T> &A)
{
    if (this != &A)
    {
        (*ref_count_) --;
        if ( *ref_count_ < 1)
        {
            destroy_();
        }

        n_ = A.n_;
        v_ = A.v_;
        ref_count_ = A.ref_count_;

        (*ref_count_) ++ ;

    }
    return *this;
}

/**
    B = A is shorthand notation for B.ref(A).
*/
template <class T>
Array1D<T> & Array1D<T>::operator=(const Array1D<T> &A)
{
    return ref(A);
}

/**
    @return the dimension (number of elements) of the array.
    This is equivalent to dim() and dim1().
*/
template <class T>
inline int Array1D<T>::dim1() const { return n_; }

/**
    @return the dimension (number of elements) of the array.
    This is equivalent to dim1() and dim1().
*/
template <class T>
inline int Array1D<T>::dim() const { return n_; }



/**
    @return the number of arrays that share the same storage area
    as this one.  (Must be at least one.)
*/
template <class T>
inline int Array1D<T>::ref_count() const
{
    return *ref_count_;
}

template <class T>
Array1D<T>::~Array1D()
{
    (*ref_count_) --;

    if (*ref_count_ < 1)
        destroy_();
}

/* private internal functions */

template <class T>
void Array1D<T>::initialize_(int n)
{


    v_ = new T[n];
    n_ = n;
}

template <class T>
void Array1D<T>::set_(const T& a)
{
    T *begin = &(v_[0]);
    T *end = begin+ n_;

    for (T* p=begin; p<end; p++)
        *p = a;

}

template <class T>
void Array1D<T>::copy_(T* p, const T* q, int len) const
{
    T *end = p + len;
    while (p<end )
        *p++ = *q++;

}

template <class T>
void Array1D<T>::destroy_()
{

    if (v_ != 0)
    {
        delete[] (v_);
    }

    if (ref_count_ != 0)
        delete ref_count_;
}

/**
    @returns location of first element, i.e. A[0] (mutable).
*/
template <class T>
const T* Array1D<T>::begin_() const { return &(v_[0]); }

/**
    @returns location of first element, i.e. A[0] (mutable).
*/
template <class T>
T* Array1D<T>::begin_() { return &(v_[0]); }




} /* namespace TNT */

#endif
/* TNT_ARRAY1D_H */
