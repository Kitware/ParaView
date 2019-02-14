/*
*
* Template Numerical Toolkit (TNT): Three-dimensional numerical array
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



#ifndef TNT_ARRAY3D_H
#define TNT_ARRAY3D_H

#include <cstdlib>
#include <iostream>
#ifdef TNT_BOUNDS_CHECK
#include <assert.h>
#endif

namespace TNT
{

/**
    Tempplated three-dimensional, numerical array which
    looks like a conventional C multiarray.
    Storage corresponds to conventional C ordering.
    That is the right-most dimension has contiguous
    elements.  Indexing is via the A[i][j][k] notation.

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
    the familiar C[i][j][k] notation.  This includes numerous
    textbooks, such as Numercial Recipes, and various
    public domain codes.

    <p>
    This class employs its own garbage collection via
    the use of reference counts.  That is, whenever
    an internal array storage no longer has any references
    to it, it is destroyed.
*/
template <class T>
class Array3D
{


  private:
    T*** v_;
    int m_;
    int n_;
    int k_;
    int *ref_count_;

    void initialize_(T* data, int m, int n, int k);
    void copy_(T* p, const T*  q, int len) const;
    void set_(const T& val);
    void destroy_();
    inline const T* begin_() const;
    inline T* begin_();

  public:

    typedef         T   value_type;

           Array3D();
           Array3D(int m, int n, int k);
           Array3D(int m, int n, int k,  T *a);
           Array3D(int m, int n, int k, const T &a);
    inline Array3D(const Array3D &A);
    inline Array3D & operator=(const T &a);
    inline Array3D & operator=(const Array3D &A);
    inline Array3D & ref(const Array3D &A);
           Array3D copy() const;
           Array3D & inject(const Array3D & A);
    inline T** operator[](int i);
    inline const T* const * operator[](int i) const;
    inline int dim1() const;
    inline int dim2() const;
    inline int dim3() const;
    inline int ref_count() const;
               ~Array3D();


};


/**
    Copy constructor. Array data is NOT copied, but shared.
    Thus, in Array3D B(A), subsequent changes to A will
    be reflected in B.  For an indepent copy of A, use
    Array3D B(A.copy()), or B = A.copy(), instead.
*/
template <class T>
Array3D<T>::Array3D(const Array3D<T> &A) : v_(A.v_), m_(A.m_),
    n_(A.n_), k_(A.k_), ref_count_(A.ref_count_)
{
    (*ref_count_)++;
}



/**
    Create a new (m x n x k) array, WITHOUT initializing array elements.
    To create an initialized array of constants, see Array3D(m,n,k, value).

    <p>
    This version avoids the O(m*n*k) initialization overhead and
    is used just before manual assignment.

    @param m the first dimension of the new matrix.
    @param n the second dimension of the new matrix.
    @param k the third dimension of the new matrix.
*/
template <class T>
Array3D<T>::Array3D(int m, int n, int k) : v_(0), m_(m), n_(n), k_(k), ref_count_(0)
{
    initialize_(new T[m*n*k], m,n,k);
    ref_count_ = new int;
    *ref_count_ = 1;
}



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
Array3D<T>::Array3D(int m, int n, int k, const T &val) : v_(0), m_(m), n_(n) ,
    k_(k), ref_count_(0)
{
    initialize_(new T[m*n*k], m,n,k);
    set_(val);
    ref_count_ = new int;
    *ref_count_ = 1;

}

/**

    Create a new (m x n x k) array,  as a view of an existing one-dimensional
    array stored in <b>C order</b>, i.e. right-most dimension varying fastest.
    (Often referred to as "row-major" ordering.)
    Note that the storage for this pre-existing array will
    never be garbage collected by the Array3D class.

    @param m the first dimension of the new matrix.
    @param n the second dimension of the new matrix.
    @param k the third dimension of the new matrix.
    @param a the one dimensional C array to use as data storage for
        the array.
*/
template <class T>
Array3D<T>::Array3D(int m, int n, int k, T *a) : v_(0), m_(m), n_(n) ,
    k_(k), ref_count_(0)
{
    initialize_(a, m, n, k);
    ref_count_ = new int;
    *ref_count_ = 2;        /* this avoids destroying original data. */

}


/**
    Used for A[i][j][k] indexing.  The first [] operator returns
    a conventional pointer which can be dereferenced using the
    same [] notation.

    If TNT_BOUNDS_CHECK macro is define, the left-most index
    is checked that it falls within the array bounds (via the
    assert() macro.)
*/
template <class T>
inline T** Array3D<T>::operator[](int i)
{
#ifdef TNT_BOUNDS_CHECK
    assert(i >= 0);
    assert(i < m_);
#endif

return v_[i];

}

