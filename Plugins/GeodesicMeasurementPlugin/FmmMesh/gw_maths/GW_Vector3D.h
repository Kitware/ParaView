/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_Vector3D.h
 *  \brief Definition of class \c GW_Vector3D
 *  \author Gabriel Peyré 2001-09-10
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_Vector3D_h
#define GW_Vector3D_h

#include "GW_MathsConfig.h"
#include "GW_VectorStatic.h" // here
#include "GW_Vector2D.h"

GW_BEGIN_NAMESPACE

/** Forward definition **/
class GW_Vector3D;
GW_Vector3D operator^( const GW_VectorStatic<3,GW_Float>& v1, const GW_VectorStatic<3,GW_Float>& v2);

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Vector3D
 *  \brief  A 3D vector, with useful operators and methods.
 *  \author Gabriel Peyré 2001-09-10
 *
 *    This class is used every where ...
 *    A constructor makes the conversion GW_Float[3] -> GW_Vector3D
 *    A lot of operator have been defined to make common maths operations, so use them !
 */
/*------------------------------------------------------------------------------*/
class GW_Vector3D:    public GW_VectorStatic<3,GW_Float>
{

public:

    GW_Vector3D()                            :GW_VectorStatic<3,GW_Float>()    {}
    GW_Vector3D( const GW_VectorStatic<3,GW_Float>& v )        :GW_VectorStatic<3,GW_Float>(v)    {}
    GW_Vector3D( GW_Float a )                :GW_VectorStatic<3,GW_Float>(a)    {}
    GW_Vector3D( GW_Float a, GW_Float b, GW_Float c )
    {
        aCoords_[0] = a;
        aCoords_[1] = b;
        aCoords_[2] = c;
    }
    GW_Vector3D( GW_Vector2D& v )
    {
        aCoords_[0] = v[0];
        aCoords_[1] = v[1];
        aCoords_[2] = 0;
    }
    /** copy operator */
    void SetCoord( GW_Float a, GW_Float b, GW_Float c )
    {
        aCoords_[0] = a;
        aCoords_[1] = b;
        aCoords_[2] = c;
    }

    GW_Vector2D ToVector2D()
    {
        return GW_Vector2D(aCoords_[0], aCoords_[1]);
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_Vector3D", s);
        GW_Vector3D v1;
        v1.Randomize();
        v1.Normalize();
        GW_ASSERT( GW_ABS(~v1 - 1)<GW_EPSILON );
        GW_ASSERT( ~(v1^v1)<GW_EPSILON );
        GW_Vector3D v2;
        v2.Randomize();
        GW_ASSERT( GW_ABS((v1^v2)*v2)<GW_EPSILON );
        GW_ASSERT( GW_ABS((v1^v2)*v1)<GW_EPSILON );
        GW_Vector3D v3 = v2^v1;
        v3.Normalize();
        v2 = v3^v1;
        v2.Normalize();
        GW_Vector3D v;
        v.Randomize();
        GW_Vector3D w = v1*(v*v1) + v2*(v*v2) + v3*(v*v3);
        GW_ASSERT( ~(v-w) < GW_EPSILON );
        TestClassFooter("GW_Vector3D", s);
    }
};

/** Cross Product
    @return The cross product between this and \a V
    @param V second vector of the cross product
**/
inline
GW_Vector3D operator^( const GW_VectorStatic<3,GW_Float>& v1, const GW_VectorStatic<3,GW_Float>& v2)
{
    return GW_Vector3D(    v1[1]*v2[2] - v1[2]*v2[1],
                        v1[2]*v2[0] - v1[0]*v2[2],
                        v1[0]*v2[1] - v1[1]*v2[0]);
}


GW_END_NAMESPACE



#endif // GW_Vector3D_h

///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
