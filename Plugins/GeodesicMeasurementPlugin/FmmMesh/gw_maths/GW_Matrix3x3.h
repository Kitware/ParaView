/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_Matrix3x3.h
 *  \brief Definition of class \c GW_Matrix3x3
 *  \author Gabriel Peyré 2001-09-18
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_Matrix3x3_h
#define GW_Matrix3x3_h

#include "GW_MathsConfig.h"
#include "GW_Maths.h"
#include "GW_MatrixStatic.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Matrix3x3
 *  \brief  A matrix of dimension 3.
 *  \author Gabriel Peyré 2001-09-18
 *
 *    A 3x3 transformation matrix. Warning : NOT THE SAME PACKING THAN OPENGL
 *    (here we use *row* major convention)
 */
/*------------------------------------------------------------------------------*/

class GW_Matrix3x3:    public GW_MatrixStatic<3,3,GW_Float>
{

public:

    GW_Matrix3x3():    GW_MatrixStatic<3,3,GW_Float>()        {}
    GW_Matrix3x3(const GW_MatrixStatic<3,3,GW_Float>& m):    GW_MatrixStatic<3,3,GW_Float>(m) {}

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3 constructor
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Initialize the matrix with given coordinates.
    */
    /*------------------------------------------------------------------------------*/
    GW_Matrix3x3(    GW_Float m00, GW_Float m01, GW_Float m02,
                    GW_Float m10, GW_Float m11, GW_Float m12,
                    GW_Float m20, GW_Float m21, GW_Float m22 )
    {
        aData_[0][0]=m00; aData_[0][1]=m01; aData_[0][2]=m02;
        aData_[1][0]=m10; aData_[1][1]=m11; aData_[1][2]=m12;
        aData_[2][0]=m20; aData_[2][1]=m21;    aData_[2][2]=m22;
    }