template <class T>
inline const T* const * Array3D<T>::operator[](int i) const { return v_[i]; }

/**
    Assign all elemnts of A to a constant scalar.
*/
template <class T>
Array3D<T> & Array3D<T>::operator=(const T &a)
{
    set_(a);
    return *this;
}
/**
    Create a new of existing matrix.  Used in B = A.copy()
    or in the construction of B, e.g. Array3D B(A.copy()),
    to create a new array that does not share data.

*/
template <class T>
Array3D<T> Array3D<T>::copy() const
{
    Array3D A(m_, n_, k_);
    copy_(A.begin_(), begin_(), m_*n_*k_);

    return A;
}


/**
    Copy the elements to from one array to another, in place.
    That is B.inject(A), both A and B must conform (i.e. have
    identical dimensions).

    This differs from B = A.copy() in that references to B
    before this assignment are also affected.  That is, if
    we have
    <pre>
    Array3D A(m,n,k);
    Array3D C(m,n,k);
    Array3D B(C);        // elements of B and C are shared.

</pre>
    then B.inject(A) affects both and C, while B=A.copy() creates
    a new array B which shares no data with C or A.

    @param A the array from elements will be copied
    @return an instance of the modified array. That is, in B.inject(A),
    it returns B.  If A and B are not conformat, no modifications to
    B are made.

*/
template <class T>
Array3D<T> & Array3D<T>::inject(const Array3D &A)
{
    if (A.m_ == m_ &&  A.n_ == n_)
        copy_(begin_(), A.begin_(), m_*n_);

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
Array3D<T> & Array3D<T>::ref(const Array3D<T> &A)
{
    if (this != &A)
    {
        (*ref_count_) --;
        if ( *ref_count_ < 1 )
        {
            destroy_();
        }

        m_ = A.m_;
        n_ = A.n_;
        k_ = A.k_;
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
Array3D<T> & Array3D<T>::operator=(const Array3D<T> &A)
{
    return ref(A);
}

/**
    @return the size of the first dimension of the array.
*/
template <class T>
inline int Array3D<T>::dim1() const { return m_; }

/**
    @return the size of the second dimension of the array.
*/
template <class T>
inline int Array3D<T>::dim2() const { return n_; }

/**
    @return the size of the third (right-most) dimension of the array.
*/
template <class T>
inline int Array3D<T>::dim3() const { return k_; }


/**
    @return the number of arrays that share the same storage area
    as this one.  (Must be at least one.)
*/
template <class T>
inline int Array3D<T>::ref_count() const
{
    return *ref_count_;
}

template <class T>
Array3D<T>::~Array3D()
{
    (*ref_count_) --;

    if (*ref_count_ < 1)
        destroy_();
}

/* private internal functions */

template <class T>
void Array3D<T>::initialize_( T* data, int m, int n, int k)
{

    v_ = new T**[m];
    v_[0] = new T*[m*n];
    v_[0][0] = data;


    for (int i=0; i<m; i++)
    {
        v_[i] = v_[0] + i * n;
        for (int j=0; j<n; j++)
            v_[i][j] = v_[0][0] + i * (n*k) + j * k;
    }

    m_ = m;
    n_ = n;
    k_ = k;
}

template <class T>
void Array3D<T>::set_(const T& a)
{
    T *begin = &(v_[0][0][0]);
    T *end = begin+ m_*n_*k_;

    for (T* p=begin; p<end; p++)
        *p = a;

}

template <class T>
void Array3D<T>::copy_(T* p, const T* q, int len) const
{
    T *end = p + len;
    while (p<end )
        *p++ = *q++;

}

template <class T>
void Array3D<T>::destroy_()
{

    if (v_ != 0)
    {
        delete[] (v_[0][0]);
        delete[] (v_[0]);
        delete[] (v_);
    }

    if (ref_count_ != 0)
        delete ref_count_;
}

/**
    @returns location of first element, i.e. A[0][0][0] (mutable).
*/
template <class T>
const T* Array3D<T>::begin_() const { return &(v_[0][0][0]); }

/**
    @returns location of first element, i.e. A[0][0][0] (mutable).
*/
template <class T>
T* Array3D<T>::begin_() { return &(v_[0][0][0]); }

/**
    Create a null (0x0x0) array.
*/
template <class T>
Array3D<T>::Array3D() : v_(0), m_(0), n_(0)
{
    ref_count_ = new int;
    *ref_count_ = 1;
}

} /* namespace TNT */

#endif
/* TNT_ARRAY3D_H */
