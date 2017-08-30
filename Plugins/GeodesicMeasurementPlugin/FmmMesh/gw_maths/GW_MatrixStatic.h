/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_MatrixStatic.h
 *  \brief Definition of class \c GW_MatrixStatic
 *  \author Gabriel Peyré 2001-09-18
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_MatrixStatic_h
#define GW_MatrixStatic_h

#include "GW_MathsConfig.h"
#include "GW_MatrixNxP.h"
#include "GW_Complex.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_MatrixStatic
 *  \brief  A matrix of variable dimension.
 *  \author Gabriel Peyré 2001-09-18
 *
 *    Note that the matrix is \e row \e major.
 *
 *    \code
 *    +-----+-----+.....+-------+
 *  | 0,0 | 0,1 |     | 0,n-1 |
 *    +-----+-----+.....+-------+
 *  | 1,0 | 1,2 |     | 1,n-1 |
 *    +-----+-----+.....+-------+
 *  .     .     . i,j .       . <- i
 *    +-----+-----+.....+-------+
 *  |p-1,0|p-1,1|     |p-1,m-1|
 *    +-----+-----+.....+-------+
 *                 ^
 *                 |
 *                 j
 *  offset = j+i*n
 *    \endcode
 */
/*------------------------------------------------------------------------------*/

template< GW_U32 r_size, GW_U32 c_size, class v_type, class traits = gw_basic_type_traits<v_type> >
class GW_MatrixStatic
{

    /** Traits parameterization of the class. */
    typedef traits traits_type;
    double Epsilon()
    { return traits_type::Epsilon(); }

