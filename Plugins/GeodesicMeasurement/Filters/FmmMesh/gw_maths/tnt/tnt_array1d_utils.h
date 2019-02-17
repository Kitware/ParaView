
#ifndef TNT_ARRAY1D_UTILS_H
#define TNT_ARRAY1D_UTILS_H

#include <cstdlib>
#include <cassert>

namespace TNT
{


/**
    Write an array to a character outstream.  Output format is one that can
    be read back in via the in-stream operator: one integer
    denoting the array dimension (n), followed by n elements,
    one per line.

*/
template <class T>
std::ostream& operator<<(std::ostream &s, const Array1D<T> &A)
{
    int N=A.dim1();
    s << "GW_VectorND, size " << N << endl;
    s << "|";
    for (int j=0; j<N; j++)
    {
       s << A[j];
       if( j!=N-1 )
            s << " ";
    }
    s << "|" << endl;

    return s;
}

/**
    Read an array from a character stream.  Input format
    is one integer, denoting the dimension (n), followed
    by n whitespace-separated elements.  Newlines are ignored

    <p>
    Note: the array being read into references new memory
    storage. If the intent is to fill an existing conformant
    array, use <code> cin >> B;  A.inject(B) ); </code>
    instead or read the elements in one-a-time by hand.

    @param s the character to read from (typically <code>std::in</code>)
    @param A the array to read into.
*/
template <class T>
std::istream& operator>>(std::istream &s, Array1D<T> &A)
{
    int N;
    s >> N;

    Array1D<T> B(N);
    for (int i=0; i<N; i++)
        s >> B[i];
    A = B;
    return s;
}



} // namespace TNT

#endif
