/*
*
* Template Numerical Toolkit (TNT): Three-dimensional Fortran numerical array
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



#ifndef TNT_FORTRAN_ARRAY3D_H
#define TNT_FORTRAN_ARRAY3D_H

#include <cstdlib>
#include <iostream>
#ifdef TNT_BOUNDS_CHECK
#include <assert.h>
#endif
#include "tnt_array3d.h"

namespace TNT
{

/**
    Templated three-dimensional, numerical array which
    looks like a conventional Fortran array.  This is
    useful when integrating with C/C++ codes translated
    from Fortran.  Storage corresponds to conventional Fortran ordering.
    That is, the left-most dimension varies fastest.
    Indexing is via the A(i,j,k) notation and A(1,1,1)
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
class Fortran_Array3D
{


  private:


          /* For an (mxnxk) array, we internally keep an (kxnxm) "transposed"
           C array and access A(i,j,k) as A[k-1][j-1][i-1].  The "-1" operation
           is inlined in a tight loop and should be easily optimized.
        */
          Array3D<T> A_;

  public:

    typedef         T   value_type;

           Fortran_Array3D();
           Fortran_Array3D(int m, int n, int k);
           Fortran_Array3D(int m, int n, int k,  T *a);
           Fortran_Array3D(int m, int n, int k, const T &a);
    inline Fortran_Array3D(const Fortran_Array3D &A);
    inline Fortran_Array3D & operator=(const T &a);
    inline Fortran_Array3D & operator=(const Fortran_Array3D &A);
    inline Fortran_Array3D & ref(const Fortran_Array3D &A);
           Fortran_Array3D copy();
           Fortran_Array3D & inject(const Fortran_Array3D & A);
    inline T& operator()(int i, int j, int k);
    inline const T& operator()(int i, int j, int k) const ;
    inline int dim1() const;
    inline int dim2() const;
    inline int dim3() const;
    inline int ref_count() const;
               ~Fortran_Array3D();


};

/**
    Create a null (0x0x0) array.
*/
template <class T>
Fortran_Array3D<T>::Fortran_Array3D() : A_() {}


/**
    Copy constructor. Array data is NOT copied, but shared.
    Thus, in Fortran_Array3D B(A), subsequent changes to A will
    be reflected in B.  For an indepent copy of A, use
    Fortran_Array3D B(A.copy()), or B = A.copy(), instead.
*/
template <class T>
Fortran_Array3D<T>::Fortran_Array3D(const Fortran_Array3D<T> &A) : A_(A.A_) {}



/**
    Create a new (m x n x k) array, WITHOUT initializing array elements.
    To create an initialized array of constants, see Fortran_Array3D(m,n,k, value).

    <p>
    This version avoids the O(m*n*k) initialization overhead and
    is used just before manual assignment.

    @param m the first dimension of the new matrix.
    @param n the second dimension of the new matrix.
    @param k the third dimension of the new matrix.
*/
template <class T>
Fortran_Array3D<T>::Fortran_Array3D(int m, int n, int k) : A_(k,n,m) {}



/**
    Create a new (m x n x k) array,  initializing array elements to
    constant specified by argument.  Most often used to
    create an array of zeros, as in A(m, n, k, 0.0).

    @param m the first dimension of the new matrix.
    @param n the second dimension of the new matrix.
    @param k the third dimension of the new matrix.
    @param val the constant value to set all elements of the new array to.
*/
template <class T>
Fortran_Array3D<T>::Fortran_Array3D(int m, int n, int k, const T &val) :
    A_(k, n, m, val) {}


/**
    Create a new (m x n x k) array,  as a view of an existing one-dimensional
    C array, with elements stored in <b>Fortran order</b>, i.e. right-most dimension
    varying fastest. (Often referred to as "column-major" ordering.)
    Note that the storage for this pre-existing array will
    never be garbage collected by the Fortran_Array3D class.

    @param m the first dimension of the new matrix.
    @param n the second dimension of the new matrix.
    @param k the third dimension of the new matrix.
    @param a the one dimensional C array to use as data storage for
        the array.
*/
template <class T>
Fortran_Array3D<T>::Fortran_Array3D(int m, int n, int k, T *a) :
    A_(k, n, m, a) {}