    /** To iterate some code on the data. */
    #define ITERATE(i,j, code )            \
        for(GW_U32 i=0; i<r_size; ++i)    \
        for(GW_U32 j=0; j<c_size; ++j)    \
        { code }

public:

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic constructor
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Base constructor, load identity.
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixStatic()
    {
        this->LoadIdentity();
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic constructor
    *
    *  \param  m original matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixStatic(const GW_MatrixStatic& m)
    {
        memcpy(aData_, m.GetData(), this->GetMemorySize() );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : copy operator
    *
    *  \param  m original matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixStatic& operator = (const GW_MatrixStatic &m)
    {
        memcpy(aData_, m.GetData(), this->GetMemorySize() );
        return *this;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic destructor
    *
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    virtual ~GW_MatrixStatic()
    {
        /* NOTHING */
    }

    static GW_U32 GetMemorySize()
    {
        return r_size*c_size*sizeof(v_type);
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Multiply
    *
    *  \param  a right hand matrix.
    *  \return vector transformed by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_VectorStatic<r_size,v_type> operator*(const GW_VectorStatic<c_size,v_type>& v) const
    {
        GW_VectorStatic<r_size,v_type> r;
        GW_MatrixStatic::Multiply( *this, v, r );
        return r;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Multiply
    *
    *  \param  a right hand matrix.
    *  \param  v left hand vector.
    *  \param  r vector transformed by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    static void Multiply(const GW_MatrixStatic& a, const GW_VectorStatic<c_size,v_type>& v, GW_VectorStatic<r_size,v_type>& r)
    {
        v_type rTemp =0;
        for(GW_U32 i=0; i<c_size; ++i)
        {
            rTemp =0;
            for(GW_U32 k=0; k<r_size; ++k)
                rTemp += a.GetData(i,k)*v.GetData(k);
            r.SetData(i, rTemp);
        }
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Scale
    *
    *  \param  rScale factor
    *  \param  a left hand matrix
    *  \param  r right hand matrix
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    static void Scale(const v_type rScale, const GW_MatrixStatic<r_size,c_size,v_type>& a, GW_MatrixStatic<r_size,c_size,v_type>& r)
    {
        ITERATE(i,j, r.SetData(i,j, a.GetData(i,j)*rScale); );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::AutoScale
    *
    *  \param  rFactor scaling factor.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void AutoScale(v_type rFactor)
    {
        GW_MatrixStatic::Scale( rFactor, *this, *this );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::operator*
    *
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixStatic operator*(v_type v) const
    {
        GW_MatrixStatic r;
        GW_MatrixStatic::Scale( v, *this, r );
        return r;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Scale
    *
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixStatic operator/(v_type v) const
    {
        GW_ASSERT(v!=0);
        if( v==0 )
            return;
        GW_MatrixStatic r;
        GW_MatrixStatic::Scale( 1/v, *this, r );
        return r;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Scale
    *
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void operator*=(v_type v)
    {
        GW_MatrixStatic::Scale( v, *this, *this );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Scale
    *
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void operator/=(v_type v)
    {
        GW_ASSERT( v!=0 );
        if( v==0 )
            return;
        GW_MatrixStatic::Scale( 1/v, *this, *this );
    }



    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Invert
    *
    *  \param  a matrix to invert
    *  \param  r result.
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Use the LU factorisation to solve the system.
    */
    /*------------------------------------------------------------------------------*/
    static void Invert(const GW_MatrixStatic& a, GW_MatrixStatic& r)
    {
        GW_ASSERT( c_size==r_size );
        GW_MatrixNxP M(c_size,c_size);
        GW_Maths::Convert( M, a );
        GW_VectorND b(c_size);
        GW_VectorND x(c_size);
        GW_VectorStatic<c_size,GW_Float> xstatic;
        for( GW_U32 i=0; i<c_size; ++i )
        {
            b.SetZero();
            b[i] = 1;
            cout << M << endl;
            cout << b << endl;
            M.LUSolve( x, b );
            cout << x << endl;
            GW_Maths::Convert(xstatic, x);
            r.SetColumn( xstatic, i );
        }
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Invert
    *
    *  \return the inverse of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixStatic Invert() const
    {
        GW_MatrixStatic r;
        GW_MatrixStatic::Invert(*this, r);
        return r;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::AutoInvert
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Invert the matrix.
    */
    /*------------------------------------------------------------------------------*/
    void AutoInvert()
    {
        GW_MatrixStatic r;        // needs temporary data
        GW_MatrixStatic::Invert(*this, r);
        this->SetData( r.GetData() );
    }


    /** Solve the linear system A*x=b */
    void Solve( GW_VectorStatic<c_size,v_type>& x, const GW_VectorStatic<r_size,v_type>& b ) const
    {
        GW_MatrixNxP M(c_size,r_size);
        GW_Maths::Convert( M, *this );
        GW_VectorND bdynamic(c_size);
        GW_Maths::Convert( bdynamic, b );
        GW_VectorND xdynamic(c_size);

        M.LUSolve( xdynamic, bdynamic );

        GW_Maths::Convert(x, xdynamic);
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Transpose
    *
    *  \param  a original matrix.
    *  \param  r result.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    static void Transpose(const GW_MatrixStatic<r_size,c_size,v_type>& a, GW_MatrixStatic<c_size,r_size,v_type>& r)
    {
        ITERATE(i,j, r.SetData(j,i, a.GetData(i,j)); )
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::AutoTranspose
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Transpose the matrix. Must be square !
    */
    /*------------------------------------------------------------------------------*/
    void AutoTranspose()
    {
        GW_ASSERT( r_size==c_size );
        GW_MatrixStatic<c_size,c_size> r;
        GW_MatrixStatic::Transpose(*this, r);
        (*this) = r;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Transpose
    *
    *  \return transoped matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_MatrixStatic<c_size,r_size,v_type> Transpose() const
    {
        GW_MatrixStatic<c_size,r_size,v_type> r;
        GW_MatrixStatic::Transpose(*this, r);
        return r;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::GetData
    *
    *  \return the data of the matrix (v_type[c_size*r_size]).
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    const v_type* GetData() const
    {
        return (v_type*) aData_;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::GetData
    *
    *  \return the data of the matrix (v_type[c_size*r_size]).
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    v_type* GetData()
    {
        return (v_type*) aData_;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::GetData
    *
    *  \param  i row number.
    *  \param  j col number.
    *  \return value of the matrix at place (i,j)
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Get an element of the matrix.
    */
    /*------------------------------------------------------------------------------*/
    v_type GetData(GW_I32 i, GW_I32 j) const
    {
        GW_ASSERT(i<r_size && j<c_size);
        return aData_[i][j];
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::SetData
    *
    *  \param  i row number.
    *  \param  j col number.
    *  \param  rVal value of the matrix at place (i,j)
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetData(GW_U32 i, GW_U32 j, v_type rVal)
    {
        GW_ASSERT(i<r_size && j<c_size);
        aData_[i][j] = rVal;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::AddData
    *
    *  \param  i row number.
    *  \param  j col number.
    *  \param  rVal value of the matrix at place (i,j)
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void AddData(GW_I32 i, GW_I32 j, v_type rVal)
    {
        GW_ASSERT(i<r_size && j<c_size);
        aData_[i][j] += rVal;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::SetData
    *
    *  \param  aData a pointer on an array of c_size*r_size 'v_type'
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetData(v_type* aData)
    {
        memcpy(aData_, aData, this->GetMemorySize() );
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::GetRow
    *
    *  \return Row vector of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_VectorStatic<c_size,v_type> GetRow( GW_U32 i ) const
    {
        GW_ASSERT(i<r_size);
        GW_VectorStatic<c_size,v_type> v;
        for( GW_U32 j=0; j<c_size; ++j )
            v[j] = this->GetData(i,j);
        return v;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::GetColumn
    *
    *  \return Column vector of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_VectorStatic<r_size,v_type> GetColumn( GW_U32 j ) const
    {
        GW_ASSERT( j<c_size );
        GW_VectorStatic<r_size,v_type> v;
        for( GW_U32 i=0; i<r_size; ++i )
            v[i] = this->GetData(i,j);
        return v;
    }

    /** access a row in raw format, use it at your own risk (i.e. M[i][j]) */
    v_type* operator[](GW_U32 i)
    {
        GW_ASSERT( i<r_size );
        return aData_[i];
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::SetRow
    *
    *  \param  v A row of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetRow( const GW_VectorStatic<c_size,v_type>& v, GW_U32 i )
    {
        GW_ASSERT( i<r_size );
        for( GW_U32 j=0; j<c_size; ++j )
            this->SetData(i,j, v[j]);
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::SetColumn
    *
    *  \param  v A column of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetColumn( const GW_VectorStatic<r_size,v_type>& v, GW_U32 j )
    {
        GW_ASSERT( j<c_size );
        for( GW_U32 i=0; i<r_size; ++i )
            this->SetData(i,j, v[i]);
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::SetZero
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Set all data to zero.
    */
    /*------------------------------------------------------------------------------*/
    void SetZero()
    {
        memset( aData_, 0, this->GetMemorySize() );
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::LoadIdentity
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Set the matrix to the identity.
    */
    /*------------------------------------------------------------------------------*/
    void LoadIdentity()
    {
        ITERATE(i,j,
            if( i==j )    aData_[i][j]=1;
            else        aData_[i][j]=0;
        )
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::SetValue
    *
    *  \param  rVal value to set to the whole matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetValue(v_type rVal)
    {
        memset( aData_, rVal, this->GetMemorySize() );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Randomize
    *
    *  \param  rMin minimum value
    *  \param  rMax maximum value
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void Randomize(v_type rMin=0, v_type rMax=1)
    {
        ITERATE(i,j, this->SetData( i, j, traits::Random( rMin, rMax) ); );
    }



    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Norm1
    *
    *  \return pseuo-norm 1 of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Float Norm1()
    {
        GW_Float rNorm = 0;
        ITERATE(i,j, rNorm+=traits_type::Modulus(this->GetData(i,j)); )
        return  rNorm/(r_size*c_size);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::Norm2
    *
    *  \return pseuo-norm 2 of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Float Norm2()
    {
        GW_Float rNorm = 0;
        ITERATE(i,j, rNorm+=traits_type::SquareModulus(this->GetData(i,j)); )
        return  sqrt( rNorm )/(r_size*c_size);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_MatrixStatic::NormInf
    *
    *  \return pseudo-infinite norm of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Float NormInf()
    {
        GW_Float rNorm = 0;
        ITERATE(i,j,
            if( traits_type::Modulus(this->GetData(i,j)) >rNorm)
                rNorm=traits_type::Modulus(this->GetData(i,j));
        )
        return rNorm;
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_MatrixStatic", s);
        GW_MatrixStatic<3,5,GW_Float> M;
        M.Randomize();

        s << "Test for creation : " << endl;
        s << M << endl;

        s << "Test for transposition : " << endl;
        GW_MatrixStatic<5,3,GW_Float> TM;
        TM = M.Transpose();
        s << TM << endl;

        s << "Test for multiplication : " << endl;
        s << "M^T*M : " << TM*M << endl;
        s << "M*M^T : " << M*TM << endl;

        s << "The rows of the matrix : " << endl;
        for( GW_U32 i=0; i<3; ++i )
            cout << M.GetRow(i) << endl;

        s << "The columns of the matrix : " << endl;
        for( GW_U32 j=0; j<5; ++j )
            cout << M.GetColumn(j) << endl;

        GW_MatrixStatic<5,5,GW_Float> TMM = (TM*M);
        GW_MatrixStatic<3,3,GW_Float> MTM = (M*TM);
        s << "M^T*M = " << TMM << endl;
        s << "M*M^T = " << MTM << endl;

        s << "Norm tests : " << endl;
        s << "|M*M^T|_inf="    << MTM.NormInf() <<
                ", |M*M^T|_1="    << MTM.Norm1()    <<
                ", |M*M^T|_2="    << MTM.Norm2()    << endl;
        s << "|M^T*M|_inf="    << TMM.NormInf() <<
                ", |M^T*M|_1="    << TMM.Norm1()    <<
                ", |M^T*M|_2="    << TMM.Norm2()    << endl;

        s << "Test TM=pseudo-identity : " << endl;
        TM.LoadIdentity();
        s << TM << endl;
        GW_MatrixStatic<3,5,GW_Float> N = M;

        s << "Replace in M a row by [pi,pi/2,pi/3,...] to get matrix N : " << endl;
        GW_VectorStatic<5,GW_Float> r;
        r[0] = GW_PI; r[1] = GW_PI/2; r[2] = GW_PI/3; r[3] = GW_PI/4; r[4] = GW_PI/5;
        N.SetRow(r, 0);
        s << N << endl;

        s << "Replace in N a column by [pi,pi*2,pi*3,...] : " << endl;
        GW_VectorStatic<3,GW_Float> c;
        c[0] = GW_PI; c[1] = GW_PI*2; c[2] = GW_PI*3;
        N.SetColumn(c, 0);
        s << N << endl;

        s << "Addition test : " << endl;
        s << "M+2.5*M = " << (M+(M*2.5)) << endl;

        s << "Substration test : " << endl;
        s << "N-2.5*N = " << (N-(N*2.5)) << endl;

        s << "Inversion test : " << endl;
        GW_MatrixStatic<8,8,GW_Float> R;
        R.Randomize();
        s << "Matrix R : " << R << endl;
        GW_MatrixStatic<8,8,GW_Float> RI = R.Invert();
        s << "The inverse of R : " << RI << endl;
        GW_Float err = (R*RI - GW_MatrixStatic<8,8,GW_Float>()).Norm2();
        s << "Norm 2 of (R*R^(-1)-Id) : " << err << endl;
        GW_ASSERT(err<GW_EPSILON);
        TestClassFooter("GW_MatrixStatic", s);

        s << "System resolution A*x=b, size 8 : " << endl;
        GW_MatrixStatic<8,8,GW_Float> A;
        A.Randomize();
        GW_VectorStatic<8,GW_Float> x,b;
        b.Randomize();
        A.Solve(x,b);
        err = (A*x-b).Norm();
        cout << "Error = " << err << endl;
        GW_ASSERT( err<GW_EPSILON );

        s << "Test of matrix over complex field : a matrix M :" << endl;
        GW_MatrixStatic< 4,4,GW_Complex<GW_Float> > CM;
        GW_Complex<GW_Float> min;
        GW_Complex<GW_Float> max;
        max.real(1); max.imag(1);
        CM.Randomize( min,max );
        cout << CM << endl;
        cout << "|M|_2=" << CM.Norm2() << ", |M|_1=" << CM.Norm1() << ", |M|_inf=" << CM.NormInf() << endl;
    }

protected:

    /** the matrix datas */
    v_type aData_[r_size][c_size];

};


template<GW_U32 r_size, GW_U32 c_size, class v_type, class traits >
inline
std::ostream& operator<<(std::ostream &s, GW_MatrixStatic<r_size, c_size, v_type,traits>& m)
{
    s << "GW_Matrix" << r_size << "x" << c_size;
    if( traits::GetBasicTypeName()!=NULL )
        s << "<" << traits::GetBasicTypeName() << ">";
    s << " : " << endl;
    for( GW_U32 i=0; i<r_size; ++i )
    {
        cout << "|";
        for( GW_U32 j=0; j<c_size; ++j )
        {
            s << m.GetData(i,j);
            if( j!=c_size-1 )
                s << " ";
        }
        s << "|";
        if( i!=r_size-1 )
            s << endl;
    }
    return s;
}


/* Multiplication operators *******************************************************/
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::Multiply
*
*  \param  a left hand matrix.
*  \param  b right hand matrix.
*  \param  r result.
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size, GW_U32 interm_size, GW_U32 c_size, class v_type>
inline
void Multiply(const GW_MatrixStatic<r_size,interm_size,v_type>& a,
                     const GW_MatrixStatic<interm_size,c_size,v_type>& b,
                     GW_MatrixStatic<r_size,c_size,v_type>& r)
{
    for( GW_U32 i=0; i<r_size; ++i )
    for( GW_U32 j=0; j<c_size; ++j )
    {
        v_type rTemp =0;
        for(GW_U32 k=0; k<interm_size; ++k)
            rTemp += a.GetData(i,k)*b.GetData(k,j);
        r.SetData(i,j, rTemp);
    }
}
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::Multiply
*
*  \param  a left hand matrix.
*  \param  b right hand matrix.
*  \param  r result.
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size, GW_U32 interm_size, GW_U32 c_size, class v_type>
inline
GW_MatrixStatic<r_size,c_size,v_type> operator*(const GW_MatrixStatic<r_size,interm_size,v_type>& a,
                                                const GW_MatrixStatic<interm_size,c_size,v_type>& b)
{
    GW_MatrixStatic<r_size,c_size,v_type> r;
    Multiply( a, b, r );
    return r;
}
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::operator *=
*
*  \param  m right hand matrix.
*  \author Gabriel Peyré 2001-11-06
*
*    These matrix must be square of the same size.
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size, class v_type>
inline
void operator *=(const GW_MatrixStatic<r_size,r_size,v_type>& a,
                 const GW_MatrixStatic<r_size,r_size,v_type>& b)
{
    GW_MatrixStatic<r_size,r_size,v_type> r;
    Multiply( a, b, r );
    a = r;
}


/* Addition operators *************************************************************/
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::Add
*
*  \param  a left hand matrix.
*  \param  b right hand matrix.
*  \param  r result.
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size,GW_U32 c_size,class v_type>
inline
static void Add(const GW_MatrixStatic<r_size,c_size,v_type>& a,
                const GW_MatrixStatic<r_size,c_size,v_type>& b,
                GW_MatrixStatic<r_size,c_size,v_type>& r)
{
    ITERATE(i,j, r.SetData(i,j, a.GetData(i,j)+b.GetData(i,j)); )
}
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::Add
*
*  \param  a left hand matrix.
*  \param  b right hand matrix.
*  \param  r result.
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size,GW_U32 c_size,class v_type>
inline
GW_MatrixStatic<r_size,c_size,v_type> operator + (const GW_MatrixStatic<r_size,c_size,v_type> & a,
                                                  const GW_MatrixStatic<r_size,c_size,v_type> & b)
{
    GW_MatrixStatic<r_size,c_size,v_type> r;
    Add( a, b, r );
    return r;
}
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::UMinus
*
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size,GW_U32 c_size,class v_type>
inline
void operator += (const GW_MatrixStatic<r_size,c_size,v_type> & a,
                  const GW_MatrixStatic<r_size,c_size,v_type> & b)
{
    Add( a, b, a );
}


/* Substraction operators *************************************************************/
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::Minus
*
*  \param  a left hand matrix.
*  \param  b right hand matrix.
*  \param  r result.
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size,GW_U32 c_size,class v_type>
inline
static void Minus(    const GW_MatrixStatic<r_size,c_size,v_type>& a,
                    const GW_MatrixStatic<r_size,c_size,v_type>& b,
                          GW_MatrixStatic<r_size,c_size,v_type>& r)
{
    ITERATE(i,j, r.SetData(i,j, a.GetData(i,j)-b.GetData(i,j)); )
}
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::-
*
*  \param  a left hand matrix.
*  \param  b right hand matrix.
*  \param  r result.
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size,GW_U32 c_size,class v_type>
inline
GW_MatrixStatic<r_size,c_size,v_type> operator - (const GW_MatrixStatic<r_size,c_size,v_type> & a,
                                                  const GW_MatrixStatic<r_size,c_size,v_type> & b)
{
    GW_MatrixStatic<r_size,c_size,v_type> r;
    Minus( a, b, r );
    return r;
}
/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::-=
*
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size,GW_U32 c_size,class v_type>
inline
void operator -= (const GW_MatrixStatic<r_size,c_size,v_type> & a,
                  const GW_MatrixStatic<r_size,c_size,v_type> & b)
{
    Minus( a, b, a );
}

/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::UMinus
*
*  \param  a left hand matrix.
*  \param  r result.
*  \return opposed of the matrix.
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size,GW_U32 c_size,class v_type>
inline
void UMinus(const GW_MatrixStatic<r_size,c_size,v_type>& a, GW_MatrixStatic<r_size,c_size,v_type>& r)
{
    ITERATE(i,j, r.SetData(i,j, -a.GetData(i,j)); )
}

/*------------------------------------------------------------------------------*/
/**
* Name : GW_MatrixStatic::unary -
*
*  \author Gabriel Peyré 2001-11-06
*/
/*------------------------------------------------------------------------------*/
template<GW_U32 r_size,GW_U32 c_size,class v_type>
inline
GW_MatrixStatic<r_size,c_size,v_type> operator - (const GW_MatrixStatic<r_size,c_size,v_type> & a)
{
    GW_MatrixStatic<r_size,c_size,v_type> r;
    UMinus( a, r );
}


#undef ITERATE

} // End namespace GW


#endif // GW_MatrixStatic_h

///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2009 Gabriel Peyre