    GW_Matrix3x3( GW_Vector3D& v1, GW_Vector3D& v2, GW_Vector3D& v3 )
    {
        aData_[0][0]=v1[0]; aData_[0][1]=v2[0]; aData_[0][2]=v3[0];
        aData_[1][0]=v1[1]; aData_[1][1]=v2[1]; aData_[1][2]=v3[1];
        aData_[1][0]=v1[2]; aData_[2][1]=v2[2]; aData_[2][2]=v3[2];
    }



    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::SetData
    *
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetData(    GW_Float m00, GW_Float m01, GW_Float m02,
                    GW_Float m10, GW_Float m11, GW_Float m12,
                    GW_Float m20, GW_Float m21, GW_Float m22 )
    {
        aData_[0][0]=m00; aData_[0][1]=m01; aData_[0][2]=m02;
        aData_[1][0]=m10; aData_[1][1]=m11; aData_[1][2]=m12;
        aData_[2][0]=m20; aData_[2][1]=m21;    aData_[2][2]=m22;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::Invert
    *
    *  \param  a matrix to invert
    *  \param  r result.
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Use of Cramer formula for speed up in dimension 3.
    */
    /*------------------------------------------------------------------------------*/
    #define ACCESS(i,j) aData[j*3+i]
    #define ACCESS_R(i,j) aResData[j*3+i]
    #define SUBDET(i0,j0, i1,j1) ( ACCESS(j0,i0)*ACCESS(j1,i1) - ACCESS(j1,i0)*ACCESS(j0,i1) )
    static void Invert(const GW_Matrix3x3& a, GW_Matrix3x3& r)
    {
        const GW_Float* aData    = a.GetData();
        GW_Float* aResData = r.GetData();

        /* cofactor */
        GW_Float c00, c01, c02,
                c10, c11, c12,
                c20, c21, c22;

        c00 = SUBDET(1,1, 2,2);
        c01 =-SUBDET(1,0, 2,2);
        c02 = SUBDET(1,0, 2,1);

        c10 =-SUBDET(0,1, 2,2);
        c11 = SUBDET(0,0, 2,2);
        c12 =-SUBDET(0,0, 2,1);

        c20 = SUBDET(0,1, 1,2);
        c21 =-SUBDET(0,0, 1,2);
        c22 = SUBDET(0,0, 1,1);

        /* compute 3x3 determinant & its reciprocal */
        GW_Float rDet = ACCESS(0,0)*c00 + ACCESS(1,0)*c01 + ACCESS(2,0)*c02;
        if (rDet == 0.0f)
        {
            /* the aData can't be inverted */
            return;
        }

        rDet = 1.0f/rDet;

        ACCESS_R(0,0) = c00*rDet;
        ACCESS_R(0,1) = c01*rDet;
        ACCESS_R(0,2) = c02*rDet;

        ACCESS_R(1,0) = c10*rDet;
        ACCESS_R(1,1) = c11*rDet;
        ACCESS_R(1,2) = c12*rDet;

        ACCESS_R(2,0) = c20*rDet;
        ACCESS_R(2,1) = c21*rDet;
        ACCESS_R(2,2) = c22*rDet;

    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::Invert
    *
    *  \return the inverse of the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Matrix3x3 Invert() const
    {
        GW_Matrix3x3 r;
        GW_Matrix3x3::Invert(*this, r);
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
        GW_Matrix3x3 r;
        GW_Matrix3x3::Invert(*this, r);
        (*this) = r;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::RotateX
    *
    *  \param val rotation angle.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void RotateX(GW_Float val)
    {
        GW_Float    Sin = (GW_Float) sin(val),
                    Cos = (GW_Float) cos(val);
        GW_Vector3D tmp = this->GetY()*Cos + this->GetZ()*Sin;
        this->SetZ(-this->GetY()*Sin + this->GetZ()*Cos);
        this->SetY(tmp);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::RotateX
    *
    *  \param val rotation angle.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void RotateY(GW_Float val)
    {
        GW_Float    Sin = (GW_Float) sin(val),
                    Cos = (GW_Float) cos(val);
        GW_Vector3D tmp = this->GetX()*Cos + this->GetZ()*Sin;
        this->SetZ(-this->GetX()*Sin + this->GetZ()*Cos);
        this->SetX(tmp);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::RotateX
    *
    *  \param val rotation angle.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void RotateZ(GW_Float val)
    {
        GW_Float    Sin = (GW_Float) sin(val),
                    Cos = (GW_Float) cos(val);
        GW_Vector3D tmp = this->GetX()*Cos + this->GetY()*Sin;
        this->SetY(-this->GetX()*Sin + this->GetY()*Cos);
        this->SetX(tmp);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::RotateX
    *
    *  \param val rotation angle.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void Rotate(GW_Coord_XYZW axe, GW_Float val)
    {
        switch (axe)
        {
        case X:
            this->RotateX(val);
            break;
        case Y:
            this->RotateY(val);
            break;
        case Z:
            this->RotateZ(val);
            break;
        case W:
            break;
        };
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::RotateX
    *
    *  \param val rotation angle.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void RotateAbsX(GW_Float val)
    {
        GW_Float    Sin = (GW_Float) sin(val),
                    Cos = (GW_Float) cos(val);
        GW_Float tmp;
        for( GW_I32 i=0; i<=2; ++i )
        {
            tmp = aData_[i][Y]*Cos-aData_[i][Z]*Sin;
            aData_[i][Z] = aData_[i][Y]*Sin+aData_[i][Z]*Cos;
            aData_[i][Y] = tmp;
        }
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::RotateX
    *
    *  \param val rotation angle.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void RotateAbsY(GW_Float val)
    {
        GW_Float    Sin = (GW_Float) sin(val),
                    Cos = (GW_Float) cos(val);
        GW_Float tmp;
        for( GW_I32 i=0; i<=2; ++i )
        {
            tmp = aData_[i][X]*Cos + aData_[i][Z]*Sin;
            aData_[i][Z] = -aData_[i][X]*Sin + aData_[i][Z]*Cos;
            aData_[i][X] = tmp;
        }
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::RotateX
    *
    *  \param val rotation angle.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void RotateAbsZ(GW_Float val)
    {
        GW_Float    Sin = (GW_Float) sin(val),
                    Cos = (GW_Float) cos(val);
        GW_Float tmp;
        for( GW_I32 i=0; i<=2; ++i )
        {
            tmp = aData_[i][X]*Cos - aData_[i][Y]*Sin;
            aData_[i][Y] = aData_[i][X]*Sin + aData_[i][Y]*Cos;
            aData_[i][X] = tmp;
        }
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::RotateX
    *
    *  \param val rotation angle.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void RotateAbs(GW_Coord_XYZW axe, GW_Float val)
    {
        switch (axe)
        {
        case X:
            this->RotateAbsX(val);
            break;
        case Y:
            this->RotateAbsY(val);
            break;
        case Z:
            this->RotateAbsZ(val);
            break;
        case W:
            break;
        };
    }



    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::GetX
    *
    *  \return X vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Vector3D GetX() const
    {
        return this->GetColumn(X);
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::GetY
    *
    *  \return Y vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Vector3D GetY() const
    {
        return this->GetColumn(Y);
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::GetZ
    *
    *  \return Z vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    GW_Vector3D GetZ() const
    {
        return this->GetColumn(Z);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::SetX
    *
    *  \param  v X vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetX( const GW_Vector3D& v)
    {
        return this->SetColumn(v,X);
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::SetX
    *
    *  \param  v Y vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetY( const GW_Vector3D& v)
    {
        return this->SetColumn(v,Y);
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix3x3::SetZ
    *
    *  \param  v Z vector of the basis defined by the matrix.
    *  \author Gabriel Peyré 2001-11-06
    */
    /*------------------------------------------------------------------------------*/
    void SetZ( const GW_Vector3D& v)
    {
        return this->SetColumn(v,Z);
    }

    /** solve the linear system A*x=b */
    void Solve( GW_Vector3D& x, const GW_Vector3D& b ) const
    {
        GW_Matrix3x3 M = this->Invert();
        x = M*b;
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_Matrix3x3", s);
        cout << "Test for matrix inversion : " << endl;
        GW_Matrix3x3 M;
        M.Randomize();
        cout << "Matrix M = " << M << endl;
        GW_Matrix3x3 MM = M.Invert();
        cout << "Matrix M^-1 = " << MM << endl;
        GW_Float err = (M*MM - GW_Matrix3x3()).Norm2();
        cout << "|| MM^-1 - ID ||_2 = " << err << endl;
        GW_ASSERT( err<GW_EPSILON );
        TestClassFooter("GW_Matrix3x3", s);
    }

};


} // End namespace GW


#endif // GW_Matrix3x3_h

///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2009 Gabriel Peyre
