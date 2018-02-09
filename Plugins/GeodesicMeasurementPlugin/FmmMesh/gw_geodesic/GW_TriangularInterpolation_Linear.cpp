/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_TriangularInterpolation_Linear.cpp
 *  \brief  Definition of class \c GW_TriangularInterpolation_Linear
 *  \author Gabriel Peyré
 *  \date   5-5-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_TriangularInterpolation_Linear.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_TriangularInterpolation_Linear.h"

#ifndef GW_USE_INLINE
    #include "GW_TriangularInterpolation_Linear.inl"
#endif

using namespace GW;


/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Linear::ComputeGradient
/**
*  \param  v0 [GW_GeodesicVertex&] 1st vertex of local frame.
*  \param  v1 [GW_GeodesicVertex&] 2nd vertex.
*  \param  v2 [GW_GeodesicVertex&] 3rd vertex.
*  \param  x [GW_Float] x local coord.
*  \param  y [GW_Float] y local coord.
*  \param  dx [GW_Float&] x coord of the gradient in local coord.
*  \param  dy [GW_Float&] y coord of the gradient in local coord.
*  \author Gabriel Peyré
*  \date   5-2-2003
*
*  Compute the gradient at given point in local frame.
*/
/*------------------------------------------------------------------------------*/
void GW_TriangularInterpolation_Linear::ComputeGradient( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2, GW_Float x, GW_Float y, GW_Float& dx, GW_Float& dy )
{
    (void) x;
    (void) y;

    GW_Float d0 = v0.GetDistance();
    GW_Float d1 = v1.GetDistance();
    GW_Float d2 = v2.GetDistance();

    /* compute gradient */
    GW_Vector3D e0 = v0.GetPosition() - v2.GetPosition();
    GW_Vector3D e1 = v1.GetPosition() - v2.GetPosition();
    GW_Float l0 = e0.Norm();
    GW_Float l1 = e1.Norm();
    e0 /= l0;
    e1 /= l1;
    GW_Float dot = e0*e1;
    /* The gradient in direction (e1,e2) is:
            |<grad(d),e0>|   |(d0-d2)/l0|   |gu|
        D = |<grad(d),e1>| = |(d1-d2)/l1| = |gv|
    We are searching for grad(d) = dx e0 + dy e1 which gives rise to the system :
        | 1  dot|   |dx|
        |dot  1 | * |dy| = D            where dot=<e0,e2>
    ie it is:
        1/det    *    |  1 -dot|*|gu|
                    |-dot  1 | |gv|
    */
    GW_Float det = 1-dot*dot;
    GW_ASSERT( det!=0 );
    GW_Float gu = (d0-d2)/l0;
    GW_Float gv = (d1-d2)/l1;
    dx = 1/det * (     gu - dot*gv  );
    dy = 1/det * (-dot*gu +     gv  );
}



/*------------------------------------------------------------------------------*/
// Name : GW_TriangularInterpolation_Linear::ComputeValue
/**
*  \param  v0 [GW_GeodesicVertex&] 1st vertex of local frame.
*  \param  v1 [GW_GeodesicVertex&] 2nd vertex.
*  \param  v2 [GW_GeodesicVertex&] 3rd vertex.
*  \param  x [GW_Float] x local coord.
*  \param  y [GW_Float] y local coord.
*  \return  value of the distance function.
*  \author Gabriel Peyré
*  \date   5-2-2003
*
*  Compute the value at a given location in the triangle.
*/
/*------------------------------------------------------------------------------*/
GW_Float GW_TriangularInterpolation_Linear::ComputeValue( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2, GW_Float x, GW_Float y )
{
    return v0.GetDistance()*x + v1.GetDistance()*y + v2.GetDistance()*(1-x-y);
}

///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
