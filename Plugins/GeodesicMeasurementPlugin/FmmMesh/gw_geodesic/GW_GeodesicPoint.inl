/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicPoint.inl
 *  \brief  Inlined methods for \c GW_GeodesicPoint
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_GeodesicPoint.h"

namespace GW {

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint constructor
/**
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicPoint::GW_GeodesicPoint()
:   pVert1_        ( NULL ),
    pVert2_        ( NULL ),
    rCoord_        ( 0 ),
    pCurFace_    ( NULL )
{
    /* NOTHING */
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint destructor
/**
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicPoint::~GW_GeodesicPoint()
{
    GW_SmartCounter::CheckAndDelete( pVert1_ );
    GW_SmartCounter::CheckAndDelete( pVert2_ );
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::SetVertex1
/**
 *  \param  Vert1 [GW_GeodesicVertex&] The first vertex.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Set the 1st vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicPoint::SetVertex1( GW_GeodesicVertex& Vert1 )
{
    GW_SmartCounter::CheckAndDelete( pVert1_ );
    pVert1_ = &Vert1;
    pVert1_->UseIt();
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::SetVertex2
/**
*  \param  Vert1 [GW_GeodesicVertex&] The 2nd vertex.
*  \author Gabriel Peyré
*  \date   4-10-2003
*
*  Set the 2nd vertex.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicPoint::SetVertex2( GW_GeodesicVertex& Vert2 )
{
    GW_SmartCounter::CheckAndDelete( pVert2_ );
    pVert2_ = &Vert2;
    pVert2_->UseIt();
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::SetCoord
/**
 *  \param  rCoord [GW_Float] The coordinate.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  The the barycentric coordinate of the point.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicPoint::SetCoord( GW_Float rCoord )
{
    GW_ASSERT( rCoord>=0 );
    GW_ASSERT( rCoord<=1 );
    rCoord_ = rCoord;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::GetVertex1
/**
 *  \return [GW_GeodesicVertex*] The 1st vertex.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Get the 1st vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicVertex* GW_GeodesicPoint::GetVertex1()
{
    return pVert1_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::GetVertex2
/**
*  \return [GW_GeodesicVertex*] The 2nd vertex.
*  \author Gabriel Peyré
*  \date   4-10-2003
*
*  Get the 2nd vertex.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicVertex* GW_GeodesicPoint::GetVertex2()
{
    return pVert2_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::GetCoord
/**
 *  \return [GW_Float] The coordinate.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Get the barycentric coordinate.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_GeodesicPoint::GetCoord()
{
    return rCoord_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::GetSubPointVector
/**
 *  \return [T_SubPointVector&] The vector.
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Get the vector of sub-points
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
T_SubPointVector& GW_GeodesicPoint::GetSubPointVector()
{
    return SubPointVector_;
}

} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
