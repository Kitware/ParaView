

#ifndef TNT_ARRAY2D_UTILS_H
#define TNT_ARRAY2D_UTILS_H

#include <cstdlib>
#include <cassert>

namespace TNT
{


/**
    Write an array to a character outstream.  Output format is one that can
    be read back in via the in-stream operator: two integers
    denoting the array dimensions (m x n), followed by m
    lines of n  elements.

*/
template <class T>
std::ostream& operator<<(std::ostream &s, const Array2D<T> &A)
{
    int M=A.dim1();
    int N=A.dim2();

    s << M << " " << N << "\n";

    for (int i=0; i<M; i++)
    {
        for (int j=0; j<N; j++)
        {
            s << A[i][j] << " ";
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
std::istream& operator>>(std::istream &s, Array2D<T> &A)
{

    int M, N;

    s >> M >> N;

    Array2D<T> B(M,N);

    for (int i=0; i<M; i++)
        for (int j=0; j<N; j++)
        {
            s >>  B[i][j];
        }

    A = B;
    return s;
}


/**
    Matrix Multiply:  compute C = A*B, where C[i][j]
    is the dot-product of row i of A and column j of B.


    @param A an (m x n) array
    @param B an (n x k) array
    @return the (m x k) array A*B, or a null array (0x0)
        if the matrices are non-conformant (i.e. the number
        of columns of A are different than the number of rows of B.)


*/
template <class T>
Array2D<T> matmult(const Array2D<T> &A, const Array2D<T> &B)
{
    if (A.dim2() != B.dim1())
        return Array2D<T>();

    int M = A.dim1();
    int N = A.dim2();
    int K = B.dim2();

    Array2D<T> C(M,K);

    for (int i=0; i<M; i++)
        for (int j=0; j<K; j++)
        {
            T sum = 0;

            for (int k=0; k<N; k++)
                sum += A[i][k] * B [k][j];

            C[i][j] = sum;
        }

    return C;

}



} // namespace TNT

#endif
