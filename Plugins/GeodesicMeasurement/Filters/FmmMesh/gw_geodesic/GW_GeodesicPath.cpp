#include "stdafx.h"
#include "GW_GeodesicPath.h"

#ifndef GW_USE_INLINE
    #include "GW_GeodesicPath.inl"
#endif

using namespace GW;

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath::AddVertexToPath
/**
 *  \param  Vert [GW_GeodesicVertex&] The vertex.
 *
 *  Helper method : add a vertex to path and compute next face.
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicPath::AddVertexToPath( GW_GeodesicVertex& Vert )
{
    pPrevFace_ = pCurFace_;
    GW_Float rBestDistance = GW_INFINITE;
    pCurFace_ = NULL;
    GW_GeodesicVertex* pSelectedVert = NULL;
    for( GW_VertexIterator it = Vert.BeginVertexIterator(); it!=Vert.EndVertexIterator(); ++it )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*)  *it;
        if( pVert->GetDistance()<rBestDistance )
        {
            rBestDistance = pVert->GetDistance();
            pSelectedVert = pVert;
            GW_GeodesicVertex* pVert1 = (GW_GeodesicVertex*) it.GetLeftVertex();
            GW_GeodesicVertex* pVert2 = (GW_GeodesicVertex*) it.GetRightVertex();
            if( pVert1!=NULL && pVert2!=NULL )
            {
                if( pVert1->GetDistance()<pVert2->GetDistance() )
                    pCurFace_ = (GW_GeodesicFace*) it.GetLeftFace();
                else
                    pCurFace_ = (GW_GeodesicFace*) it.GetRightFace();
            }
            else if( pVert1!=NULL )
            {
                pCurFace_ = (GW_GeodesicFace*) it.GetLeftFace();
            }
            else
            {
                GW_ASSERT( pVert2!=NULL );
                pCurFace_ = (GW_GeodesicFace*) it.GetRightFace();
            }
        }
    }
    GW_ASSERT( pCurFace_!=NULL );
    GW_ASSERT( pSelectedVert!=NULL );

    GW_GeodesicPoint* pPoint = new GW_GeodesicPoint;
    Path_.push_back( pPoint );
    pPoint->SetVertex1( Vert );
    pPoint->SetVertex2( *pSelectedVert );
    pPoint->SetCoord(1);
    pPoint->SetCurFace( *pCurFace_ );
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath::InitPath
/**
 *  \param  StartVert [GW_GeodesicVertex&] Starting point of the path.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Compute the first face to begin the search.
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicPath::InitPath( GW_GeodesicVertex& StartVert )
{
    this->ResetPath();
    this->AddVertexToPath( StartVert );
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath::AddNewPoint
/**
 *  \return [GW_I32] >0 if the path is ended.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Compute a new point and add it to the path.
 */
