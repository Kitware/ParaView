/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_Matrix4x4.h
 *  \brief Definition of class \c GW_Matrix4x4
 *  \author Gabriel Peyré 2001-08-04
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_Matrix4x4_h
#define GW_Matrix4x4_h

#include "GW_MathsConfig.h"
#include "GW_Maths.h"
#include "GW_MatrixStatic.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Matrix4x4
 *  \brief  a 4x4 matrix
 *  \author Gabriel Peyré 2001-08-04
 *
 *    A 4x4 transformation matrix. Warning : NOT THE SAME PACKING THAN OPENGL
 *    (here we use *row* major convention)
 */
/*------------------------------------------------------------------------------*/

class GW_Matrix4x4:    public GW_MatrixStatic<4,4,GW_Float>
{

public:

    GW_Matrix4x4():    GW_MatrixStatic<4,4,GW_Float>()        {}
    GW_Matrix4x4(const GW_MatrixStatic<4,4,GW_Float>& m):    GW_MatrixStatic<4,4,GW_Float>(m) {}

    GW_Matrix4x4(    GW_Float m00, GW_Float m01, GW_Float m02, GW_Float m03,
                    GW_Float m10, GW_Float m11, GW_Float m12, GW_Float m13,
                    GW_Float m20, GW_Float m21, GW_Float m22, GW_Float m23,
                    GW_Float m30, GW_Float m31, GW_Float m32, GW_Float m33  )
    {
        aData_[0][0]=m00; aData_[0][1]=m01; aData_[0][2]=m02; aData_[0][3]=m03;
        aData_[1][0]=m10; aData_[1][1]=m11; aData_[1][2]=m12; aData_[1][3]=m13;
        aData_[2][0]=m20; aData_[2][1]=m21;    aData_[2][2]=m22; aData_[2][3]=m23;
        aData_[3][0]=m30; aData_[3][1]=m31; aData_[3][2]=m32; aData_[3][3]=m33;
    }

    void SetData(    GW_Float m00, GW_Float m01, GW_Float m02, GW_Float m03,
                    GW_Float m10, GW_Float m11, GW_Float m12, GW_Float m13,
                    GW_Float m20, GW_Float m21, GW_Float m22, GW_Float m23,
                    GW_Float m30, GW_Float m31, GW_Float m32, GW_Float m33  )
    {
        aData_[0][0]=m00; aData_[0][1]=m01; aData_[0][2]=m02; aData_[0][3]=m03;
        aData_[1][0]=m10; aData_[1][1]=m11; aData_[1][2]=m12; aData_[1][3]=m13;
        aData_[2][0]=m20; aData_[2][1]=m21;    aData_[2][2]=m22; aData_[2][3]=m23;
        aData_[3][0]=m30; aData_[3][1]=m31; aData_[3][2]=m32; aData_[3][3]=m33;
    }

    void SetData( GW_U32 i, GW_U32 j, GW_Float r )
    {
        GW_ASSERT( i<4 && j<4 );
        aData_[i][j] = r;
    }

    /** Invert, assuming the matrix is *ORTHOGONAL* */
    void AutoOrthoInvert()
    {
        GW_Matrix4x4 r;
        OrthoInvert(*this, r);
        (*this) = r;
    }

