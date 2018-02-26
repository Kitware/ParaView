/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Vertex.inl
 *  \brief  Inlined methods for \c GW_Vertex
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_Vertex.h"

namespace GW {

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex constructor
/**
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vertex::GW_Vertex()
:    Position_    ( 0,0,0 ),
    Normal_        ( 0,0,1 ),
    CurvDirMin_    ( 1,0,0 ),
    CurvDirMax_    ( 0,1,0 ),
    rMinCurv_    ( 0 ),
    rMaxCurv_    ( 0 ),
    pFace_        ( NULL ),
    nID_        ( 0 ),
    pUserData_    ( NULL )
{
    TexCoords_[0] = TexCoords_[1] = 0;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex destructor
/**
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vertex::~GW_Vertex()
{
    if( pUserData_!=NULL )
        GW_SmartCounter::CheckAndDelete( pUserData_ );
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::SetPosition
/**
 *  \param  Position [GW_Vector3D&] The new position.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set the position of the vertex (in the local coords system of
 *  the mesh)
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Vertex::SetPosition( const GW_Vector3D& Position )
{
    Position_ = Position;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetPosition
/**
 *  \return [GW_Vector3D&] The position.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Return the position, in local frame.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vector3D& GW_Vertex::GetPosition()
{
    return Position_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::SetNormal
/**
 *  \param  Normal [GW_Vector3D&] The normal. Should not be face-based if you want to use gouraud shading.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set the normal to the mesh, on the location of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Vertex::SetNormal( GW_Vector3D& Normal )
{
    Normal_ = Normal;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetNormal
/**
 *  \return [GW_Vector3D&] The normal.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  The normal ot the mesh.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vector3D& GW_Vertex::GetNormal()
{
    return Normal_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::SetTexCoords
/**
 *  \param  u [GW_Float] U coord
 *  \param  v [GW_Float] V coord
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Texture coordinates. Should be in [O,1]
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Vertex::SetTexCoords( GW_Float u, GW_Float v )
{
    TexCoords_[0] = u;
    TexCoords_[1] = v;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::SetTexCoordU
/**
 *  \param  u [GW_Float] U coord
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Only set U coord
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Vertex::SetTexCoordU( GW_Float u )
{
    TexCoords_[0] = u;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::SetTexCoordV
/**
*  \param  v [GW_Float] V coord
*  \author Gabriel Peyré
*  \date   2-15-2003
*
*  Only set V coord
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Vertex::SetTexCoordV( GW_Float v )
{
    TexCoords_[1] = v;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetTexCoordU
/**
 *  \return [GW_Float] U coord
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get U component.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_Vertex::GetTexCoordU()
{
    return TexCoords_[0];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetTexCoordV
/**
 *  \return [GW_Float] V coord.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get V coord.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_Vertex::GetTexCoordV()
{
    return TexCoords_[1];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::void    SetID
/**
 *  \param  nID [GW_U32] New ID
 *  \author Gabriel Peyré
 *  \date   3-31-2003
 *
 *  Set the number of the vertex in the mesh.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Vertex::SetID(GW_U32 nID)
{
    nID_ = nID;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetID
/**
 *  \return [GW_U32] The ID
 *  \author Gabriel Peyré
 *  \date   3-31-2003
 *
 *  Get the ID of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_U32 GW_Vertex::GetID() const
{
    return nID_;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetMinCurv
/**
 *  \return [GW_Float] The value.
 *  \author Gabriel Peyré
 *  \date   4-3-2003
 *
 *  Get the minimum curvature value.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_Vertex::GetMinCurv()
{
    return rMinCurv_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetMaxCurv
/**
*  \return [GW_Float] The value.
*  \author Gabriel Peyré
*  \date   4-3-2003
*
*  Get the maximum curvature value.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_Vertex::GetMaxCurv()
{
    return rMaxCurv_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetGaussianCurv
/**
*  \return [GW_Float] The value.
*  \author Gabriel Peyré
*  \date   4-3-2003
*
*  Get the gaussian curvature value.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_Vertex::GetGaussianCurv()
{
    return rMaxCurv_*rMinCurv_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetMeanCurv
/**
*  \return [GW_Float] The value.
*  \author Gabriel Peyré
*  \date   4-3-2003
*
*  Get the mean curvature value.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_Vertex::GetMeanCurv()
{
    return (GW_Float) 0.5*(rMaxCurv_+rMinCurv_);
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetMaxAbsCurv
/**
*  \return [GW_Float] The value.
*  \author Gabriel Peyré
*  \date   4-3-2003
*
*  Get the maximum of absolute values of the max/min curvature.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_Vertex::GetMaxAbsCurv()
{
    return GW_MAX( GW_ABS(rMaxCurv_), GW_ABS(rMinCurv_));
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetMinCurvDirection
/**
 *  \return [GW_Vector3D&] The direction.
 *  \author Gabriel Peyré
 *  \date   4-3-2003
 *
 *  Get the minimum curvature direction.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vector3D& GW_Vertex::GetMinCurvDirection()
{
    return CurvDirMin_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetMaxCurvDirection
/**
*  \return [GW_Vector3D&] The direction.
*  \author Gabriel Peyré
*  \date   4-3-2003
*
*  Get the maximum curvature direction.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vector3D& GW_Vertex::GetMaxCurvDirection()
{
    return CurvDirMax_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::SetUserData
/**
 *  \param  pUserData [GW_SmartCounter*] The new used data.
 *  \author Gabriel Peyré
 *  \date   5-30-2003
 *
 *  Set a user data.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Vertex::SetUserData( GW_SmartCounter* pUserData )
{
    if( pUserData_!=NULL )
        GW_SmartCounter::CheckAndDelete( pUserData_ );
    pUserData_ = pUserData;
    if( pUserData_!=NULL )
        pUserData_->UseIt();
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::GetUserData
/**
 *  \return [GW_SmartCounter*] The user data.
 *  \author Gabriel Peyré
 *  \date   5-30-2003
 *
 *  Get the user data.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_SmartCounter* GW_Vertex::GetUserData()
{
    return pUserData_;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::ComputeUniqueId
/**
*  \param  Vert0 [GW_Vertex&] 1st vertex.
*  \param  Vert1 [GW_Vertex&] 2nd vertex.
*  \param  Vert2 [GW_Vertex&] 3rd vertex.
*  \author Gabriel Peyré
*  \date   4-13-2003
*
*  Compute an ID for the unordered 3-uplet.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_U32 GW_Vertex::ComputeUniqueId( const GW_Vertex& Vert0, const GW_Vertex& Vert1, const GW_Vertex& Vert2 )
{
    GW_U32 nId0 = Vert0.GetID();
    GW_U32 nId1 = Vert1.GetID();
    GW_U32 nId2 = Vert2.GetID();
    /* classify the Id */
    GW_ORDER( nId0, nId1 );
    GW_ORDER( nId1, nId2 );
    GW_ORDER( nId0, nId1 );
    return (GW_U32) nId0 + (nId1<<10) + (nId2<<20);
}

/*------------------------------------------------------------------------------*/
// Name : GW_Vertex::ComputeUniqueId
/**
*  \param  Vert0 [GW_Vertex&] 1st vertex.
*  \param  Vert1 [GW_Vertex&] 2nd vertex.
*  \author Gabriel Peyré
*  \date   4-13-2003
*
*  Compute an ID for the unordered 2-uplet.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_U32 GW_Vertex::ComputeUniqueId( const GW_Vertex& Vert0, const GW_Vertex& Vert1 )
{
    GW_U32 nId0 = Vert0.GetID();
    GW_U32 nId1 = Vert1.GetID();
    /* classify the Id */
    GW_ORDER( nId0, nId1 );
    return (GW_U32) nId0 + (nId1<<15);
}




} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
