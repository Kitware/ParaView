/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicVertex.cpp
 *  \brief  Definition of class \c GW_GeodesicVertex
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_GeodesicVertex.cpp (c) Gabriel Peyré 2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_GeodesicVertex.h"
#include "GW_VoronoiVertex.h"

#ifndef GW_USE_INLINE
    #include "GW_GeodesicVertex.inl"
#endif

using namespace GW;



/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::SetStoppingVertex
/**
 *  \param  bIsStoppingVertex [GW_Bool] Yes/No
 *  \author Gabriel Peyré
 *  \date   4-26-2003
 *
 *  Set on/off stopping criterion.
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicVertex::SetStoppingVertex( GW_Bool bIsStoppingVertex )
{
    bIsStoppingVertex_ = bIsStoppingVertex;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::GetIsStoppingVertex
/**
 *  \return [GW_Bool] Stopping state.
 *  \author Gabriel Peyré
 *  \date   4-26-2003
 *
 *  Get the stopping vertex state.
 */
/*------------------------------------------------------------------------------*/
GW_Bool GW_GeodesicVertex::GetIsStoppingVertex()
{
    return bIsStoppingVertex_;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::AddParameterVertex
/**
 *  \param  VornoiVert [GW_VoronoiVertex&] The corresponding voronoi vertex.
 *  \param  rParam [GW_Float] The parameter.
 *  \author Gabriel Peyré
 *  \date   4-26-2003
 *
 *  Add a parameter value.
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicVertex::AddParameterVertex( GW_VoronoiVertex& VornoiVert, GW_Float rParam )
{
    if( pParameterVert_[0]==NULL )
    {
        pParameterVert_[0] = &VornoiVert;
        rParameter_[0] = rParam;
    }
    else if( pParameterVert_[1]==NULL )
    {
        pParameterVert_[1] = &VornoiVert;
        rParameter_[1] = rParam;
    }
    else if( pParameterVert_[2]==NULL )
    {
        pParameterVert_[2] = &VornoiVert;
        rParameter_[2] = rParam;
    }
    else
    {
//        GW_ASSERT(GW_False);
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::GetParameterVertex
/**
 *  \param  nNum [GW_U32] The number of the parameter.
 *  \param  rParam [GW_Float&] Return value : the parameter.
 *  \return [GW_VoronoiVertex*] The corresponding vertex.
 *  \author Gabriel Peyré
 *  \date   4-26-2003
 *
 *  Return the value of a given parameter.
 */
/*------------------------------------------------------------------------------*/
GW_VoronoiVertex* GW_GeodesicVertex::GetParameterVertex( GW_U32 nNum, GW_Float& rParam )
{
    GW_ASSERT( nNum<3 );
    rParam = rParameter_[nNum];
    return pParameterVert_[nNum];
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::SetParameterVertex
/**
 *  \param  nNum [GW_U32] The number of the parameter.
 *  \param  rParam [GW_Float] The value.
 *  \author Gabriel Peyré
 *  \date   4-27-2003
 *
 *  Set the value of a given parameter. You should first add the
 *  given vertex via \c
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicVertex::SetParameterVertex( GW_U32 nNum, GW_Float rParam )
{
    GW_ASSERT( nNum<3 );
    GW_ASSERT( pParameterVert_[nNum]!=NULL );
    rParameter_[nNum] = rParam;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::SetParameterVertex
/**
 *  \param  a [GW_Float] 1st param
 *  \param  b [GW_Float] 2nd param
 *  \param  c [GW_Float] 3rd param
 *  \author Gabriel Peyré
 *  \date   4-28-2003
 *
 *  Set the three parameters in one time.
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicVertex::SetParameterVertex( GW_Float a, GW_Float b, GW_Float c  )
{
    this->SetParameterVertex( 0, a );
    this->SetParameterVertex( 1, b );
    this->SetParameterVertex( 2, c );
}


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
