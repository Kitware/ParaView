/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_VoronoiVertex.cpp
 *  \brief  Definition of class \c GW_VoronoiVertex
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_VoronoiVertex.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_VoronoiVertex.h"

#ifndef GW_USE_INLINE
    #include "GW_VoronoiVertex.inl"
#endif

using namespace GW;

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex constructor
/**
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_VoronoiVertex::GW_VoronoiVertex()
:    GW_Vertex        (),
    pBaseVertex_    ( NULL )
{
    /* NOTHING */
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex destructor
/**
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_VoronoiVertex::~GW_VoronoiVertex()
{
    GW_SmartCounter::CheckAndDelete( pBaseVertex_ );
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex::GetBaseVertex
/**
 *  \return [GW_Vertex*] The base vertex.
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 *
 *  Return the vertex on the original mesh.
 */
/*------------------------------------------------------------------------------*/
GW_GeodesicVertex* GW_VoronoiVertex::GetBaseVertex()
{
    return pBaseVertex_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex::GetBaseVertex
/**
*  \return [GW_Vertex*] The base vertex.
*  \author Gabriel Peyré
*  \date   4-19-2003
*
*  Return the vertex on the original mesh.
*/
/*------------------------------------------------------------------------------*/
const GW_GeodesicVertex* GW_VoronoiVertex::GetBaseVertex() const
{
    return pBaseVertex_;
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiVertex::SetBaseVertex
/**
 *  \param  pBaseVertex [GW_Vertex] The base vertex.
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 *
 *  Set the base vertex.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiVertex::SetBaseVertex( GW_GeodesicVertex& BaseVertex )
{
    GW_SmartCounter::CheckAndDelete( pBaseVertex_ );
    pBaseVertex_ = &BaseVertex;
    pBaseVertex_->UseIt();
    /* copy the parameters of the base mesh */
    this->SetPosition(    BaseVertex.GetPosition() );
    this->SetNormal(    BaseVertex.GetNormal() );
}


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