   /*------------------------------------------------------------------------------*/
    /**
    * Name : OrthoInvert
    *
    *  \return The inverse of an orthogonal matrix.
    *  \author Gabriel Peyré 2001-08-04
    *
    *    Compute the inverse the matrix, assuming that it's an orthogonal matrix.
    */
    /*------------------------------------------------------------------------------*/
    static void OrthoInvert(const GW_Matrix4x4& a, GW_Matrix4x4& r)
    {
        /* the inverse translation : -A^(-1)*t
            +--------+ +--+
            |00 01 02| |30|
           -|10 11 12|*|31|
            |20 21 22| |32|
            +--------+ +--+
        */
        GW_Float t0=a.aData_[0][0]*a.aData_[3][0]+a.aData_[0][1]*a.aData_[3][1]+a.aData_[0][2]*a.aData_[3][2];
        GW_Float t1=a.aData_[1][0]*a.aData_[3][0]+a.aData_[1][1]*a.aData_[3][1]+a.aData_[1][2]*a.aData_[3][2];
        GW_Float t2=a.aData_[2][0]*a.aData_[3][0]+a.aData_[2][1]*a.aData_[3][1]+a.aData_[2][2]*a.aData_[3][2];
        /* the matrix is supposed to be orthogonal !!!!! */
        r.SetData(
            a.aData_[0][0], a.aData_[0][1], a.aData_[0][2], -t0,
            a.aData_[1][0], a.aData_[1][1], a.aData_[1][2], -t1,
            a.aData_[2][0], a.aData_[2][1], a.aData_[2][2], -t2,
            a.aData_[0][3], a.aData_[1][3], a.aData_[2][3],  a.aData_[3][3]);
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix4x4::TransformOnZ
    *
    *  \param  x X component of the original vector.
    *  \param  y Y component of the original vector.
    *  \param  z Z component of the original vector.
    *  \return The X coordinate of the oringal vector transformed by the matrix.
    *  \author Gabriel Peyré 2001-10-30
    *
    *    This speed up computation when you only needs depth information (used by
    *    the alpha pipeline).
    */
    /*------------------------------------------------------------------------------*/
    GW_Float TransformOnZ(GW_Float x, GW_Float y, GW_Float z)
    {
        return aData_[0][2]*x + aData_[1][2]*y + aData_[2][2]*z + aData_[3][2];
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix4x4::TransformOnZ
    *
    *  \param  v the original vector.
    *  \return The X coordinate of the oringal vector transformed by the matrix.
    *  \author Gabriel Peyré 2001-10-30
    *
    *    This speed up computation when you only needs depth information (used by
    *    the alpha pipeline).
    */
    /*------------------------------------------------------------------------------*/
    GW_Float TransformOnZ(GW_Vector3D& v)
    {
        return aData_[0][2]*v[X] + aData_[1][2]*v[Y] + aData_[2][2]*v[Z] + aData_[3][2];
    }

    void RotateX(GW_Float val)
    {
        GW_Float Sin=(GW_Float) sin(val),
            Cos=(GW_Float) cos(val);
        GW_Vector3D tmp=GetY()*Cos+GetZ()*Sin;
        SetZ(-GetY()*Sin+GetZ()*Cos);
        SetY(tmp);
    }

    void RotateY(GW_Float val)
    {
        GW_Float Sin=(GW_Float) sin(val),
            Cos=(GW_Float) cos(val);
        GW_Vector3D tmp=GetX()*Cos+GetZ()*Sin;
        SetZ(-GetX()*Sin+GetZ()*Cos);
        SetX(tmp);
    }

    void RotateZ(GW_Float val)
    {
        GW_Float Sin=(GW_Float) sin(val),
            Cos=(GW_Float) cos(val);
        GW_Vector3D tmp=GetX()*Cos+GetY()*Sin;
        SetY(-GetX()*Sin+GetY()*Cos);
        SetX(tmp);
    }

    void Rotate(GW_U32 axe, GW_Float val)
    {
        switch (axe)
        {
        case X:
            RotateX(val);
            break;
        case Y:
            RotateY(val);
            break;
        case Z:
            RotateZ(val);
            break;
        case W:
            break;
        };
    }

    void RotateAbsX(GW_Float val)
    {
        GW_Float Sin=(GW_Float) sin(val),
            Cos=(GW_Float) cos(val);
        GW_Float tmp;
        for (GW_I32 i=0; i<=2; i++)
        {
            tmp=aData_[i][Y]*Cos-aData_[i][Z]*Sin;
            aData_[i][Z]=aData_[i][Y]*Sin+aData_[i][Z]*Cos;
            aData_[i][Y]=tmp;
        }
    }

    void RotateAbsY(GW_Float val)
    {
        GW_Float Sin=(GW_Float) sin(val),
            Cos=(GW_Float) cos(val);
        GW_Float tmp;
        for (GW_I32 i=0; i<=2; i++)
        {
            tmp=aData_[i][X]*Cos+aData_[i][Z]*Sin;
            aData_[i][Z]=-aData_[i][X]*Sin+aData_[i][Z]*Cos;
            aData_[i][X]=tmp;
        }
    }

    void RotateAbsZ(GW_Float val)
    {
        GW_Float Sin=(GW_Float) sin(val),
            Cos=(GW_Float) cos(val);
        GW_Float tmp;
        for (GW_I32 i=0; i<=2; i++)
        {
            tmp=aData_[i][X]*Cos-aData_[i][Y]*Sin;
            aData_[i][Y]=aData_[i][X]*Sin+aData_[i][Y]*Cos;
            aData_[i][X]=tmp;
        }
    }

    void RotateAbs(GW_U32 axe, GW_Float val)
    {
        switch (axe)
        {
        case X:
            RotateAbsX(val);
            break;
        case Y:
            RotateAbsY(val);
            break;
        case Z:
            RotateAbsZ(val);
            break;
        case W:
            break;
        };
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix4x4::GetRotation
    *
    *  \return The rotation part of the matrix.
    *  \author Gabriel Peyré 2001-08-04
    *
    *    Compute a matrix without the translation (last column).
    */
    /*------------------------------------------------------------------------------*/
    GW_Matrix3x3 GetRotation()
    {
        GW_Matrix3x3 M( this->GetData(0,0), this->GetData(0,1), this->GetData(0,2),
                        this->GetData(1,0), this->GetData(1,1), this->GetData(1,2),
                        this->GetData(2,0), this->GetData(2,1), this->GetData(2,2) );
        return M;
    }

    //-----------------------------------------------------------------------------
    // Name: GW_Matrix4x4::GetCoords
    /**
    *   @return a pointer on the datas of the matrix
    */
    ///    \author Gabriel Peyré 2001-08-30
    //-----------------------------------------------------------------------------
    GW_Float* GetCoords() const
    {
        return (GW_Float*) aData_;
    }


    //-----------------------------------------------------------------------------
    // Name: GW_Matrix4x4::SetCoords
    /**
    *    Set the different elements of the matrix.
    */
    /// \author Gabriel Peyré 2001-08-31
    //-----------------------------------------------------------------------------
    void SetCoords(
                GW_Float m00, GW_Float m10, GW_Float m20, GW_Float m30,
                GW_Float m01, GW_Float m11, GW_Float m21, GW_Float m31,
                GW_Float m02, GW_Float m12, GW_Float m22, GW_Float m32,
                GW_Float m03, GW_Float m13, GW_Float m23, GW_Float m33)
    {
        aData_[0][0]=m00; aData_[0][1]=m01; aData_[0][2]=m02; aData_[0][3]=m03;
        aData_[1][0]=m10; aData_[1][1]=m11; aData_[1][2]=m12; aData_[1][3]=m13;
        aData_[2][0]=m20; aData_[2][1]=m21;    aData_[2][2]=m22; aData_[2][3]=m23;
        aData_[3][0]=m30; aData_[3][1]=m31; aData_[3][2]=m32; aData_[3][3]=m33;
    }
    void AutoScaleTranslation(GW_Float v)
    {
        aData_[3][0] *= v;
        aData_[3][1] *= v;
        aData_[3][2] *= v;
    }

    void UnScale()
    {
        GW_Float l = ::sqrt(aData_[0][0]*aData_[0][0] + aData_[0][1]*aData_[0][1] + aData_[0][2]*aData_[0][2] );
        if( l!=0 )
        {
            aData_[0][0] /= l;
            aData_[0][1] /= l;
            aData_[0][2] /= l;
        }
        l = ::sqrt(aData_[1][0]*aData_[1][0] + aData_[1][1]*aData_[1][1] + aData_[1][2]*aData_[1][2] );
        if( l!=0 )
        {
            aData_[1][0] /= l;
            aData_[1][1] /= l;
            aData_[1][2] /= l;
        }
        l = ::sqrt(aData_[2][0]*aData_[2][0] + aData_[2][1]*aData_[2][1] + aData_[2][2]*aData_[2][2] );
        if( l!=0 )
        {
            aData_[2][0] /= l;
            aData_[2][1] /= l;
            aData_[2][2] /= l;
        }

    }

    /* scale operations **************************************************/
    void AutoScale( const GW_Vector3D& v)
    {
        for(GW_U32 i=0; i<3; ++i)
        for(GW_U32 j=0; j<4; ++j)
            aData_[i][j] *= v[i];
    }
    void AutoScale( GW_U32 axe, GW_Float v )
    {
        for(GW_U32 i=0; i<4; ++i)
            aData_[axe][i] *= v;
    }


    GW_Matrix4x4 operator +( const GW_Vector3D& v )
    {
        GW_Matrix4x4 r = *this;
        r.aData_[3][0] += v[0];
        r.aData_[3][2] += v[1];
        r.aData_[3][1] += v[2];
    }
    void operator +=( const GW_Vector3D& v )
    {
        aData_[3][0] += v[0];
        aData_[3][2] += v[1];
        aData_[3][1] += v[2];
    }
    /* translation by a vector */
    void Translate( const GW_Vector3D& v)
    {
        (*this) += v;
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix4x4::SetScale
    *
    *  \param  rScale the new scale value.
    *  \author Gabriel Peyré 2001-10-23
    */
    /*------------------------------------------------------------------------------*/
    void SetScale(GW_Float rScale)
    {
        GW_ASSERT( rScale>GW_EPSILON );
        aData_[3][3] = rScale;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix4x4::GetScale
    *
    *  \return The scale factors
    *  \author Antoine Bouthors 2002-09-03
    *
    */
    /*------------------------------------------------------------------------------*/
    GW_Vector3D GetScale() const
    {
        return GW_Vector3D( aData_[X][X], aData_[Y][Y], aData_[Z][Z] );
    }


    void Translate(GW_U32 axe, GW_Float v)
    {
        aData_[3][axe]+=v;
    }

    GW_Vector3D GetX() const
    {
        return GW_Vector3D((GW_Float*) aData_[X]);
    }

    GW_Vector3D GetY() const
    {
        return GW_Vector3D((GW_Float*) aData_[Y]);
    }

    GW_Vector3D GetZ() const
    {
        return GW_Vector3D((GW_Float*) aData_[Z]);
    }

    void SetX( const GW_Vector3D& v)
    {
        aData_[0][0]=v[0];
        aData_[0][1]=v[1];
        aData_[0][2]=v[2];
    }

    void SetY( const GW_Vector3D& v)
    {
        aData_[1][0]=v[0];
        aData_[1][1]=v[1];
        aData_[1][2]=v[2];
    }

    void SetZ( const GW_Vector3D& v)
    {
        aData_[2][0]=v[0];
        aData_[2][1]=v[1];
        aData_[2][2]=v[2];
    }

    void SetTranslation( const GW_Vector3D& v)
    {
        aData_[3][X] = v[X];
        aData_[3][Y] = v[Y];
        aData_[3][Z] = v[Z];
    }

    void SetTranslation(GW_Coord_XYZW axe, GW_Float v)
    {
        aData_[3][axe]=v;
    }

    void SetScale( const GW_Vector3D& v)
    {
        aData_[X][X] = v[X];
        aData_[Y][Y] = v[Y];
        aData_[Z][Z] = v[Z];
    }

    void SetScale(GW_Coord_XYZW axe, GW_Float v)
    {
        aData_[axe][axe]=v;
    }

    void SetRotation( const GW_Matrix3x3& m)
    {
        for( GW_U32 i=0; i<3; ++i )
        for( GW_U32 j=0; j<3; ++j )
            this->SetData(i,j, m.GetData(i,j)  );
    }

    GW_Vector3D GetTranslation()
    {
        return GW_Vector3D(aData_[3][0], aData_[3][1], aData_[3][2]);
    }

    GW_Float* GetTranslationPtr()
    {
        return aData_[3];
    }

    void ReComputeBasis_GivenX( GW_Vector3D v)
    {
        /* re-nGWm v */
        v.Normalize();
        SetX(v);
        /* re-compute a basis */
        GW_Vector3D axe_z=v^GetY();
        GW_Float n=~(axe_z);
        if (n<GW_EPSILON)
        {
            /* old X is colinear to new Z, so use Y to compute the new basis */
            GW_Vector3D axe_y=GetZ()^v;
            axe_y.Normalize();
            SetY(axe_y);
            SetZ(v^axe_y);
        }
        else
        {
            /* re-nGWm the new Y */
            axe_z.Normalize();
            SetZ(axe_z);
            SetY(axe_z^v);
        }
    }

    void ReComputeBasis_GivenY( GW_Vector3D v)
    {
        /* re-nGWm v */
        v.Normalize();
        SetY(v);
        /* re-compute a basis */
        GW_Vector3D axe_x=v^GetZ();
        GW_Float n=~(axe_x);
        if (n<GW_EPSILON)
        {
            /* old X is colinear to new Z, so use Y to compute the new basis */
            GW_Vector3D axe_z=GetX()^v;
            axe_z.Normalize();
            SetZ(axe_z);
            SetX(v^axe_z);
        }
        else
        {
            /* re-nGWm the new X */
            axe_x.Normalize();
            SetX(axe_x);
            SetY(axe_x^v);
        }
    }

    void ReComputeBasis_GivenZ( GW_Vector3D v)
    {
        /* re-nGWm v */
        v.Normalize();
        SetZ(v);
        /* re-compute a basis */
        GW_Vector3D axe_x=GetY()^v;
        GW_Float n=~(axe_x);
        if (n<GW_EPSILON)
        {
            /* old X is colinear to new Z, so use Y to compute the new basis */
            GW_Vector3D axe_y=v^GetX();
            axe_y.Normalize();
            SetY(axe_y);
            SetX(axe_y^v);
        }
        else
        {
            /* re-nGWm the new X */
            axe_x.Normalize();
            SetX(axe_x);
            SetY(v^axe_x);
        }
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix4x4::Invert
    *
    *  \return The inverse of the matrix.
    *  \author Gabriel Peyré 2001-08-04
    *
    *    Compute the inverse the matrix, not assuming that it's
    *    an orthogonal matrix.
    *    Make use of the Cramer formula to speed up the computations.
    */
    /*------------------------------------------------------------------------------*/
    static void Invert(const GW_Matrix4x4& a, GW_Matrix4x4& r)
    {
        double x00, x01, x02;
        double x10, x11, x12;
        double x20, x21, x22;
        double x30, x31, x32;
        double rcp;
        double y01, y02, y03, y12, y13, y23;
        double z02, z03, z12, z13, z22, z23, z32, z33;

    #define x03 x01
    #define x13 x11
    #define x23 x21
    #define x33 x31
    #define z00 x02
    #define z10 x12
    #define z20 x22
    #define z30 x32
    #define z01 x03
    #define z11 x13
    #define z21 x23
    #define z31 x33

        /* read 1st two columns of matrix into registers */
        x00 = a.aData_[0][0];
        x01 = a.aData_[1][0];
        x10 = a.aData_[0][1];
        x11 = a.aData_[1][1];
        x20 = a.aData_[0][2];
        x21 = a.aData_[1][2];
        x30 = a.aData_[0][3];
        x31 = a.aData_[1][3];

        /* compute all six 2x2 determinants of 1st two columns */
        y01 = x00*x11 - x10*x01;
        y02 = x00*x21 - x20*x01;
        y03 = x00*x31 - x30*x01;
        y12 = x10*x21 - x20*x11;
        y13 = x10*x31 - x30*x11;
        y23 = x20*x31 - x30*x21;

        /* read 2nd two columns of matrix into registers */
        x02 = a.aData_[2][0];
        x03 = a.aData_[3][0];
        x12 = a.aData_[2][1];
        x13 = a.aData_[3][1];
        x22 = a.aData_[2][2];
        x23 = a.aData_[3][2];
        x32 = a.aData_[2][3];
        x33 = a.aData_[3][3];

        /* compute all 3x3 cofactGWs for 2nd two columns */
        z33 = x02*y12 - x12*y02 + x22*y01;
        z23 = x12*y03 - x32*y01 - x02*y13;
        z13 = x02*y23 - x22*y03 + x32*y02;
        z03 = x22*y13 - x32*y12 - x12*y23;
        z32 = x13*y02 - x23*y01 - x03*y12;
        z22 = x03*y13 - x13*y03 + x33*y01;
        z12 = x23*y03 - x33*y02 - x03*y23;
        z02 = x13*y23 - x23*y13 + x33*y12;

        /* compute all six 2x2 determinants of 2nd two columns */
        y01 = x02*x13 - x12*x03;
        y02 = x02*x23 - x22*x03;
        y03 = x02*x33 - x32*x03;
        y12 = x12*x23 - x22*x13;
        y13 = x12*x33 - x32*x13;
        y23 = x22*x33 - x32*x23;

        /* read 1st two columns of matrix into registers */
        x00 = a.aData_[0][0];
        x01 = a.aData_[1][0];
        x10 = a.aData_[0][1];
        x11 = a.aData_[1][1];
        x20 = a.aData_[0][2];
        x21 = a.aData_[1][2];
        x30 = a.aData_[0][3];
        x31 = a.aData_[1][3];

        /* compute all 3x3 cofactGWs for 1st column */
        z30 = x11*y02 - x21*y01 - x01*y12;
        z20 = x01*y13 - x11*y03 + x31*y01;
        z10 = x21*y03 - x31*y02 - x01*y23;
        z00 = x11*y23 - x21*y13 + x31*y12;

        /* compute 4x4 determinant & its reciprocal */
        rcp = x30*z30 + x20*z20 + x10*z10 + x00*z00;
        if (rcp == 0.0f)
        {
            /* the matrix can't be inverted */
            return;
        }

        rcp = 1.0f/rcp;

        /* compute all 3x3 cofactGWs for 2nd column */
        z31 = x00*y12 - x10*y02 + x20*y01;
        z21 = x10*y03 - x30*y01 - x00*y13;
        z11 = x00*y23 - x20*y03 + x30*y02;
        z01 = x20*y13 - x30*y12 - x10*y23;

        /* multiply all 3x3 cofactGWs by reciprocal */
        r.SetData(0,0, (GW_Float)(z00*rcp));
        r.SetData(0,1, (GW_Float)(z01*rcp));
        r.SetData(1,0, (GW_Float)(z10*rcp));
        r.SetData(0,2, (GW_Float)(z02*rcp));
        r.SetData(2,0, (GW_Float)(z20*rcp));
        r.SetData(0,3, (GW_Float)(z03*rcp));
        r.SetData(3,0, (GW_Float)(z30*rcp));
        r.SetData(1,1, (GW_Float)(z11*rcp));
        r.SetData(1,2, (GW_Float)(z12*rcp));
        r.SetData(2,1, (GW_Float)(z21*rcp));
        r.SetData(1,3, (GW_Float)(z13*rcp));
        r.SetData(3,1, (GW_Float)(z31*rcp));
        r.SetData(2,2, (GW_Float)(z22*rcp));
        r.SetData(2,3, (GW_Float)(z23*rcp));
        r.SetData(3,2, (GW_Float)(z32*rcp));
        r.SetData(3,3, (GW_Float)(z33*rcp));
    #undef x03
    #undef x13
    #undef x23
    #undef x33
    #undef z00
    #undef z10
    #undef z20
    #undef z30
    #undef z01
    #undef z11
    #undef z21
    #undef z31

    }

    /** Return the inverse of the matrix */
    GW_Matrix4x4 Invert() const
    {
        GW_Matrix4x4 r;
        GW_Matrix4x4::Invert( *this, r );
        return r;
    }

    /** solve the linear system A*x=b */
    void Solve( GW_Vector4D& x, const GW_Vector4D& b ) const
    {
        GW_Matrix4x4 M = this->Invert();
        GW_MatrixStatic<4,4,GW_Float>::Multiply( M, b, x );
    }



    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Matrix4x4::*
    *
    *  \return The product of the matrix by a vectGW.
    *    \param  v The result of the product.
    *  \author Gabriel Peyré 2001-08-04
    */
    /*------------------------------------------------------------------------------*/
    GW_Vector3D operator*( const GW_Vector3D& v)
    {
        return GW_Vector3D(
            aData_[0][0]*v[0]+aData_[1][0]*v[1]+aData_[2][0]*v[2]+aData_[3][0],
            aData_[0][1]*v[0]+aData_[1][1]*v[1]+aData_[2][1]*v[2]+aData_[3][1],
            aData_[0][2]*v[0]+aData_[1][2]*v[1]+aData_[2][2]*v[2]+aData_[3][2]);
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_Matrix4x4", s);
        cout << "Test for matrix inversion : " << endl;
        GW_Matrix4x4 M;
        M.Randomize();
        cout << "Matrix M = " << M << endl;
        GW_Matrix4x4 MM = M.Invert();
        cout << "Matrix M^-1 = " << MM << endl;
        GW_Float err = (M*MM - GW_Matrix4x4()).Norm2();
        cout << "|| MM^-1 - ID ||_2 = " << err << endl;
        GW_ASSERT( err<GW_EPSILON );
        TestClassFooter("GW_Matrix4x4", s);
    }

};


} // End namespace GW



#endif // GW_Matrix4x4_h

///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2009 Gabriel Peyre
