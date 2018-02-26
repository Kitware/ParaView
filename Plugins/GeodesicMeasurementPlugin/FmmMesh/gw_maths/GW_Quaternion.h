/*----------------------------------------------------------------------------*/
/*                         GW_Quaternion.h                                    */
/*----------------------------------------------------------------------------*/
/* a quaternion                                                               */
/*----------------------------------------------------------------------------*/


/** \file
    This file contains the definition of a quaternion, which can be used to
    represent a rotation, and is mainly used for animation purposes.
    \author Gabriel.
**/

#ifndef _GW_QUATERNION_H_
#define _GW_QUATERNION_H_

#include "GW_MathsConfig.h"
#include "GW_Maths.h"
#include "GW_Matrix3x3.h"

namespace GW
{

class GW_Matrix3x3;

/*--------------------------------------------------------------------*/
/*                       class GW_Quaternion                          */
/*--------------------------------------------------------------------*/
/* a quaternion                                                       */
/*--------------------------------------------------------------------*/

/*! \ingroup group_primitive
 *  \brief class GW_Quaternion in group group primitive
 */

/// A quatenion has 4 components, and represent rotation matrix.
/**
    The quaternion algebra is known to be the only real algebra (with division)
    of finite dimension after the complex plane.
    This result is due to Frobenius
    \author Gabriel
*/

class GW_Quaternion
{

public:

    GW_Quaternion::GW_Quaternion()
    {
        x=y=z=0;
        w=1;
    }

    GW_Quaternion::GW_Quaternion(const GW_Float _w, const GW_Float _x,
                                const GW_Float _y, const GW_Float _z)
    {
        x=_x;
        y=_y;
        z=_z;
        w=_w;
    }

