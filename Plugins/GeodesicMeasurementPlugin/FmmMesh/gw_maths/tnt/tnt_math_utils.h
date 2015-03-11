#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <math.h>
/* needed for sqrt() below */

namespace TNT
{
/**
    @returns hypotenuse of real (non-complex) scalars a and b by
    avoiding underflow/overflow
    using (a * sqrt( 1 + (b/a) * (b/a))), rather than
    sqrt(a*a + b*b).
*/
template <class Real>
Real hypot(const Real &a, const Real &b)
{

    if (a== 0)
        return abs(b);
    else
    {
        Real c = b/a;
        return a * sqrt(1 + c*c);
    }
}

/**
    @returns the minimum of scalars a and b.
*/
#undef min
template <class Scalar>
Scalar min(const Scalar &a, const Scalar &b)
{
    return  a < b ? a : b;
}

/**
    @returns the maximum of scalars a and b.
*/
#undef max
template <class Scalar>
Scalar max(const Scalar &a, const Scalar &b)
{
    return  a > b ? a : b;
}

/* added by G.Peyre */
#define        TNT_MIN(a, b)       ((a) < (b) ? (a) : (b))            //!<    Returns the min value between a and b
#define        TNT_MAX(a, b)       ((a) > (b) ? (a) : (b))            //!<    Returns the max value between a and b
#define        TNT_ABS(a)       ((a) > 0 ? (a) : -(a))            //!<    Returns the absolute value a


/**
    @returns the absolute value of a real (no-complex) scalar.
*/
template <class Real>
Real abs(const Real &a)
{
    return  (a > 0 ? a : -a);
}

}
#endif
/* MATH_UTILS_H */
