/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicVertex.inl
 *  \brief  Inlined methods for \c GW_GeodesicVertex
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_GeodesicVertex.h"

namespace GW {

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex constructor
/**
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicVertex::GW_GeodesicVertex()
:    GW_Vertex    (),
    rDistance_    ( GW_INFINITE ),
    nState_        ( kFar ),
    pFront_        ( NULL ),
    bIsStoppingVertex_    ( GW_False ),
    bBoundaryReached_    ( GW_False )
{
    pParameterVert_[0] = pParameterVert_[1] = pParameterVert_[2] = NULL;
    rParameter_[0] = rParameter_[1] = rParameter_[2] = 0;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex destructor
/**
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicVertex::~GW_GeodesicVertex()
{
    /* NOTHING */
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::GetDistance
/**
 *  \return [GW_Float] Current distance. If the vertex is not "Dead", then it is not reliable.
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Get the current distance.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_GeodesicVertex::GetDistance()
{
    return rDistance_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::SetDistance
/**
 *  \param  rDistance [GW_Float] Current distance.
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Set the current distance.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicVertex::SetDistance( GW_Float rDistance )
{
    rDistance_ = rDistance;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::SetState
/**
 *  \param  nState [T_GeodesicVertexState] The new state.
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Set the state of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicVertex::SetState( T_GeodesicVertexState nState )
{
    nState_ = nState;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::GetState
/**
 *  \return [T_GeodesicVertexState] The current state.
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Return the current state of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicVertex::T_GeodesicVertexState GW_GeodesicVertex::GetState()
{
    return nState_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::GetFront
/**
 *  \return [GW_GeodesicVertex*] \c NULL if the vertex hasn't already been reached by a front.
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Get the vertex from which the front was started.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicVertex* GW_GeodesicVertex::GetFront()
{
    return pFront_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::SetFront
/**
 *  \param  pFront [GW_GeodesicVertex*] The vertex from which the front started.
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Set the front to which this vertex belongs.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicVertex::SetFront( GW_GeodesicVertex* pFront )
{
    pFront_ = pFront;
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::CompareVertex
/**
 *  \param  pVert1 [GW_GeodesicVertex*] 1st vertex.
 *  \param  pVert2 [GW_GeodesicVertex*] 2nd vertex.
 *  \return [GW_Bool] True if distance of the 1st mesh is < to the one of the 2nd.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Compare the distance of the 2 vertex. Used by the heap sorter.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Bool GW_GeodesicVertex::CompareVertex(GW_GeodesicVertex* pVert1, GW_GeodesicVertex* pVert2)
{
    return pVert1->GetDistance()>pVert2->GetDistance();
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::ResetGeodesicVertex
/**
 *  \author Gabriel Peyré
 *  \date   4-26-2003
 *
 *  Reset the datas for geodesic computations.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicVertex::ResetGeodesicVertex()
{
    rDistance_    = GW_INFINITE;
    nState_        = kFar;
    pFront_        = NULL;
    bIsStoppingVertex_    = GW_False;
    FrontOverlapInfo_.Reset();
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::ResetParametrizationData
/**
 *  \author Gabriel Peyré
 *  \date   4-27-2003
 *
 *  Reset only the data relative to parametrization.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicVertex::ResetParametrizationData()
{
    pParameterVert_[0] = pParameterVert_[1] = pParameterVert_[2] = NULL;
    rParameter_[0] = rParameter_[1] = rParameter_[2] = 0;
    bBoundaryReached_ = GW_False;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::SetBoundaryReached
/**
 *  \param  bBoundaryReached [GW_Bool] Was it reached ?
 *  \author Gabriel Peyré
 *  \date   4-30-2003
 *
 *  Is the vertex a boundary one reached by a parametrizing front ?
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicVertex::SetBoundaryReached( GW_Bool bBoundaryReached )
{
    bBoundaryReached_ = bBoundaryReached;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::GetBoundaryReached
/**
 *  \return [GW_Bool] Answer.
 *  \author Gabriel Peyré
 *  \date   4-30-2003
 *
 *  Reached on a boundary ?
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Bool GW_GeodesicVertex::GetBoundaryReached()
{
    return bBoundaryReached_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::ComputeFrontIntersection
/**
 *  \param  v1 [GW_GeodesicVertex&] 1st vertex.
 *  \param  v2 [GW_GeodesicVertex&] 2nd vertex.
 *  \param  pInter [GW_Vector3D*] Location of intersection.
 *  \param  pLambda [GW_Float*] Location in barycentric coord.
 *  \author Gabriel Peyré
 *  \date   5-30-2003
 *
 *  Compute the location of intersection of two front on a edge.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicVertex::ComputeFrontIntersection( GW_GeodesicVertex& v1, GW_GeodesicVertex& v2,
                                                   GW_Vector3D* pInter, GW_Float* pLambda )
{
    GW_GeodesicVertex* pFront1 = v1.GetFront();    GW_ASSERT( pFront1!=NULL );
    GW_GeodesicVertex* pFront2 = v2.GetFront();    GW_ASSERT( pFront2!=NULL );
    /* retrieve ovelap info */
    GW_Float d1 = v1.GetDistance();
    T_FrontOverlapInfo& info1 = v1.GetFrontOverlapInfo();
    GW_GeodesicVertex* pF1  = info1.pFront1_;
    GW_Float d2_1 = info1.rDist1_;
    if( pF1!=pFront2 )
    {
        pF1  = info1.pFront2_;
        d2_1 = info1.rDist2_;
    }

