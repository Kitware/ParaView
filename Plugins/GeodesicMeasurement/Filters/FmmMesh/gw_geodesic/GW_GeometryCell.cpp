/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeometryCell.cpp
 *  \brief  Definition of class \c GW_GeometryCell
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_GeometryCell.cpp(c) Gabriel Peyré2004";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_GeometryCell.h"

#ifndef GW_USE_INLINE
    #include "GW_GeometryCell.inl"
#endif

using namespace GW;

/*------------------------------------------------------------------------------*/
// Name : GW_GeometryCell::InitSampling
/**
 *  \param  v1 [GW_Vector3D&] 1st vert
 *  \param  v2 [GW_Vector3D&] 2nd corner
 *  \param  v3 [GW_Vector3D&] 3rd corner
 *  \param  v4 [GW_Vector3D&] 4th corner.
 *  \param  n [GW_U32] Number of points on width.
 *  \param  n [GW_U32] Number of points on height.
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  Initialize the position of each point.
 */
/*------------------------------------------------------------------------------*/
void GW_GeometryCell::InitSampling( GW_Vector3D& v1, GW_Vector3D& v2, GW_Vector3D& v3, GW_Vector3D& v4, GW_U32 n, GW_U32 p )
{
    this->Reset(n,p);
    for( GW_U32 i=0; i<n; ++i )
    {
        GW_Float x = ((GW_Float)i)/((GW_Float) (n-1));
        for( GW_U32 j=0; j<p; ++j )
        {
            GW_Float y = ((GW_Float)j)/((GW_Float) (p-1));
            GW_Vector3D pos = (v1*(1-x)+v2*x)*(1-y) + (v4*(1-x)+v3*x)*y;
            this->SetData(i,j, pos);
        }
    }
}



///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
