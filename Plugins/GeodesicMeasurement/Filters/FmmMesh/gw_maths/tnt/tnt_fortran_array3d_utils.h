

#ifndef TNT_FORTRAN_ARRAY3D_UTILS_H
#define TNT_FORTRAN_ARRAY3D_UTILS_H

#include <cstdlib>
#include <cassert>

namespace TNT
{


/**
    Write an array to a character outstream.  Output format is one that can
    be read back in via the in-stream operator: three integers
    denoting the array dimensions (m x n x k), followed by m
    (n x k) arrays in "row-major" order.

*/
template <class T>
std::ostream& operator<<(std::ostream &s, const Fortran_Array3D<T> &A)
{
    int M=A.dim1();
    int N=A.dim2();
    int K=A.dim3();

    s << M << " " << N << " " << K << "\n";

    for (int i=1; i<=M; i++)
    {
        for (int j=1; j<=N; j++)
        {
            for (int k=1; k<=K; k++)
                s << A(i,j,k) << " ";
            s << "\n";
        }
        s << "\n";
    }


    return s;
}

/**
    Read an array from a character stream.  Input format
    is three integers, denoting the dimensions (m x n x k), followed
    by m*n*k whitespace-separated elements in "row-major" order
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
std::istream& operator>>(std::istream &s, Fortran_Array3D<T> &A)
{

    int M, N, K;

    s >> M >> N >> K;

    Fortran_Array3D<T> B(M,N,K);

    for (int i=1; i<=M; i++)
        for (int j=1; j<=N; j++)
            for (int k=1; k<=K; k++)
                s >>  B(i,j,k);

    A = B;
    return s;
}




} // namespace TNT

#endif