/*------------------------------------------------------------------------------*/
GW_I32 GW_GeodesicPath::AddNewPoint()
{
//    rStepSize_ = 1000;
    GW_ASSERT( pCurFace_!=NULL );
    GW_ASSERT( !Path_.empty() );
    GW_GeodesicPoint* pPoint = Path_.back();
    GW_ASSERT( pPoint!=NULL );
    GW_GeodesicVertex* pVert1 = pPoint->GetVertex1();
    GW_ASSERT( pVert1!=NULL );
    GW_GeodesicVertex* pVert2 = pPoint->GetVertex2();
    GW_ASSERT( pVert2!=NULL );
    GW_GeodesicVertex* pVert3 = (GW_GeodesicVertex*) pCurFace_->GetVertex( *pVert1, *pVert2 );
    GW_ASSERT( pVert3!=NULL );

    /* compute coords of the last point */
    T_SubPointVector& SubPointVector = pPoint->GetSubPointVector();
    GW_Float x,y,z;    // barycentric coords of the point
    /* the sub-points should be empty */
    GW_ASSERT( SubPointVector.empty() );
    /* we begin on an edge */
    x = pPoint->GetCoord();
    y = 1-x;
    z = 0;

    GW_Float l1 = ~( pVert1->GetPosition() - pVert3->GetPosition() );
    GW_Float l2 = ~( pVert2->GetPosition() - pVert3->GetPosition() );

    pCurFace_->SetUpTriangularInterpolation();

    GW_U32 nNum = 0;
    while( nNum<1000 )    // never stop, this is just to avoid infinite loop
    {
        nNum++;
        GW_Float dx, dy;

        pCurFace_->ComputeGradient( *pVert1, *pVert2, *pVert3, x, y, dx, dy );

        GW_Float l, a;
        /* Try each kind of possible crossing.
           The barycentric coords of the point is (x-l*dx/l1,y-l*dy/l2,z+l*(dx/l1+dy/l2)) */
        if( GW_ABS(dx)>GW_EPSILON )
        {
            l = l1*x/dx;        // position along the line
            a = y-l*dy/l2;        // coordinate with respect to v2
            if( l>0 && l<=rStepSize_ && 0<=a && a<=1 )
            {
                /* the crossing occurs on [v2,v3] */
                GW_GeodesicPoint* pNewPoint = new GW_GeodesicPoint;
                Path_.push_back( pNewPoint );
                pNewPoint->SetVertex1( *pVert2 );
                pNewPoint->SetVertex2( *pVert3 );
                pNewPoint->SetCoord( a );
                pPrevFace_ = pCurFace_;
                if( pCurFace_->GetFaceNeighbor( *pVert2, *pVert3 )==NULL )
                {
                    pNewPoint->SetCurFace( *pCurFace_ );
                    /* check if we are *really* on a boundarie of the mesh */
                    GW_ASSERT( pCurFace_->GetEdgeNumber( *pVert2, *pVert3 )>=0 );
                    /* we should stay on the same face */
                    return 0;
                }
                pCurFace_ = (GW_GeodesicFace*) pCurFace_->GetFaceNeighbor( *pVert2, *pVert3 );
                GW_ASSERT( pCurFace_!=NULL );
                pNewPoint->SetCurFace( *pCurFace_ );
                /* test for ending */
                if( a<0.01 && pVert2->GetDistance()<GW_EPSILON )
                    return -1;
                if( a>0.99 && pVert3->GetDistance()<GW_EPSILON )
                    return -1;
                return 0;
            }
        }
        if( (GW_ABS(dy)>GW_EPSILON) != 0 )
        {
            l = l2*y/dy;      // position along the line
            a = x-l*dx/l1;      // coordinate with respect to v1
            if( l>0 && l<=rStepSize_ && 0<=a && a<=1 )
            {
                /* the crossing occurs on [v1,v3] */
                GW_GeodesicPoint* pNewPoint = new GW_GeodesicPoint;
                Path_.push_back( pNewPoint );
                pNewPoint->SetVertex1( *pVert1 );
                pNewPoint->SetVertex2( *pVert3 );
                pNewPoint->SetCoord( a );
                pPrevFace_ = pCurFace_;
                if( pCurFace_->GetFaceNeighbor( *pVert1, *pVert3 )==NULL )
                {
                    pNewPoint->SetCurFace( *pCurFace_ );
                    /* check if we are *really* on a boundarie of the mesh */
                    GW_ASSERT( pCurFace_->GetEdgeNumber( *pVert1, *pVert3 )>=0 );
                    /* we should stay on the same face, the fact that pPrevFace_==pCurFace_
                       will force to go on an edge */
                    return 0;
                }
                pCurFace_ = (GW_GeodesicFace*) pCurFace_->GetFaceNeighbor( *pVert1, *pVert3 );
                GW_ASSERT( pCurFace_!=NULL );
                pNewPoint->SetCurFace( *pCurFace_ );
                /* test for ending */
                if( a<0.01 && pVert1->GetDistance()<GW_EPSILON )
                    return -1;
                if( a>0.99 && pVert3->GetDistance()<GW_EPSILON )
                    return -1;
                return 0;
            }
        }
        if( GW_ABS(dx/l1+dy/l2)>GW_EPSILON )
        {
            l = -z/(dx/l1+dy/l2);      // position along the line
            a = x-l*dx/l1;      // coordinate with respect to v1
            if( l>0 && l<=rStepSize_ && 0<=a && a<=1 )
            {
                /* the crossing occurs on [v1,v2] */
                GW_GeodesicPoint* pNewPoint = new GW_GeodesicPoint;
                Path_.push_back( pNewPoint );
                pNewPoint->SetVertex1( *pVert1 );
                pNewPoint->SetVertex2( *pVert2 );
                pNewPoint->SetCoord( a );
                pPrevFace_ = pCurFace_;
                if( pCurFace_->GetFaceNeighbor( *pVert1, *pVert2 )==NULL )
                {
                    pNewPoint->SetCurFace( *pCurFace_ );
                    /* check if we are *really* on a boundarie of the mesh */
                    GW_ASSERT( pCurFace_->GetEdgeNumber( *pVert1, *pVert2 )>=0 );
                    /* we should stay on the same face */
                    return 0;
                }
                pCurFace_ = (GW_GeodesicFace*) pCurFace_->GetFaceNeighbor( *pVert1, *pVert2 );
                GW_ASSERT( pCurFace_!=NULL );
                pNewPoint->SetCurFace( *pCurFace_ );
                /* test for ending */
                if( a<0.01 && pVert1->GetDistance()<GW_EPSILON )
                    return -1;
                if( a>0.99 && pVert2->GetDistance()<GW_EPSILON )
                    return -1;
                return 0;
            }
        }

        if( GW_ABS(dx)<GW_EPSILON && GW_ABS(dx)<GW_EPSILON  )
        {
            /* special case : we must follow the edge. */
            GW_GeodesicVertex* pSelectedVert = pVert1;
            if( pVert2->GetDistance()<pSelectedVert->GetDistance() )
                pSelectedVert = pVert2;
            if( pVert3->GetDistance()<pSelectedVert->GetDistance() )
                pSelectedVert = pVert3;
            // just a check
            this->AddVertexToPath( *pSelectedVert );
            GW_ASSERT( pCurFace_!=NULL );
            if( pSelectedVert->GetDistance()<GW_EPSILON )
                return -1;
            if( pCurFace_==pPrevFace_ && pPoint->GetCoord()>1-GW_EPSILON )
            {
                /* hum, problem, we are in a local minimum */
                return -1;
            }
            return 0;
        }

        /* no intersection: we can advance */
        GW_Float xprev = x;
        x = x - rStepSize_*dx/l1;
        y = y - rStepSize_*dy/l2;

        if( x<0 || x>1 || y<0 || y>1 )
        {
            GW_ASSERT( z==0 );

            GW_GeodesicFace* pNextFace = (GW_GeodesicFace*) pCurFace_->GetFaceNeighbor( *pVert3 );

            if( pNextFace==pPrevFace_ || pCurFace_->GetFaceNeighbor( *pVert3 )==NULL )
            {
                /* special case : we must follow the edge. */
                GW_GeodesicVertex* pSelectedVert = pVert1;
                if( pVert2->GetDistance()<pSelectedVert->GetDistance() )
                    pSelectedVert = pVert2;
                if( pVert3->GetDistance()<pSelectedVert->GetDistance() )
                    pSelectedVert = pVert3;
                // just a check
                this->AddVertexToPath( *pSelectedVert );
                GW_ASSERT( pCurFace_!=NULL );
                if( pSelectedVert->GetDistance()<GW_EPSILON )
                    return -1;
                if( pCurFace_==pPrevFace_ && pPoint->GetCoord()>1-GW_EPSILON )
                {
                    /* hum, problem, we are in a local minimum */
                    return -1;
                }
                //    if( pSelectedVert==pVert3 )
                //        GW_ASSERT( x==0 || y==0 );
            }
            else
            {
                /* we should go on another face */
                pPrevFace_ = pCurFace_;
                pCurFace_ = (GW_GeodesicFace*) pCurFace_->GetFaceNeighbor( *pVert3 );
                GW_ASSERT( pCurFace_!=NULL );
                GW_GeodesicPoint* pNewPoint = new GW_GeodesicPoint;
                Path_.push_back( pNewPoint );
                pNewPoint->SetVertex1( *pVert1 );
                pNewPoint->SetVertex2( *pVert2 );
                pNewPoint->SetCurFace( *pCurFace_ );
                pNewPoint->SetCoord( xprev );
            }
            return 0;
        }

        SubPointVector.push_back( GW_Vector3D(x,y,1-x-y) );
    }
    GW_GeodesicVertex* pSelectedVert = pVert1;
    if( pVert2->GetDistance()<pSelectedVert->GetDistance() )
        pSelectedVert = pVert2;
    if( pVert3->GetDistance()<pSelectedVert->GetDistance() )
        pSelectedVert = pVert3;
    // just a check
    this->AddVertexToPath( *pSelectedVert );
    GW_ASSERT( pCurFace_!=NULL );
    if( pSelectedVert->GetDistance()<GW_EPSILON )
        return -1;
    if( pCurFace_==pPrevFace_ && pPoint->GetCoord()>1-GW_EPSILON )
    {
        /* hum, problem, we are in a local minimum */
        return -1;
    }
    return 0;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath::ComputePath
/**
 *  \param  StartVert [GW_GeodesicVertex&] The starting point.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Compute the whole path.
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicPath::ComputePath( GW_GeodesicVertex& StartVert, GW_U32 nMaxLength )
{
    this->InitPath( StartVert );
    GW_U32 nNum = 0;
    while( this->AddNewPoint()==0 && nNum<nMaxLength )
    { nNum++; }
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath::ResetPath
/**
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Clear everything in the path.
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicPath::ResetPath()
{
    for( IT_GeodesicPointList it=Path_.begin(); it!=Path_.end(); ++it )
    {
        GW_GeodesicPoint* pPoint = *it;
        GW_DELETE( pPoint );
        *it = NULL;
    }
    Path_.clear();
}



///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
