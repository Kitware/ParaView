/*
*
* Template Numerical Toolkit (TNT): Two-dimensional Fortran numerical array
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



#ifndef TNT_FORTRAN_ARRAY2D_H
#define TNT_FORTRAN_ARRAY2D_H

#include <cstdlib>
#include <iostream>

#ifdef TNT_BOUNDS_CHECK
#include <assert.h>
#endif

#include "tnt_array2d.h"

namespace TNT
{

/**
    Templated two-dimensional, numerical array which
    looks like a conventional Fortran array.  This is
    useful when integrating with C/C++ codes translated
    from Fortran.  Storage corresponds to conventional Fortran ordering.
    That is, the left-most dimension varies fastest.
    Indexing is via the A(i,j) notation and A(1,1)
    is the first element.

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
    This class employs its own garbage collection via
    the use of reference counts.  That is, whenever
    an internal array storage no longer has any references
    to it, it is destroyed.
*/
template <class T>
class Fortran_Array2D
{


  private:
          /* For an (mxn) array, we internally keep an (nxm) "transposed"
           C array and access A(i,j) as A[j-1][i-1].  The "-1" operation
           is inlined in a tight loop and should be easily optimized.
        */
          Array2D<T> A_;

  public:

    typedef         T   value_type;

           Fortran_Array2D();
           Fortran_Array2D(int m, int n);
           Fortran_Array2D(int m, int n,  T *a);
           Fortran_Array2D(int m, int n, const T &a);
    inline Fortran_Array2D(const Fortran_Array2D &A);
    inline Fortran_Array2D & operator=(const T &a);
    inline Fortran_Array2D & operator=(const Fortran_Array2D &A);
    inline Fortran_Array2D & ref(const Fortran_Array2D &A);
           Fortran_Array2D copy();
           Fortran_Array2D & inject(const Fortran_Array2D & A);
    inline T& operator()(int i, int j);
    inline const T& operator()(int i, int j) const ;
    inline int dim1() const;
    inline int dim2() const;
    inline int ref_count() const;
               ~Fortran_Array2D();


};

/**
    Create a null (0x0x0) array.
*/
template <class T>
Fortran_Array2D<T>::Fortran_Array2D() : A_() {}


/**
    Copy constructor. Array data is NOT copied, but shared.
    Thus, in Fortran_Array2D B(A), subsequent changes to A will
    be reflected in B.  For an indepent copy of A, use
    Fortran_Array2D B(A.copy()), or B = A.copy(), instead.
*/
template <class T>
Fortran_Array2D<T>::Fortran_Array2D(const Fortran_Array2D<T> &A) : A_(A.A_) {}



/**
    Create a new (m x n) array, WITHOUT initializing array elements.
    To create an initialized array of constants, see Fortran_Array2D(m,n, value).

    <p>
    This version avoids the O(m*n) initialization overhead and
    is used just before manual assignment.

    @param m the first dimension of the new matrix.
    @param n the second dimension of the new matrix.
*/
template <class T>
Fortran_Array2D<T>::Fortran_Array2D(int m, int n) : A_(n,m) {}



/**
    Create a new (m x n ) array,  initializing array elements to
    constant specified by argument.  Most often used to
    create an array of zeros, as in A(m, n, k, 0.0).

    @param m the first dimension of the new matrix.
    @param n the second dimension of the new matrix.
    @param val the constant value to set all elements of the new array to.
*/
template <class T>
Fortran_Array2D<T>::Fortran_Array2D(int m, int n, const T &val) : A_(n, m, val) {}


/**

    Create a new (m x n) array,  as a view of an existing one-dimensional
    C array, with elements stored in <b>Fortran order</b>, i.e. right-most dimension
    varying fastest. (Often referred to as "column-major" ordering.)
    Note that the storage for this pre-existing array will
    never be garbage collected by the Fortran_Array2D class.


    @param m the first dimension of the new matrix.
    @param n the second dimension of the new matrix.
    @param a the one dimensional C array to use as data storage for
        the array.
*/
template <class T>
Fortran_Array2D<T>::Fortran_Array2D(int m, int n, T *a) : A_(n, m, a) {}




/**
    Elements are accessed via A(i,j) indexing.

    If TNT_BOUNDS_CHECK macro is defined, the indices
    are checked that they falls within the array bounds (via the
    assert() macro.)
*/
template <class T>
inline T& Fortran_Array2D<T>::operator()(int i, int j)
{
#ifdef TNT_BOUNDS_CHECK
    assert(j >= 1);
    assert(j <= A_.dim1());
    assert(i >= 1);
    assert(i <= A_.dim2());
#endif

    return A_[j-1][i-1];

}

/**
    Read-only version of A(i,j) indexing.

    If TNT_BOUNDS_CHECK macro is defined, the indices
    are checked that they falls within the array bounds (via the
    assert() macro.)
*/
template <class T>
inline const T& Fortran_Array2D<T>::operator()(int i, int j)  const
{
#ifdef TNT_BOUNDS_CHECK
    assert(j >= 1);
    assert(j <= A_.dim1());
    assert(i >= 1);
    assert(i <= A_.dim2());
#endif

    return A_[j-1][i-1];

}


/**
    Assign all elemnts of A to a constant scalar.
*/
template <class T>
Fortran_Array2D<T> & Fortran_Array2D<T>::operator=(const T &a)
{
    A_ = a;
    return *this;
}
/**
    Create a new of existing matrix.  Used in B = A.copy()
    or in the construction of B, e.g. Fortran_Array2D B(A.copy()),
    to create a new array that does not share data.

*/
template <class T>
Fortran_Array2D<T> Fortran_Array2D<T>::copy()
{

    Fortran_Array2D B;

    B.A_= A_.copy();
    return B;
}


/**
    Copy the elements to from one array to another, in place.
    That is B.inject(A), both A and B must conform (i.e. have
    identical dimensions).

    This differs from B = A.copy() in that references to B
    before this assignment are also affected.  That is, if
    we have
    <pre>
    Fortran_Array2D A(m,n);
    Fortran_Array2D C(m,n);
    Fortran_Array2D B(C);        // elements of B and C are shared.

</pre>
    then B.inject(A) affects both and C, while B=A.copy() creates
    a new array B which shares no data with C or A.

    @param A the array from elements will be copied
    @return an instance of the modified array. That is, in B.inject(A),
    it returns B.  If A and B are not conformat, no modifications to
    B are made.

*/
template <class T>
Fortran_Array2D<T> & Fortran_Array2D<T>::inject(const Fortran_Array2D &A)
{
    A_.inject(A.A_);

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
Fortran_Array2D<T> & Fortran_Array2D<T>::ref(const Fortran_Array2D<T> &A)
{
    A_.ref(A.A_);
    return *this;
}

/**
    B = A is shorthand notation for B.ref(A).
*/
template <class T>
Fortran_Array2D<T> & Fortran_Array2D<T>::operator=(const Fortran_Array2D<T> &A)
{
    return ref(A);
}

/**
    @return the size of the first dimension of the array.
*/
template <class T>
inline int Fortran_Array2D<T>::dim1() const { return A_.dim2(); }

/**
    @return the size of the second dimension of the array.
*/
template <class T>
inline int Fortran_Array2D<T>::dim2() const { return A_.dim1(); }



/**
    @return the number of arrays that share the same storage area
    as this one.  (Must be at least one.)
*/
template <class T>
inline int Fortran_Array2D<T>::ref_count() const { return A_.ref_count(); }

template <class T>
Fortran_Array2D<T>::~Fortran_Array2D()
{
}




} /* namespace TNT */

#endif
/* TNT_FORTRAN_ARRAY2D_H */
