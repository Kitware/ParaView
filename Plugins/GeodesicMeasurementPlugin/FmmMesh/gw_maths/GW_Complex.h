
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Complex.h
 *  \brief  Definition of class \c GW_Complex
 *  \author Gabriel Peyré
 *  \date   6-11-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_COMPLEX_H_
#define _GW_COMPLEX_H_

#include "GW_MathsConfig.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Complex
 *  \brief  A complex class.
 *  \author Gabriel Peyré
 *  \date   6-11-2003
 *
 *  Arithmetics on complex fields.
 */
/*------------------------------------------------------------------------------*/
template<class T>
class GW_Complex: public std::complex<T>
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_Complex(T& a, T& b):std::complex<T>(a,b) {}
    GW_Complex(T& a):std::complex<T>(a,a) {}
    GW_Complex():std::complex<T>(0,0) {}
    //@}

    GW_Complex& operator=(const T& b)
    {
        this->real(b);
        this->imag(b);
        return *this;
    }

    T SquareModulus() const
    { return this->real()*this->real()+this->imag()*this->imag(); }
    T Modulus() const
    { return ::sqrt(this->SquareModulus()); }

    GW_Complex Conjugate() const
    {
        return GW_Complex(this->real(),-this->imag());
    }


    /** Complex class operators */
    GW_Complex operator/(const GW_Complex& b) const
    {
        GW_Float m = b.Modulus();
        if( m!=0 )
            return (*this)*b.Conjugate()/m;
    }
    void operator/(const GW_Complex& a)
    {
        (*this) = (*this)/a;
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_MatrixStatic", s);
        GW_Complex<GW_Float> x(1,1),y(3,2), z;
        z = x/y;
        z *= y;
        GW_ASSERT( (z-x).Modulus()<GW_EPSILON );
        TestClassFooter("GW_MatrixStatic", s);
    }

private:

};

#if 0
template<>
struct gw_basic_type_traits< GW_Complex<GW_Float> >
{
    static double SquareModulus(const GW_Complex<GW_Float>& val)
    { return (double) val.SquareModulus(); }
    static double Modulus(const GW_Complex<GW_Float>& val)
    { return (double) val.Modulus(); }
    /** must return a number at random between say [0,1] */
    static GW_Complex<GW_Float> Random(const GW_Complex<GW_Float>& min, const GW_Complex<GW_Float>& max)
    {
        GW_Float r = GW_RAND_RANGE(min.real(),max.real());
        GW_Float i = GW_RAND_RANGE(min.imag(),max.imag());
        return  GW_Complex<GW_Float>( r, i );
    }
    static const char* GetBasicTypeName()
    { return "complex"; }
    static GW_Bool IsInvertible(const GW_Complex<GW_Float>& x)
    { return (x.real()!=0 && x.imag()!=0); }
};
#endif

/** Pretty print. */
template< class c_type >
inline
std::ostream& operator<<(std::ostream &s, GW_Complex<c_type>& c)
{
    s << c.real() << "+i*" << c.imag();
    return s;
}

} // End namespace GW


#endif // _GW_COMPLEX_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
