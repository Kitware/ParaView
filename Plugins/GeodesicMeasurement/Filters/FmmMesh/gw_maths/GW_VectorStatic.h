/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_VectorStatic.h
 *  \brief Definition of class \c GW_VectorStatic
 *  \author Gabriel Peyré 2001-09-10
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_VectorStatic_h
#define GW_VectorStatic_h

#include "GW_MathsConfig.h"
#include <string.h>
// #include "GW_VectorND.h"// here


GW_BEGIN_NAMESPACE

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_VectorStatic
 *  \brief  A vector of fixed size, with useful operators and methods.
 *  \author Gabriel Peyré 2001-09-10
 *
 *    This class is used every where ...
 *    A lot of operator have been defined to make common maths operations, so use them !
 */
/*------------------------------------------------------------------------------*/

template<GW_U32 v_size, class v_type>
class GW_VectorStatic
{

public:

    //-------------------------------------------------------------------------
    /** \name constructors/destructor  */
    //-------------------------------------------------------------------------
    //@{
    GW_VectorStatic()
    {
        for( GW_U32 i=0; i<v_size; ++i )
            aCoords_[i] = 0;
    }
    GW_VectorStatic( const GW_VectorStatic& v )
    {
        memcpy( aCoords_, v.aCoords_, v_size*sizeof(v_type) );
    }
    GW_VectorStatic( v_type a )
    {
        for( GW_U32 i=0; i<v_size; ++i )
            aCoords_[i] = a;
    }
    GW_VectorStatic( v_type* pa )
    {
        memcpy( aCoords_, pa, v_size*sizeof(v_type) );
    }
    virtual ~GW_VectorStatic(){}
    /** copy operator */
    GW_VectorStatic& operator=(const GW_VectorStatic& v)
    {
        memcpy( aCoords_, v.aCoords_, v_size*sizeof(v_type) );
        return *this;
    }
    //@}

    /** Substraction
        \return The vector this minus the vector \a V
        \param V Vector to substract
    **/
    GW_VectorStatic operator-( const GW_VectorStatic& V) const
    {
        GW_VectorStatic<v_size,v_type> v1;
        for( GW_U32 i=0; i<v_size; ++i )
            v1[i] = aCoords_[i] - V[i];
        return v1;
    }
    /** Addition
        \return The vector this plus the vector \a V
        \param V Vector to add
    **/
     GW_VectorStatic operator+( const GW_VectorStatic& V) const
    {
        GW_VectorStatic<v_size,v_type> v1;
        for( GW_U32 i=0; i<v_size; ++i )
            v1[i] = aCoords_[i] + V[i];
        return v1;
    }
    /** Inversion
        \return The opposed of the vector
    **/
    GW_VectorStatic operator-() const
    {
        GW_VectorStatic<v_size,v_type> v1;
        for( GW_U32 i=0; i<v_size; ++i )
            v1[i] = -aCoords_[i];
        return v1;
    }
    /** Dot Product
        \return The dot product between this and \a V
        \param V second vector of the dot product
    */
    v_type operator*( const GW_VectorStatic& V) const
    {
        v_type r = 0;
        for( GW_U32 i=0; i<v_size; ++i )
            r += aCoords_[i]*V.aCoords_[i];
        return r;
    }
    /** Length
        \return The length of the vector
    **/
    GW_Float operator~() const
    {
        return (GW_Float) ::sqrt(this->SquareNorm());
    }
    /** Multiplication
        \return The vector this multiplied by \a f
        \param f The 'v_type' to multiply the vector by.
    **/
    GW_VectorStatic operator*( v_type f ) const
    {
        GW_VectorStatic<v_size,v_type> v1;
        for( GW_U32 i=0; i<v_size; ++i )
            v1[i] = aCoords_[i]*f;
        return v1;
    }
    /** Normalisation
        \return The vector normalised (i.e. with a length of 1)
    **/
    GW_VectorStatic operator!() const
    {
        GW_Float n = this->Norm();
        GW_Float d;
        if( n!=0 ) d=1/n;
        else d=0;
        return (*this)*( (v_type) d);
    }

    /** Comparison
        \return false if the 2 vectors are the same (with an error of \a EPSILON)
        \param V The vector to compare to
    **/
    /*
     GW_Bool operator!=( const GW_VectorStatic& V) const
    {
        for( int i=0; i<v_size )
            if( fabs(aCoords_[i]-V.aCoords_[i])>GW_EPSILON ) return true;
        return false;
    }
    */

    /** Comparison
        \return true if the 2 vectors are the same (with an error of \a EPSILON)
        \param V The vector to compare to
    **/
    /*
     GW_Bool operator==( const GW_VectorStatic& V) const
    {
        for( GW_U32 i=0; i<v_size )
            if( fabs(aCoords_[i]-V.aCoords_[i])>GW_EPSILON ) return false;
        return false;
    }
    */