    GW_Quaternion::GW_Quaternion(const GW_Float _w, const GW_Float axe[3])
    {
        x=axe[X];
        y=axe[Y];
        z=axe[Z];
        w=_w;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion constructor
    *
    *  \param  axe the axe of the quaternion
    *  \author Gabriel Peyré 2001-09-10
    */
    /*------------------------------------------------------------------------------*/
    GW_Quaternion::GW_Quaternion(GW_Vector3D& axe)
    {
        x = axe[X];
        y = axe[Y];
        z = axe[Z];
        w = 0;
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion constructor
    *
    *  \param  mat Rotation matrix
    *  \author Antoine Bouthors 2002-09-03
    *
    */
    /*------------------------------------------------------------------------------*/
    GW_Quaternion::GW_Quaternion(GW_Matrix3x3& mat)
    {
        BuildFromMatrix( mat );
    }

    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::GetX
    *
    *  \return the X coord.
    *  \author Gabriel Peyré 2001-11-28
    */
    /*------------------------------------------------------------------------------*/
    GW_Float GW_Quaternion::GetX() const
    {
        return x;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::GetY
    *
    *  \return the Y coord.
    *  \author Gabriel Peyré 2001-11-28
    */
    /*------------------------------------------------------------------------------*/
    GW_Float GW_Quaternion::GetY() const
    {
        return y;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::GetZ
    *
    *  \return the Z coord.
    *  \author Gabriel Peyré 2001-11-28
    */
    /*------------------------------------------------------------------------------*/
    GW_Float GW_Quaternion::GetZ() const
    {
        return z;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::GetW
    *
    *  \return the W coord.
    *  \author Gabriel Peyré 2001-11-28
    */
    /*------------------------------------------------------------------------------*/
    GW_Float GW_Quaternion::GetW() const
    {
        return w;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::SetW
    *
    *  \return the W coord.
    *  \author Gabriel Peyré 2001-11-28
    */
    /*------------------------------------------------------------------------------*/
    void GW_Quaternion::SetW(GW_Float rVal)
    {
        w = rVal;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::SetX
    *
    *  \return the X coord.
    *  \author Gabriel Peyré 2001-11-28
    */
    /*------------------------------------------------------------------------------*/
    void GW_Quaternion::SetX(GW_Float rVal)
    {
        x = rVal;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::SetY
    *
    *  \return the Y coord.
    *  \author Gabriel Peyré 2001-11-28
    */
    /*------------------------------------------------------------------------------*/
    void GW_Quaternion::SetY(GW_Float rVal)
    {
        y = rVal;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::SetZ
    *
    *  \return the Z coord.
    *  \author Gabriel Peyré 2001-11-28
    */
    /*------------------------------------------------------------------------------*/
    void GW_Quaternion::SetZ(GW_Float rVal)
    {
        z = rVal;
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::SetIdentity
    *
    *  \return this
    *  \author Antoine Bouthors 2002-09-01
    *
    * Set the quaternion to the identity (x=y=z=0;w=1;)
    */
    /*------------------------------------------------------------------------------*/
    GW_Quaternion& GW_Quaternion::SetIdentity()
    {
        x=y=z=0;
        w=1;
        return( *this );
    }


    /** Quats multiplication. */
    GW_Quaternion GW_Quaternion::operator*(const GW_Quaternion& b) const
    {
        return GW_Quaternion(w*b.w - (x*b.x + y*b.y + z*b.z),
                        w*b.x + b.w*x + y*b.z - z*b.y,
                        w*b.y + b.w*y + z*b.x - x*b.z,
                        w*b.z + b.w*z + x*b.y - y*b.x);
    }
    /** Auto multiplication */
    GW_Quaternion& GW_Quaternion::operator*=(const GW_Quaternion& q)
    {
        GW_Float _w = w*q.w - (x*q.x + y*q.y + z*q.z);
        GW_Float _x = w*q.x + q.w*x + y*q.z - z*q.y;
        GW_Float _y = w*q.y + q.w*y + z*q.x - x*q.z;
        z = w*q.z + q.w*z + x*q.y - y*q.x;
        w = _w;
        x = _x;
        y = _y;
        return *this;
    }
    /** Divide */
    GW_Quaternion GW_Quaternion::operator/(const GW_Quaternion& q)
    {
        GW_Quaternion qinv = q.Invert();
        qinv = (*this)*q;
        return q;
    }
    /** Scaling */
    GW_Quaternion GW_Quaternion::operator*(GW_Float m) const
    {
        return GW_Quaternion(m*w, m*x, m*y, m*z);
    }
    /** Auto scaling */
    GW_Quaternion& GW_Quaternion::operator*=(GW_Float f)
    {
        z *= f;
        w *= f;
        x *= f;
        y *= f;
        return *this;
    }
    /** Invert-Scaling */
    GW_Quaternion GW_Quaternion::operator/(GW_Float m) const
    {
        if( m==0 )
            return GW_Quaternion();
        else
            return GW_Quaternion(w/m, x/m, y/m, z/m);
    }
    /** Auto invert-scaling */
    GW_Quaternion& GW_Quaternion::operator/=(GW_Float f)
    {
        if( f!=0 )
        {
            f = 1/f;
            z *= f;
            w *= f;
            x *= f;
            y *= f;
        }
        return *this;
    }
    /** Auto-addition */
    GW_Quaternion& GW_Quaternion::operator+=(GW_Quaternion &q)
    {
        w+=q.w;
        x+=q.x;
        y+=q.y;
        z+=q.z;
        return *this;
    }
    /** Auto-substraction. */
    GW_Quaternion& GW_Quaternion::operator-=(GW_Quaternion &q)
    {
        w-=q.w;
        x-=q.x;
        y-=q.y;
        z-=q.z;
        return *this;
    }
    /** Addition of quats */
    GW_Quaternion GW_Quaternion::operator+(GW_Quaternion &q) const
    {
        return GW_Quaternion(w+q.w, x+q.x, y+q.y, z+q.z);
    }
    /** Substraction. */
    GW_Quaternion GW_Quaternion::operator-(GW_Quaternion &q) const
    {
        return GW_Quaternion(w-q.w, x-q.x, y-q.y, z-q.z);
    }
    /** Unary minus. */
    GW_Quaternion GW_Quaternion::operator-() const
    {
        return GW_Quaternion(-w, -x, -y, -z);
    }
    /** Conjugaison. */
    GW_Quaternion GW_Quaternion::Conjugate() const
    {
        return GW_Quaternion(w, -x, -y, -z);
    }
    /** Auto conjugaison. */
    void GW_Quaternion::AutoConjugate()
    {
        x=-x;
        y=-y;
        z=-z;
    }
    /** Inversion */
    GW_Quaternion Invert() const
    {
        return this->Conjugate()/this->SquareNorm();
    }
    /** auto inversion */
    void AutoInvert()
    {
        this->AutoConjugate();
        (*this) /= this->SquareNorm();
    }
    /** Build a unit length quat. */
    void GW_Quaternion::AutoNormalize()
    {
        GW_Float norm = this->Norm();
        if (norm == 0.0)
        {
            w = 1.0;
            x = y = z = 0.0;
        }
        else
        {
            GW_Float recip = 1/norm;

            w *= recip;
            x *= recip;
            y *= recip;
            z *= recip;
        }
    }
    /** Build a unit length quat. */
    GW_Quaternion GW_Quaternion::Normalize() const
    {
        GW_Float norme = this->Norm();
        if (norme == 0.0)
        {
            return GW_Quaternion();
        }
        else
        {
            GW_Float recip = 1/norme;
            return GW_Quaternion(w*recip, x*recip, y*recip, z*recip);
        }
    }
    /** Return a normalized quaternion. */
    GW_Quaternion operator !() const
    {
        return this->Normalize();
    }
    /** The norm. */
    GW_Float GW_Quaternion::Norm() const
    {
        return ::sqrt( this->SquareNorm() );
    }
    /** get the norm */
    GW_Float GW_Quaternion::operator ~()
    {
        return this->Norm();
    }
    /** Square norm. */
    GW_Float GW_Quaternion::SquareNorm() const
    {
        return w*w + x*x + y*y + z*z;
    }


    /** Build from axis and angle */
    void GW_Quaternion::BuildFromAxis(const GW_Float angle, GW_Float _x, GW_Float _y, GW_Float _z)
    {
        GW_Float omega, s, c;

        s = _x*_x + _y*_y + _z*_z;

        if (fabs(s) > GW_EPSILON)
        {
            c=1/s;

            _x*=c;
            _y*=c;
            _z*=c;

            omega=-0.5f*angle;
            s=(GW_Float)sin(omega);

            x=s*_x;
            y=s*_y;
            z=s*_z;
            w=(GW_Float)cos(omega);
        }
        else
        {
            x=y=0.0f;
            z=0.0f;
            w=1.0f;
        }
        this->AutoNormalize();
    }


    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::ToAxisAndAngle
    *
    *  \param  axis output the axis of the rotation.
    *  \param  angle output the angle of the rotation (\b IN \b RADIAN).
    *  \author Gabriel Peyré 2001-11-06
    *
    *    A quaternion can be converted back to a rotation axis and angle
    *    using the following algorithm :
    *
    *    \code
    *    If the axis of rotation is         (ax, ay, az)
    *    and the angle is                   theta (radians)
    *    then the                           angle= 2 * acos(w)
    *
    *    ax= x / scale
    *    ay= y / scale
    *  az= z / scale
    *
    *    where scale = x^2 + y^2 + z^2
    *    \endcode
    */
    /*------------------------------------------------------------------------------*/
    void GW_Quaternion::ToAxisAndAngle(GW_Vector3D& axis, GW_Float& angle) const
    {
        GW_Float rAngle = 2*((GW_Float) acos( w ));
        GW_Float rScale = x*x + y*y + z*z;

        if( GW_ABS(rScale)<GW_EPSILON )
            return;

        rScale = 1/rScale;

        axis.SetData(0, x*rScale );
        axis.SetData(1, y*rScale );
        axis.SetData(2, z*rScale );
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion constructor
    *
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Create a matrix representing the same rotation.
    *
    *    Then the quaternion can then be converted into a 4x4 rotation
    *    matrix using the following expression:
    *
    *    \code
    *        |       2     2                                |
    *        | 1 - 2Y  - 2Z    2XY - 2ZW      2XZ + 2YW     |
    *        |                                              |
    *        |                       2     2                |
    *    M = | 2XY + 2ZW       1 - 2X  - 2Z   2YZ - 2XW     |
    *        |                                              |
    *        |                                      2     2 |
    *        | 2XZ - 2YW       2YZ + 2XW      1 - 2X  - 2Y  |
    *        |                                              |
    *    \endcode
    */
    /*------------------------------------------------------------------------------*/
    GW_Matrix3x3 GW_Quaternion::ToMatrix() const
    {
        return GW_Matrix3x3(1.0f - 2*y*y - 2*z*z, 2*x*y + 2*w*z, 2*x*z - 2*w*y,
                        2*x*y - 2*w*z, 1.0f - 2*x*x - 2*z*z, 2*y*z + 2*w*x,
                        2*x*z + 2*w*y, 2*y*z - 2*w*x, 1.0f - 2*x*x - 2*y*y);
    }
    /*------------------------------------------------------------------------------*/
    /**
    * Name : GW_Quaternion::BuildFromMatrix
    *
    *  \param  m rotation matrix.
    *  \author Gabriel Peyré 2001-11-06
    *
    *    Build the quaternion to reflect the rotation matrix.
    */
    /*------------------------------------------------------------------------------*/
    void GW_Quaternion::BuildFromMatrix( const GW_Matrix3x3& m )
    {
        /** \todo find this f&%$* formula */
        const GW_Float* M = m.GetData();

        // Compute the Trace of the matrix
        GW_Float T = 1 + M[0] + M[4] + M[8];
        GW_Float S;
        if( T > 0 )
        {
            S = 0.5f / ::sqrt( T );
            x = .25f / S;
            y = ( M[7] - M[5] ) * S;
            z = ( M[2] - M[6] ) * S;
            w = ( M[3] - M[1] ) * S;
        }
        else
        {
            // find the column of the matrix which has the greatest value
            GW_U32 i = 2;
            if( M[0] > M[4] && M[0] > M[8] ) i = 0;
            if( M[4] > M[0] && M[4] > M[8] ) i = 1;

            switch( i )
            {
                case 0:
                    S = ::sqrt( 1.f + M[0] - M[4] - M[8] ) * 2;
                    y = .5f / S;
                    z = ( M[1] + M[3] ) / S;
                    w = ( M[2] + M[6] ) / S;
                    x = ( M[5] + M[7] ) / S;
                break;

                case 1:
                    S = ::sqrt( 1.f - M[0] + M[4] - M[8] ) * 2;
                    z = .5f / S;
                    y = ( M[1] + M[3] ) / S;
                    x = ( M[2] + M[6] ) / S;
                    w = ( M[5] + M[7] ) / S;
                break;

                case 2:
                    S = ::sqrt( 1.f - M[0] - M[4] + M[8] ) * 2;
                    w = .5f / S;
                    x = ( M[1] + M[3] ) / S;
                    y = ( M[2] + M[6] ) / S;
                    z = ( M[5] + M[7] ) / S;
                break;
            }
        }
    }

    /** Spherical interpolation. */
    GW_Quaternion GW_Quaternion::Slerp(GW_Quaternion& b, GW_Float t)
    {
        GW_Float omega, cosom, sinom, sclp, sclq;

        cosom = x*b.x + y*b.y + z*b.z + w*b.w;

        if ((1.0f+cosom) > GW_EPSILON)
        {
            if ((1.0f-cosom) > GW_EPSILON)
            {
                omega = (GW_Float) acos(cosom);
                sinom = (GW_Float) sin(omega);
                sclp = (GW_Float) sin((1.0f-t)*omega) / sinom;
                sclq = (GW_Float) sin(t*omega) / sinom;
            }
            else
            {
                sclp = 1.0f - t;
                sclq = t;
            }

            x = sclp*x + sclq*b.x;
            y = sclp*y + sclq*b.y;
            z = sclp*z + sclq*b.z;
            w = sclp*w + sclq*b.w;
        }
        else
        {
            x =-y;
            y = x;
            z =-w;
            w = z;

            sclp = (GW_Float) sin((1.0f-t) * GW_PI * 0.5);
            sclq = (GW_Float) sin(t * GW_PI * 0.5);

            x = sclp*x + sclq*b.x;
            y = sclp*y + sclq*b.y;
            z = sclp*z + sclq*b.z;
        }
        return *this;
    }

    /** Linear interpolation. */
    GW_Quaternion GW_Quaternion::Lerp(GW_Quaternion& to, GW_Float t)
    {
        return GW_Quaternion(w*(1-t) + to.w*t,x*(1-t) + to.x*t, y*(1-t) + to.y*t,
                            z*(1-t) + to.z*t).Normalize();
    }

    /** Compute the exponential. */
    GW_Quaternion& GW_Quaternion::Exp()
    {
        GW_Float mul;
        GW_Float length = x*x + y*y + z*z;

        if (length > GW_EPSILON)
            mul = (GW_Float) sin(length)/length;
        else
            mul = 1.0;

        w = (GW_Float) cos(length);

        x *= mul;
        y *= mul;
        z *= mul;

        return *this;
    }

    /** Compute the logarithm. */
    GW_Quaternion& GW_Quaternion::Log()
    {
        GW_Float length = x*x+y*y+z*z;
        length = (GW_Float) atan(length/w);

        w = 0.0;

        x *= length;
        y *= length;
        z *= length;

        return *this;
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_Quaternion", s);

        TestClassFooter("GW_Quaternion", s);
    }



protected:

    /** the fourth component of the quaterion */
    GW_Float w, x, y, z;

};

inline
std::ostream& operator<<(std::ostream &s, GW_Quaternion& m)
{
    s << "GW_Quaternion : {w=" << m.GetW() << ",x=" << m.GetX() << ",y=" << m.GetY() << ",z=" << m.GetZ() << "}";
    return s;
}


} // namespace GW

#endif /* #ifndef _GW_QUATERNION_H_ */

///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2009 Gabriel Peyre
