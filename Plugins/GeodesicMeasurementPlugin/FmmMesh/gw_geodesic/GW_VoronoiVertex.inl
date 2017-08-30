/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_VoronoiVertex.inl
 *  \brief  Inlined methods for \c GW_VoronoiVertex
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_VoronoiVertex.h"

namespace GW {


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex::AddNeighbor
/**
*  \param  Node [GW_VoronoiVertex&] The son node.
*  \author Gabriel Peyré
*  \date   5-15-2003
*
*  Add a new son to the list.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_VoronoiVertex::AddNeighbor( GW_VoronoiVertex& Node )
{
    GW_ASSERT( !(this->IsNeighbor(Node)) );
    NeighborList_.push_back( &Node );
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex::RemoveNeighbor
/**
*  \param  Node [GW_VoronoiVertex&] The son node.
*  \author Gabriel Peyré
*  \date   5-15-2003
*
*  Add a son to the list of sons.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_VoronoiVertex::RemoveNeighbor( GW_VoronoiVertex& Node )
{
    NeighborList_.remove( &Node );
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex::BeginNeighborIterator
/**
*  \return [IT_VoronoiVertexList] The iterator.
*  \author Gabriel Peyré
*  \date   5-15-2003
*
*  begin iterator on the son list.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
IT_VoronoiVertexList GW_VoronoiVertex::BeginNeighborIterator()
{
    return NeighborList_.begin();
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex::EndNeighborIterator
/**
*  \return [IT_VoronoiVertexList] The iterator.
*  \author Gabriel Peyré
*  \date   5-15-2003
*
*  end iterator on the son list.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
IT_VoronoiVertexList GW_VoronoiVertex::EndNeighborIterator()
{
    return NeighborList_.end();
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex::IsNeighbor
/**
 *  \param  Node [GW_VoronoiVertex&] The vertex.
 *  \return [GW_Bool] Answer
 *  \author Gabriel Peyré
 *  \date   5-15-2003
 *
 *  Is the given vertex a neighbor of us ?
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Bool GW_VoronoiVertex::IsNeighbor( GW_VoronoiVertex& Node )
{
    for( IT_VoronoiVertexList it = this->BeginNeighborIterator(); it!=this->EndNeighborIterator(); ++it )
    {
        GW_VoronoiVertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        if( pVert==&Node )
            return GW_True;
    }
    return GW_False;
}


} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