    /** Division
        Divide the vector by \a f
        \param f The 'v_type' to divide the vector by
    **/
    void operator/=(v_type f)
    {
        if( f!=0 )
            (*this) *= (v_type) 1/f;
    }

    /** Division
        \return The vector divided by \a f
        \param f The 'v_type' to divide the vector by
    **/
    GW_VectorStatic    operator/(v_type f) const
    {
        if( f!=0 )
        {
            GW_VectorStatic v1(*this);
            v1 /= f;
            return v1;
        }
        else
            return GW_VectorStatic<v_size,v_type>();
    }

    /** Mulitplication
        Mulitply the vector by \a f
        \param f The 'v_type' to multiply the vector by
    **/
    void operator*=(v_type f)
    {
        for( GW_U32 i=0; i<v_size; ++i )
            aCoords_[i] *= f;
    }

    /** Addition
        Add the vector \a V to the vector this
        \param V The vector to add
    **/
    void operator+=( const GW_VectorStatic& V)
    {
        for( GW_U32 i=0; i<v_size; ++i )
            aCoords_[i] += V.aCoords_[i];
    }

    /** Substraction
        Substract the vector \a V to the vector this
        \param V The vector to substract
    **/
    void operator-=( const GW_VectorStatic& V)
    {
        for( GW_U32 i=0; i<v_size; ++i )
            aCoords_[i] -= V.aCoords_[i];
    }

    /**    \return The specified coordinate of the vector. As it is a reference, you can modify it
        \param i The coordinate to retrieve (0=x, 1=y, 2=z)
    **/
    v_type& operator[](GW_U32 i)        //< coordonées
    {
        GW_ASSERT( i<v_size );
        return aCoords_[i];
    }

    /** \return The specified coordinate of the vector. As it is a const reference, you cannot modify it
        \param i The coordinate to retrieve (0=x, 1=y, 2=z)
    **/
    const v_type& operator[](GW_U32 i) const        //< coordonées
    {
        GW_ASSERT( i<v_size );
        return aCoords_[i];
    }

    /** Normalizes the vector (length = 1) */
    void Normalize()
    {
        GW_Float n = this->Norm();
        if( n<GW_EPSILON )
        {
            this->SetZero();
            aCoords_[0] = 1;
        }
        else
            (*this) /= n;
    }

    GW_Float Norm() const
    {
        return ~(*this);
    }

    /** compute the square of the norm */
    v_type SquareNorm() const
    {
        v_type r = 0;
        for( GW_U32 i=0; i<v_size; ++i )
            r += aCoords_[i]*aCoords_[i];
        return r;
    }