    GW_Float d2 = v2.GetDistance();
    T_FrontOverlapInfo& info2 = v2.GetFrontOverlapInfo();
    GW_GeodesicVertex* pF2  = info2.pFront1_;
    GW_Float d1_2 = info2.rDist1_;
    if( pF2!=pFront1 )
    {
        pF2  = info2.pFront2_;
        d1_2 = info2.rDist2_;
    }

    /* test if overlap is usable */
    if( pF1==pFront2 && pF2==pFront1 )
    {
        /* we search for a position x in [0,1] such that d1+x*(d1_2-d1)=d2_1+x*(d2-d2_1). Then barycentric coord is lambda=x/d */
        GW_Float lambda = 1 - (d2_1-d1)/(d1_2 + d2_1 - d1 - d2);
        GW_CLAMP( lambda, 0,1 );
        if( pLambda!=NULL )
            *pLambda = lambda;
        if( pInter!=NULL )
            *pInter = v1.GetPosition()*lambda + v2.GetPosition()*(1-lambda);
    }
    else    // use classical extrapolation
    {
//        cout << "missed" << endl;
        GW_GeodesicVertex::ComputeFrontIntersection( v1, d1, v2, d2, pInter, pLambda );
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicVertex::ComputeFrontIntersection
/**
*  \param  v1 [GW_GeodesicVertex&] 1st vertex.
*  \param  v2 [GW_GeodesicVertex&] 2nd vertex.
*  \param  pInter [GW_Vector3D*] Location of intersection.
*  \param  pLambda [GW_Float*] Location in barycentric coord.
*  \author Gabriel Peyré
*  \date   5-30-2003
*
*  Compute the location of intersection of two front on a edge.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicVertex::ComputeFrontIntersection( GW_GeodesicVertex& v1, GW_Float d1, GW_GeodesicVertex& v2, GW_Float d2, GW_Vector3D* pInter, GW_Float* pLambda )
{
    GW_Float d = ~( v1.GetPosition() - v2.GetPosition() );
    /* we search for a position x in [0,d] such that d1+x=d2+d-x. Then barycentric coord is lambda=x/d */
    GW_Float lambda = 1 - 0.5*(d+d2-d1)/d;
    GW_CLAMP( lambda, 0,1 );
    if( pLambda!=NULL )
        *pLambda = lambda;
    if( pInter!=NULL )
        *pInter = v1.GetPosition()*lambda + v2.GetPosition()*(1-lambda);
}



} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