/**
    Elements are accessed via A(i,j,k) indexing.

    If TNT_BOUNDS_CHECK macro is defined, the indices
    are checked that they falls within the array bounds (via the
    assert() macro.)
*/
template <class T>
inline T& Fortran_Array3D<T>::operator()(int i, int j, int k)
{
#ifdef TNT_BOUNDS_CHECK
    assert(k >= 1);
    assert(k <= A_.dim1());
    assert(j >= 1);
    assert(j <= A_.dim2());
    assert(i >= 1);
    assert(i <= A_.dim3());
#endif

    return A_[k-1][j-1][i-1];

}

/**
    Read-only version of A(i,j,k) indexing.

    If TNT_BOUNDS_CHECK macro is defined, the indices
    are checked that they falls within the array bounds (via the
    assert() macro.)
*/
template <class T>
inline const T& Fortran_Array3D<T>::operator()(int i, int j, int k)  const
{
#ifdef TNT_BOUNDS_CHECK
    assert(k >= 1);
    assert(k <= A_.dim1());
    assert(j >= 1);
    assert(j <= A_.dim2());
    assert(i >= 1);
    assert(i <= A_.dim3());
#endif

    return A_[k-1][j-1][i-1];

}


/**
    Assign all elemnts of A to a constant scalar.
*/
template <class T>
Fortran_Array3D<T> & Fortran_Array3D<T>::operator=(const T &a)
{
    A_ = a;
    return *this;
}
/**
    Create a new of existing matrix.  Used in B = A.copy()
    or in the construction of B, e.g. Fortran_Array3D B(A.copy()),
    to create a new array that does not share data.

*/
template <class T>
Fortran_Array3D<T> Fortran_Array3D<T>::copy()
{

    Fortran_Array3D B;

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
    Fortran_Array3D A(m,n,k);
    Fortran_Array3D C(m,n,k);
    Fortran_Array3D B(C);        // elements of B and C are shared.

</pre>
    then B.inject(A) affects both and C, while B=A.copy() creates
    a new array B which shares no data with C or A.

    @param A the array from elements will be copied
    @return an instance of the modified array. That is, in B.inject(A),
    it returns B.  If A and B are not conformat, no modifications to
    B are made.

*/
template <class T>
Fortran_Array3D<T> & Fortran_Array3D<T>::inject(const Fortran_Array3D &A)
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
Fortran_Array3D<T> & Fortran_Array3D<T>::ref(const Fortran_Array3D<T> &A)
{
    A_.ref(A.A_);
    return *this;
}

/**
    B = A is shorthand notation for B.ref(A).
*/
template <class T>
Fortran_Array3D<T> & Fortran_Array3D<T>::operator=(const Fortran_Array3D<T> &A)
{
    return ref(A);
}

/**
    @return the size of the first dimension of the array.
*/
template <class T>
inline int Fortran_Array3D<T>::dim1() const { return A_.dim3(); }

/**
    @return the size of the second dimension of the array.
*/
template <class T>
inline int Fortran_Array3D<T>::dim2() const { return A_.dim2(); }

/**
    @return the size of the third (right-most) dimension of the array.
*/
template <class T>
inline int Fortran_Array3D<T>::dim3() const { return A_.dim1(); }


/**
    @return the number of arrays that share the same storage area
    as this one.  (Must be at least one.)
*/
template <class T>
inline int Fortran_Array3D<T>::ref_count() const { return A_.ref_count(); }

template <class T>
Fortran_Array3D<T>::~Fortran_Array3D()
{
}


} /* namespace TNT */

#endif
/* TNT_FORTRAN_ARRAY3D_H */