    /** return a pointer on the vector datas */
    const v_type* GetCoord(void) const
    {
        return aCoords_;
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_VectorStatic::Interpolate
    *
    *  interpolate between two vectors.
    */
    /*------------------------------------------------------------------------------*/
    void Interpolate(const GW_VectorStatic& p1, const GW_VectorStatic& p2,
                        GW_Float valp1, GW_Float valp2, GW_Float IsoLevel,
                        GW_VectorStatic& newvertex )
    {
        GW_Float mu;
        if( GW_ABS(IsoLevel-valp1) < GW_EPSILON )
        {
            newvertex = p1;
            return;
        }
        if( GW_ABS(IsoLevel-valp2) < GW_EPSILON )
        {
            newvertex = p2;
            return;
        }
        if( GW_ABS(valp1-valp2) < GW_EPSILON )
        {
            newvertex = p1;
            return;
        }
        mu = (IsoLevel - valp1) / (valp2 - valp1);
        newvertex = p1 + (p2 - p1)*mu;
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_VectorStatic::Rotate
    /**
    *  \param  a [GW_Float] Angle
    *  \return [GW_VectorStatic] Rotated vector.
    *  \author Gabriel Peyré
    *  \date   5-26-2003
    *
    *  Return a rotated vector.
    */
    /*------------------------------------------------------------------------------*/
    GW_VectorStatic Rotate( GW_Float a, GW_U32 c1, GW_U32 c2 )
    {
        GW_ASSERT( c1<v_size );
        GW_ASSERT( c2<v_size );
        GW_VectorStatic v;
        v[c1] = (v_type) cos(a)*aCoords_[c1] - sin(a)*aCoords_[c2];
        v[c2] = (v_type) sin(a)*aCoords_[c1] + cos(a)*aCoords_[c2];
        return v;
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Vector::SetCoord
    *
    *  \param  C array of 2 floats [the 2 corrdonates]
    *  \author Gabriel Peyré 2001-09-10
    */
    /*------------------------------------------------------------------------------*/
    void SetCoord(v_type *c)
    {
        memcpy(aCoords_,c,sizeof(v_type)*v_size);
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Vector::SetCoord
    *
    *  \param  xy value of the 2 coordinates.
    *  \author Gabriel Peyré 2001-09-10
    *
    *    Assign the same value to each coordinate.
    */
    /*------------------------------------------------------------------------------*/
    void SetCoord(v_type a)
    {
        for( GW_U32 i=0; i<v_size; ++i )
            aCoords_[i] = a;
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GetData
    *
    *  \return The array of 'v_type' of the vector.
    *  \author Gabriel Peyré 2001-09-29
    */
    /*------------------------------------------------------------------------------*/
    v_type* GetData() const
    {
        return (v_type*) aCoords_;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_VectorStatic::GetData
    *
    *  \param  i offset in the vector.
    *  \return the value of the given coord.
    *  \author Gabriel Peyré 2001-09-29
    */
    /*------------------------------------------------------------------------------*/
    v_type GetData(GW_I32 i) const
    {
        GW_ASSERT( i<v_size );
        return aCoords_[i];
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_VectorStatic::SetData
    *
    *  \param  i offset in the vector.
    *  \param  rVal value to set to this coord.
    *  \author Gabriel Peyré 2001-09-29
    */
    /*------------------------------------------------------------------------------*/
    void SetData(GW_I32 i, v_type rVal)
    {
        GW_ASSERT( static_cast<v_type>(i)<v_size );
        aCoords_[i] = rVal;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_VectorStatic::SetZero
    *
    *  \author Gabriel Peyré 2001-09-29
    *
    *    set the vector to zero.
    */
    /*------------------------------------------------------------------------------*/
    void SetZero()
    {
        memset( this->GetData(), 0, v_size*sizeof(v_type) );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_VectorStatic::SetValue
    *
    *  \param  rVal the value to set to the vector.
    *  \author Gabriel Peyré 2001-09-29
    */
    /*------------------------------------------------------------------------------*/
    void SetValue(v_type rVal)
    {
        this->SetCoord(rVal);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_VectorStatic::Randomize
    *
    *  \param  rMin=0 minimum value.
    *  \param  rMax=1 maximum value.
    *  \author Gabriel Peyré 2001-09-29
    */
    /*------------------------------------------------------------------------------*/
    void Randomize(v_type rMin = 0, v_type rMax = 1)
    {
        for( GW_U32 i=0; i<v_size; ++i )
            this->SetData( i, (v_type) (GW_RAND_RANGE( rMin, rMax )) );
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_VectorStatic", s);
        cout << "Float based vectors ********************" << endl;
        cout << "Test for creation :" << endl;
        GW_VectorStatic<v_size,GW_Float> v;
        v.SetData(0,1);
        v.SetData(1,1);
        cout << v << endl;
        cout << "Test for norm : " << endl;
        cout << v.Norm() << endl;
        cout << "Test for scale : " << endl;
        cout << "v*2 : " << v*2 << " ... norm=" << (v*2).Norm() << "." << endl;
        cout << "Test for rotation : " << endl;
        GW_U32 n_rot = 8;
        for( GW_U32 i=0; i<=n_rot; ++i )
        {
            GW_Float a = GW_TWOPI/n_rot*i;
            cout << "Angle " << a << " : " << v.Rotate(a, 0,1 ) << endl;
        }
        cout << "Test for soustraction : v-2*v" << endl;
        cout << (v-v*2) << endl;
        cout << "Test for addition : v+2*v" << endl;
        cout << (v+v*2) << endl;
        cout << "done." << endl;

        cout << "Integer based vectors ********************" << endl;
        cout << "Test for creation (entry in [0,4]) :" << endl;
        GW_VectorStatic<v_size,GW_I32> vi;
        vi.Randomize(0,5);
        cout << vi << endl;
        cout << "Square norm = " << vi.SquareNorm() << endl;
        cout << "Addition v+2*v : " << (vi+vi*2) << endl;
        TestClassFooter("GW_VectorStatic", s);
    }


protected:

    /** Space coords (x,y) of the vector  */
    v_type        aCoords_[v_size];

};


template<GW_U32 v_size, class v_type>
inline
std::ostream& operator<<(std::ostream &s, GW_VectorStatic<v_size,v_type> &v)
{
    s << "GW_Vector" << v_size << "D : |";
    for( GW_U32 i=0; i<v_size-1; ++i )
        s << v[i] << " ";
    s << v[v_size-1] << "|";
    return s;
}

GW_END_NAMESPACE


#endif // GW_VectorStatic_h

///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
