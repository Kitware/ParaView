

#ifndef TNT_FORTRAN_ARRAY2D_UTILS_H
#define TNT_FORTRAN_ARRAY2D_UTILS_H

#include <iostream>

namespace TNT
{


/**
    Write an array to a character outstream.  Output format is one that can
    be read back in via the in-stream operator: two integers
    denoting the array dimensions (m x n), followed by m
    lines of n  elements.

*/
template <class T>
std::ostream& operator<<(std::ostream &s, const Fortran_Array2D<T> &A)
{
    int M=A.dim1();
    int N=A.dim2();

    s << M << " " << N << "\n";

    for (int i=1; i<=M; i++)
    {
        for (int j=1; j<=N; j++)
        {
            s << A(i,j) << " ";
        }
        s << "\n";
    }


    return s;
}

/**
    Read an array from a character stream.  Input format
    is two integers, denoting the dimensions (m x n), followed
    by m*n whitespace-separated elements in "row-major" order
    (i.e. right-most dimension varying fastest.)  Newlines
    are ignored.

    <p>
    Note: the array being read into references new memory
    storage. If the intent is to fill an existing conformant
    array, use <code> cin >> B;  A.inject(B) ); </code>
    instead or read the elements in one-a-time by hand.

    @param s the character to read from (typically <code>std::in</code>)
    @param A the array to read into.
*/
template <class T>
std::istream& operator>>(std::istream &s, Fortran_Array2D<T> &A)
{

    int M, N;

    s >> M >> N;

    Fortran_Array2D<T> B(M,N);

    for (int i=1; i<=M; i++)
        for (int j=1; j<=N; j++)
        {
            s >>  B(i,j);
        }

    A = B;
    return s;
}




} // namespace TNT

#endif
