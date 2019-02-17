/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_Matrix2x2.h
 *  \brief Definition of class \c GW_Matrix2x2
 *  \author Gabriel Peyré 2001-09-18
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_Matrix2x2_h
#define GW_Matrix2x2_h

#include "GW_MathsConfig.h"
#include "GW_Maths.h"
#include "GW_MatrixStatic.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Matrix2x2
 *  \brief  A matrix of dimension 3.
 *  \author Gabriel Peyré 2001-09-18
 *
 *    A 3x3 transformation matrix. Warning : NOT THE SAME PACKING THAN OPENGL
 *    (here we use *row* major convention)
 */
/*------------------------------------------------------------------------------*/

class GW_Matrix2x2:    public GW_MatrixStatic<2,2,GW_Float>
{

public:

    GW_Matrix2x2():    GW_MatrixStatic<2,2,GW_Float>()        {}
    GW_Matrix2x2(const GW_MatrixStatic<2,2,GW_Float>& m):    GW_MatrixStatic<2,2,GW_Float>(m) {}

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix2x2 constructor
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Initialize the matrix with given coordinates.
    */
    /*------------------------------------------------------------------------------*/
    GW_Matrix2x2(    GW_Float m00, GW_Float m01,
                    GW_Float m10, GW_Float m11 )
    {
        aData_[0][0]=m00; aData_[0][1]=m01;
        aData_[1][0]=m10; aData_[1][1]=m11;
    }

    GW_Matrix2x2( GW_Vector2D& v1, GW_Vector2D& v2 )
    {
        aData_[0][0]=v1[0]; aData_[0][1]=v2[0];
        aData_[1][0]=v1[1]; aData_[1][1]=v2[1];
    }



    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix2x2::SetData
    *
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetData(    GW_Float m00, GW_Float m01,
                    GW_Float m10, GW_Float m11 )
    {
        aData_[0][0]=m00; aData_[0][1]=m01;
        aData_[1][0]=m10; aData_[1][1]=m11;
    }

    void SetData( GW_U32 i, GW_U32 j, GW_Float r )
    {
        GW_ASSERT( i<2 && j<2 );
        aData_[i][j] = r;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix2x2::Invert
    *
    *  \param  a matrix to invert
    *  \param  r result.
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Use of Cramer formula for speed up in dimension 2.
    */
    /*------------------------------------------------------------------------------*/
    static void Invert(const GW_Matrix2x2& a, GW_Matrix2x2& r)
    {
        GW_Float det = a.GetData(0,0)*a.GetData(1,1) - a.GetData(1,0)*a.GetData(0,1);
        if( det==0 )
            return;
        det = 1/det;
        r.SetData(0,0,  det*a.GetData(1,1));
        r.SetData(1,0, -det*a.GetData(1,0));
        r.SetData(0,1, -det*a.GetData(0,1));
        r.SetData(1,1,  det*a.GetData(0,0));
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix2x2::Invert
    *
    *  \return the inverse of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Matrix2x2 Invert() const
    {
        GW_Matrix2x2 r;
        GW_Matrix2x2::Invert(*this, r);
        return r;
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : AutoInvert
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Invert the matrix.
    */
    /*------------------------------------------------------------------------------*/
    void AutoInvert()
    {
        GW_Matrix2x2 r;
        GW_Matrix2x2::Invert(*this, r);
        (*this) = r;
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix2x2::GetX
    *
    *  \return X vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Vector2D GetX() const
    {
        return this->GetColumn(X);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix2x2::GetY
    *
    *  \return Y vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Vector2D GetY() const
    {
        return this->GetColumn(Y);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix2x2::SetX
    *
    *  \param  v X vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetX( const GW_Vector2D& v)
    {
        return this->SetColumn(v,X);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix2x2::SetX
    *
    *  \param  v Y vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetY( const GW_Vector2D& v)
    {
        return this->SetColumn(v,Y);
    }

    /** solve the linear system A*x=b */
    void Solve( GW_Vector2D& x, const GW_Vector2D& b ) const
    {
        GW_Matrix2x2 M = this->Invert();
        x = M*b;
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_Matrix2x2", s);
        cout << "Test for matrix inversion : " << endl;
        GW_Matrix2x2 M;
        M.Randomize();
        cout << "Matrix M = " << M << endl;
        GW_Matrix2x2 MM = M.Invert();
        cout << "Matrix M^-1 = " << MM << endl;
        GW_Float err = (M*MM - GW_Matrix2x2()).Norm2();
        cout << "|| MM^-1 - ID ||_2 = " << err << endl;
        GW_ASSERT( err<GW_EPSILON );
        TestClassFooter("GW_Matrix2x2", s);
    }

};


} // End namespace GW


#endif // GW_Matrix2x2_h

///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2009 Gabriel Peyre
