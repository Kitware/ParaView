/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_VoronoiMesh.inl
 *  \brief  Inlined methods for \c GW_VoronoiMesh
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_VoronoiMesh.h"
#include "../gw_core/GW_PolygonIntersector.h"

namespace GW {

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh constructor
/**
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_VoronoiMesh::GW_VoronoiMesh()
:    GW_Mesh(),
    bInterpolationPreparationDone_( GW_False )
{
    /* NOTHING */
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh destructor
/**
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_VoronoiMesh::~GW_VoronoiMesh()
{
    /* delete the map of path */
    for( IT_VertexPathMap it = VertexPathMap_.begin(); it!=VertexPathMap_.end(); ++it )
    {
        T_GeodesicVertexList* pList = it->second;
        GW_ASSERT( pList!=NULL );
        GW_DELETE( pList );
    }
    VertexPathMap_.clear();
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::GetBaseVertexList
/**
 *  \return [T_GeodesicVertexList&] The list.
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Get the list of base points.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
T_GeodesicVertexList& GW_VoronoiMesh::GetBaseVertexList()
{
    return BaseVertexList_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::Reset
/**
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Clear the list of points.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_VoronoiMesh::Reset()
{
    GW_Mesh::Reset();
    BaseVertexList_.clear();
    for( IT_VertexPathMap it = VertexPathMap_.begin(); it!=VertexPathMap_.end(); ++it )
    {
        T_GeodesicVertexList* pVertList = it->second;
        GW_DELETE(pVertList);
    }
    VertexPathMap_.clear();
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::GetNbrBasePoints
/**
 *  \return [GW_U32] The number.
 *  \author Gabriel Peyré
 *  \date   4-13-2003
 *
 *  Get the number of base points.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_U32 GW_VoronoiMesh::GetNbrBasePoints()
{
    return (GW_U32) BaseVertexList_.size();
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::GetGeodesicBoundariesMap
/**
 *  \return [T_VertexPathMap&] The map.
 *  \author Gabriel Peyré
 *  \date   4-23-2003
 *
 *  Get the map containing all geodesic boundaries.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
T_VertexPathMap& GW_VoronoiMesh::GetGeodesicBoundariesMap()
{
    return VertexPathMap_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::InterpolatePosition
/**
*  \param  Position Answer : the position.
*  \param  v0 [GW_VoronoiVertex&] 1st vertex parameter.
*  \param  v1 [GW_VoronoiVertex&] 2nd vertex parameter
*  \param  v2 [GW_VoronoiVertex&] 3rd vertex parameter.
*  \param  p0 [GW_Float] 1st value parameter (barycentric coord).
*  \param  p1 [GW_Float] 2nd value parameter (barycentric coord).
*  \param  p2 [GW_Float] 3rd value parameter (barycentric coord).
*  \param  pGuess [GW_Face*] A guess of the surrounding face.
*  \author Gabriel Peyré
*  \date   4-30-2003
*
*  Interpolate the position for a given value of parameters
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_VoronoiMesh::InterpolatePosition( GW_GeodesicMesh& Mesh, GW_Vector3D& Position, GW_VoronoiVertex& v0, GW_VoronoiVertex& v1, GW_VoronoiVertex& v2,
                                         GW_Float a, GW_Float b, GW_Float c )
{
    /* find a guess to start */
    GW_U32 nID = GW_Vertex::ComputeUniqueId( v0, v1, v2 );
    GW_ASSERT( CentralParameterMap_.find(nID)!=CentralParameterMap_.end() );
    GW_GeodesicVertex* pVert = CentralParameterMap_[nID];
    GW_ASSERT( pVert!=NULL );
    if( pVert==NULL )
        return;
    GW_Face* pGuess = pVert->GetFace();
    GW_Face* pPrevGuess = NULL;

    GW_Float x, y, z;

    /* order the vertex by increasing ID */
    const GW_VoronoiVertex* pVParam0 = &v0, *pVParam1 = &v1, *pVParam2 = &v2;

    /* now search for "the" face */
    GW_GeodesicVertex* pVert0, *pVert1, *pVert2;
    GW_Face* pSelectedFace = NULL;
    GW_U32 nNumIter = 0;    // just to avoid infinite loop.
    while( pSelectedFace==NULL )
    {
        pVert0 = (GW_GeodesicVertex*) pGuess->GetVertex(0);
        pVert1 = (GW_GeodesicVertex*) pGuess->GetVertex(1);
        pVert2 = (GW_GeodesicVertex*) pGuess->GetVertex(2);

        /* parameters of the surrounding face */
        GW_Float a0, b0, c0,
                 a1, b1, c1,
                 a2, b2, c2;
        GW_Bool bValid = GW_True;
        bValid = bValid & GW_VoronoiMesh::GetParameterVertex( *pVert0, a0, b0, c0, *pVParam0, *pVParam1, *pVParam2 );
        bValid = bValid & GW_VoronoiMesh::GetParameterVertex( *pVert1, a1, b1, c1, *pVParam0, *pVParam1, *pVParam2 );
        bValid = bValid & GW_VoronoiMesh::GetParameterVertex( *pVert2, a2, b2, c2, *pVParam0, *pVParam1, *pVParam2 );

        /* translate coordinate in global param into coord in local face param */
        GW_I32 nRet = GW_Maths::ComputeLocalGeodesicParameter( x, y, z,      a, b, c,
                                                a0, b0, c0,   a1, b1, c1,   a2, b2, c2 );

        if( nRet<0 || !bValid )
        {
            /* the conversion was impossible */
            GW_VoronoiMesh::InterpolatePositionExhaustiveSearch( Mesh, Position, v0, v1, v2, a, b, c );
            return;
        }
        else if( x<0 )
            pGuess = pGuess->GetFaceNeighbor( *pVert0 );
        else if( y<0 )
            pGuess = pGuess->GetFaceNeighbor( *pVert1 );
        else if( z<0 )
            pGuess = pGuess->GetFaceNeighbor( *pVert2 );
        else
        {
            /* yes, this is it ! */
            pSelectedFace = pGuess;
        }
        GW_ASSERT( pGuess!=NULL );

        nNumIter++;
        if( nNumIter>200  )
        {
//            GW_ASSERT( GW_False );
            /* use brute force method */
            GW_VoronoiMesh::InterpolatePositionExhaustiveSearch( Mesh, Position, v0, v1, v2, a, b, c );
            return;
        }

        pPrevGuess = pGuess;
    }

    /* this face is the correct one */
    if( x>1.1 || y>1.1 || z>1.1 || x<-0.1 || y<-0.1 || z<-0.1 )
    {
        GW_CLAMP(x, 0,1);
        GW_CLAMP(y, 0,1);
        z = 1-x-y;
    }
    Position = pVert0->GetPosition()*x + pVert1->GetPosition()*y + pVert2->GetPosition()*z;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::GetParameterVertex
/**
 *  \param  Vert [GW_GeodesicVertex&] Vertex.
 *  \param  a [GW_Float&] 1st param.
 *  \param  b [GW_Float&] 2nd param.
 *  \param  c [GW_Float&] 3rd param.
 *  \param  ParamV0 [GW_VoronoiVertex&] 1st vertex param.
 *  \param  ParamV1 [GW_VoronoiVertex&] 2nd vertex param.
 *  \param  ParamV2 [GW_VoronoiVertex&] 3rd vertex param.
 *    \return Is the result valid (e.g. it is false if the vertex doesn't belongs to the corresponding face)
 *  \author Gabriel Peyré
 *  \date   5-1-2003
 *
 *  Helper method : retrieve the parameters of a vertex, taking care
 *  of boundary vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Bool GW_VoronoiMesh::GetParameterVertex( GW_GeodesicVertex& Vert, GW_Float& a, GW_Float& b, GW_Float& c, const GW_VoronoiVertex& ParamV0, const GW_VoronoiVertex& ParamV1, const GW_VoronoiVertex& ParamV2 )
{
    GW_Float unordered_coords[3];
    GW_Float ordered_coords[3];
    const GW_VoronoiVertex* pParamV[3] = {&ParamV0, &ParamV1, &ParamV2};
    GW_VoronoiVertex* pV[3];

    /* store the parameter value and vertex */
    for( GW_U32 i=0; i<3; ++i )
    {
        pV[i] = Vert.GetParameterVertex( i, unordered_coords[i] );
        if( pV[i]==NULL )
            unordered_coords[i] = -GW_INFINITE;
    }

    GW_I32 nConnexion[3] = {-1,-1,-1};

    /* set up the connexion */
    for( GW_U32 i=0; i<3; ++i )
    {
        if( pParamV[i]==pV[0] )
            nConnexion[i] = 0;
        else if( pParamV[i]==pV[1] )
            nConnexion[i] = 1;
        else if( pParamV[i]==pV[2] )
            nConnexion[i] = 2;
    }

    /* computed the ordered coords */
    for( GW_U32 i=0; i<3; ++i )
    {
        if( nConnexion[i]!=-1 )
            ordered_coords[i] = unordered_coords[nConnexion[i]];
        else
        {
            /* special case */
            GW_U32 i1 = (i+1)%3;
            GW_U32 i2 = (i+2)%3;
            if( nConnexion[i1]!=-1 && nConnexion[i2]!=-1 )
            {
            //    return GW_False;
                GW_U32 nTarget = 3 - nConnexion[i1] - nConnexion[i2];
                if( GW_ABS(unordered_coords[nTarget])>GW_EPSILON )
                {
                    a = b = c = -GW_INFINITE;
                    return GW_False;
                }
                else
                    ordered_coords[i] = 0;
            }
            else
            {
                a = b = c = -GW_INFINITE;
                return GW_False;
            }
        }
    }

    a = ordered_coords[0];
    b = ordered_coords[1];
    c = ordered_coords[2];

    /* rescale if necessary */
    GW_Float s = a+b+c;
    if( s<=0 )
        return GW_False;
    if( s!=1 && s!=0 )
    {
        a /= s; b /= s; c /= s;
    }
    return GW_True;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::GetVoronoiFromGeodesic
/**
 *  \param  Vert [GW_GeodesicVertex&] The geodesic vertex.
 *  \return [GW_VoronoiVertex*] The voronoi vertex.
 *  \author Gabriel Peyré
 *  \date   5-15-2003
 *
 *  Find the voronoi vertex corresponding to the geodesic vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_VoronoiVertex* GW_VoronoiMesh::GetVoronoiFromGeodesic( GW_GeodesicVertex& Vert )
{
    if( VoronoiVertexMap_.find(Vert.GetID())==VoronoiVertexMap_.end() )
        return NULL;
    return VoronoiVertexMap_[Vert.GetID()];
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::InterpolatePositionExhaustiveSearch
/**
*  \param  Position Answer : the position.
*  \param  v0 [GW_VoronoiVertex&] 1st vertex parameter.
*  \param  v1 [GW_VoronoiVertex&] 2nd vertex parameter
*  \param  v2 [GW_VoronoiVertex&] 3rd vertex parameter.
*  \param  p0 [GW_Float] 1st value parameter (barycentric coord).
*  \param  p1 [GW_Float] 2nd value parameter (barycentric coord).
*  \param  p2 [GW_Float] 3rd value parameter (barycentric coord).
*  \param  pGuess [GW_Face*] A guess of the surrounding face.
*  \author Gabriel Peyré
*  \date   4-30-2003
*
*  Interpolate the position for a given value of parameters.
*    SLOW : makes an EXHAUSTIVE search !
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_VoronoiMesh::InterpolatePositionExhaustiveSearch( GW_GeodesicMesh& Mesh, GW_Vector3D& Position,
                                         GW_VoronoiVertex& v0, GW_VoronoiVertex& v1, GW_VoronoiVertex& v2,
                                         GW_Float a, GW_Float b, GW_Float c )
{
    GW_Float rBestDist = GW_INFINITE;
    GW_Face* pSelectedFace = NULL;
    GW_Float SelectedCoord[3] = {1,0,0};

    const GW_VoronoiVertex* pVParam0 = &v0, *pVParam1 = &v1, *pVParam2 = &v2;
    GW_Float x,y,z;
    GW_GeodesicVertex *pVert0=NULL, *pVert1=NULL, *pVert2=NULL;

    for( GW_U32 i=0; i<Mesh.GetNbrFace(); ++i )
    {
        GW_Face* pFace = Mesh.GetFace(i);

        pVert0 = (GW_GeodesicVertex*) pFace->GetVertex(0);
        pVert1 = (GW_GeodesicVertex*) pFace->GetVertex(1);
        pVert2 = (GW_GeodesicVertex*) pFace->GetVertex(2);

        /* parameters of the surrounding face */
        GW_Float    a0, b0, c0,
                    a1, b1, c1,
                    a2, b2, c2;
        GW_Bool bValid = GW_True;
        bValid = bValid & GW_VoronoiMesh::GetParameterVertex( *pVert0, a0, b0, c0, *pVParam0, *pVParam1, *pVParam2 );
        bValid = bValid & GW_VoronoiMesh::GetParameterVertex( *pVert1, a1, b1, c1, *pVParam0, *pVParam1, *pVParam2 );
        bValid = bValid & GW_VoronoiMesh::GetParameterVertex( *pVert2, a2, b2, c2, *pVParam0, *pVParam1, *pVParam2 );

        /* translate coordinate in global param into coord in local face param */
        GW_I32 nRet = GW_Maths::ComputeLocalGeodesicParameter( x, y, z,      a, b, c,
                                            a0, b0, c0,   a1, b1, c1,   a2, b2, c2 );
        if( bValid && nRet>=0 )
        {
            if( x>=0 && y>=0 && z>=0 )
            {
                /* it's ok ! */
                Position = pVert0->GetPosition()*x + pVert1->GetPosition()*y + pVert2->GetPosition()*z;
                return;
            }
            /* try best match */
            GW_Float rDist = GW_ABS(0.333-x)+GW_ABS(0.333-y)+GW_ABS(0.333-z);
            if( rDist<rBestDist )
            {
                rBestDist = rDist;
                SelectedCoord[0] = x;
                SelectedCoord[1] = y;
                SelectedCoord[2] = z;
                pSelectedFace = pFace;
            }
        }
    }

    GW_CLAMP(SelectedCoord[0], 0,1);
    GW_CLAMP(SelectedCoord[1], 0,1);
    GW_CLAMP(SelectedCoord[2], 0,1);
    GW_Float s = SelectedCoord[0]+SelectedCoord[1]+SelectedCoord[2];
    if( s!=0 )
    {
        SelectedCoord[0] /= s;
        SelectedCoord[1] /= s;
        SelectedCoord[2] /= s;
    }

    /* snif, we didn't find a correct face ... */
    GW_ASSERT( pSelectedFace!=NULL );
    if( pSelectedFace==NULL )
        return;

    Position =    pSelectedFace->GetVertex(0)->GetPosition()*SelectedCoord[0] +
                pSelectedFace->GetVertex(1)->GetPosition()*SelectedCoord[1] +
                pSelectedFace->GetVertex(2)->GetPosition()*SelectedCoord[2];
}


/*------------------------------------------------------------------------------*/
// Name : GetNaturalNeighborWeights
/**
*  \param  Weights [T_FloatMap&] The weights for each neighbor.
*  \param  Mesh [GW_GeodesicMesh&] The mesh.
*  \param  Vert [GW_GeodesicVertex&] The vertex where we want to perform the interpolation.
*  \author Gabriel Peyré
*  \date   5-30-2003
*
*  Compute the weights for a natural neighbors interpolation.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_VoronoiMesh::GetNaturalNeighborWeights( T_FloatMap& Weights, GW_GeodesicMesh& Mesh, GW_GeodesicVertex& Vert )
{
    if( Vert.GetFace()==NULL )
        return;

    /* just correct the distance map if necessary */
    GW_GeodesicVertex* pStartVert = BaseVertexList_.back();
    if( pStartVert->GetFront()!=pStartVert )
    {
        Mesh.RegisterVertexInsersionCallbackFunction( GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersion );
        GW_VoronoiMesh::ResetOnlyVertexState( Mesh );
        Mesh.PerformFastMarching( pStartVert  );
        Mesh.RegisterVertexInsersionCallbackFunction( NULL );
    }

    if( !bInterpolationPreparationDone_ )
    {
        GW_VoronoiMesh::PrepareInterpolation( Mesh );
        bInterpolationPreparationDone_ = GW_True;
    }

    Weights.clear();

    /* special case for original point */
    for( IT_GeodesicVertexList it=BaseVertexList_.begin(); it!=BaseVertexList_.end(); ++it )
    {
        if( &Vert==*it )
        {
            // break;
            Weights[ Vert.GetID() ] = 1;
            return;
        }
    }

    /* perform a Fast Marching from the point */
    Mesh.RegisterVertexInsersionCallbackFunction( GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionNN );
    GW_VoronoiMesh::ResetOnlyVertexState( Mesh );
    Mesh.PerformFastMarching( &Vert  );

    /* compute contribution for weights */
    T_FaceList FaceToProceed;
    FaceToProceed.push_back( Vert.GetFace() );
    GW_Float rTotalArea = 0;
    T_FaceMap FaceMap;
    FaceMap[ Vert.GetFace()->GetID() ] = Vert.GetFace();

    while( !FaceToProceed.empty() )
    {
        GW_Face* pFace = FaceToProceed.front();
        GW_ASSERT( pFace!=NULL );
        FaceToProceed.pop_front();

        /* compute contribution */
        rTotalArea += GW_VoronoiMesh::NaturalNeighborContribution( *pFace, Vert, Weights );

        /* add neighbors */
        for( GW_U32 i=0; i<3; ++i )
        {
            GW_Face* pNewFace = pFace->GetFaceNeighbor(i);
            if( pNewFace!=NULL && FaceMap.find(pNewFace->GetID())==FaceMap.end() )
            {
                GW_Bool bAddThisFace = GW_False;
                for( GW_U32 nVert=0; nVert<3; ++nVert )
                {
                    GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) pNewFace->GetVertex(nVert);
                    GW_ASSERT( pVert!=NULL );
                    if( pVert->GetFront()==&Vert )
                    {
                        bAddThisFace = GW_True;
                        break;
                    }
                }
                if( bAddThisFace )
                {
                    FaceToProceed.push_back( pNewFace );
                    FaceMap[ pNewFace->GetID() ] = pNewFace;    // so that it won't be added anymore
                }
            }
        }
    }

    /* scale so that weights sum to 1 */
    GW_ASSERT( rTotalArea>0 );
    if( rTotalArea>0 )
    for( IT_FloatMap it=Weights.begin(); it!=Weights.end(); ++it )
        it->second /= rTotalArea;


    Mesh.RegisterVertexInsersionCallbackFunction( NULL );
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::NaturalNeighborContribution
/**
*  \param  Face [GW_Face&] The face.
*  \param  Vert [GW_GeodesicVertex&] The vertex to interpolate.
*  \param  Weights [T_FloatMap&] The weights.
*  \return [GW_Float] Total area that contriibute to interpolation.
*  \author Gabriel Peyré
*  \date   5-30-2003
*
*  Add the contribution to the weights for this face.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_VoronoiMesh::NaturalNeighborContribution( GW_Face& Face, GW_GeodesicVertex& Vert, T_FloatMap& Weights )
{
    GW_GeodesicInformationDuplicata* pDuplicata = NULL;
    /* the area of the face */
    GW_Float rContrib = 0;
    GW_Float lambda;

    /* for each vertex see if there is a contribution */
    GW_U32 nNumContrib = 0;
    GW_U32 nNumOther = 0;
    GW_GeodesicVertex* pVertContrib[3]    =    { NULL, NULL, NULL };
    GW_Float rDistance[3]                =    { -1, -1, -1 };
    for( GW_U32 i=0; i<3; ++i )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Face.GetVertex(i);    GW_ASSERT( pVert!=NULL );
        if( pVert->GetFront()==&Vert )
        {
            pVertContrib[nNumContrib] = pVert;
            rDistance[nNumContrib] = pVert->GetDistance();
            nNumContrib++;
        }
        else
        {
            pVertContrib[2-nNumOther] = pVert;
            GW_GeodesicInformationDuplicata* pDuplicata1 = (GW_GeodesicInformationDuplicata*) pVert->GetUserData();
            GW_ASSERT( pDuplicata1!=NULL );
            rDistance[2-nNumOther] = pDuplicata1->rDistance_;
            nNumOther++;
        }
    }

    /* Flatten each 3 vertex */
    GW_ASSERT( pVertContrib[0]!=NULL );
    GW_ASSERT( pVertContrib[1]!=NULL );
    GW_ASSERT( pVertContrib[2]!=NULL );
    GW_Vector2D VertContribPos[3];
    GW_Vector3D e1 = pVertContrib[1]->GetPosition() - pVertContrib[0]->GetPosition();
    GW_Vector3D e2 = pVertContrib[2]->GetPosition() - pVertContrib[0]->GetPosition();
    GW_Float l1 = ~e1;
    GW_Float l2 = ~e2;
    e1 /= l1;
    e2 /= l2;
    VertContribPos[1] = GW_Vector2D( l1, 0 );
    GW_Float dot = e1*e2;
    VertContribPos[2] = GW_Vector2D( l2*dot, l2*sqrt(1-dot*dot) );

    T_Vector2DList ContribPoly;
    /* for each special case, Compute the polygon */
    switch(nNumContrib) {
    case 1:
    {
        /* the contributor is pVertContrib[0], and pVertContrib[1] & pVertContrib[2] are not contributing */
        /* compute 1st intersection point */
        GW_GeodesicVertex::ComputeFrontIntersection( *pVertContrib[0], rDistance[0], *pVertContrib[1], rDistance[1], NULL, &lambda );
        GW_Vector2D inter1 = VertContribPos[0]*lambda + VertContribPos[1]*(1-lambda);
        /* compute 2nd intersection point */
        GW_GeodesicVertex::ComputeFrontIntersection( *pVertContrib[0], rDistance[0], *pVertContrib[2], rDistance[2], NULL, &lambda );
        GW_Vector2D inter2 = VertContribPos[0]*lambda + VertContribPos[2]*(1-lambda);


        GW_GeodesicInformationDuplicata* pDuplicata1 = (GW_GeodesicInformationDuplicata*) pVertContrib[1]->GetUserData();
        GW_ASSERT( pDuplicata1!=NULL );
        GW_GeodesicInformationDuplicata* pDuplicata2 = (GW_GeodesicInformationDuplicata*) pVertContrib[2]->GetUserData();
        GW_ASSERT( pDuplicata2!=NULL );

        /* two cases */
        if( pDuplicata1->pFront_!=pDuplicata2->pFront_ )
        {
            /* a quadrilater, we must compute the other intersection */
            GW_GeodesicVertex::ComputeFrontIntersection( *pVertContrib[1], rDistance[1], *pVertContrib[2], rDistance[2], NULL, &lambda );
            GW_Vector2D inter3 = VertContribPos[1]*lambda + VertContribPos[2]*(1-lambda);
            GW_Vector2D center = (inter1+inter2+inter3)/3;        // isobarycenter
            /* add the points to the polygon */
            ContribPoly.push_back( VertContribPos[0] );    // remember that non-contributive vertex are in clockwise orientation
            ContribPoly.push_back( inter1 );
            ContribPoly.push_back( center );
            ContribPoly.push_back( inter2 );
            GW_ASSERT( GW_PolygonIntersector::CheckOrientation( ContribPoly ) );
        }
        else
        {
            /* a triangle */
            ContribPoly.push_back( VertContribPos[0] );
            ContribPoly.push_back( inter1 );
            ContribPoly.push_back( inter2 );
            GW_ASSERT( GW_PolygonIntersector::CheckOrientation( ContribPoly ) );

        }

        /* compute the % of contribution to give to each vertex */
        return DistributeContribution( Face, Weights, pVertContrib, VertContribPos, &ContribPoly );
    }
    break;
    case 2:
    {
        /* compute the intersection points */
        GW_GeodesicVertex::ComputeFrontIntersection( *pVertContrib[0], rDistance[0], *pVertContrib[2], rDistance[2], NULL, &lambda );
        GW_Vector2D inter1 = VertContribPos[0]*lambda + VertContribPos[2]*(1-lambda);
        GW_GeodesicVertex::ComputeFrontIntersection( *pVertContrib[1], rDistance[1], *pVertContrib[2], rDistance[2], NULL, &lambda );
        GW_Vector2D inter2 = VertContribPos[1]*lambda + VertContribPos[2]*(1-lambda);

        /* the polygon is a quadrilater */
        ContribPoly.push_back( VertContribPos[0] );
        ContribPoly.push_back( VertContribPos[1] );
        ContribPoly.push_back( inter2 );
        ContribPoly.push_back( inter1 );
        GW_ASSERT( GW_PolygonIntersector::CheckOrientation( ContribPoly ) );

        /* compute the % of contribution to give to each vertex */
        return DistributeContribution( Face, Weights, pVertContrib, VertContribPos, &ContribPoly );
    }
    break;
    case 3:
        /* compute the % of contribution to give to each vertex */
        return DistributeContribution( Face, Weights, pVertContrib, VertContribPos, NULL );    // NULL to avoid intersection computations
        break;
    default:
        GW_ASSERT(GW_False);
        return 0;
    }
}



GW_INLINE
GW_Float GW_VoronoiMesh::DistributeContribution( GW_Face& Face, T_FloatMap& Weights,
                                                GW_GeodesicVertex* pVert[3], GW_Vector2D VertPos[3],
                                                T_Vector2DList* pPolyContrib  )
{
    GW_GeodesicInformationDuplicata* pDuplicata = NULL;
    GW_U32 nID;
    GW_Float lambda;
    GW_GeodesicVertex* pFront[3] = {NULL, NULL, NULL};
    GW_Float d[3];

    /* get the front information */
    for( GW_U32 i=0; i<3; ++i )
    {
        pDuplicata = (GW_GeodesicInformationDuplicata*) pVert[i]->GetUserData();    GW_ASSERT( pDuplicata!=NULL );
        pFront[i] = pDuplicata->pFront_;        GW_ASSERT( pFront[i]!=NULL );
        d[i] = pDuplicata->rDistance_;
    }

    GW_Float rContrib, rContribTemp = 0;

    /* The 3 fronts are the same ************************************************************************************/
    if( pFront[0]==pFront[1] && pFront[1]==pFront[2] )
    {
        if( pPolyContrib==NULL )    // the whole polygon is contributing
            rContrib = Face.GetArea();
        else
            rContrib = GW_PolygonIntersector::ComputePolygonArea( *pPolyContrib );

        nID = pFront[0]->GetID();
        if( Weights.find(nID)==Weights.end() )    //    first time some contribution for this vertex is computed
            Weights[nID] = rContrib;
        else
            Weights[nID] += rContrib;
        return rContrib;
    }

    /* two fronts same and one front different ********************************************************************/
    for( GW_U32 i=0; i<3; ++i )
    {
        GW_U32 i1 = (i+1)%3;
        GW_U32 i2 = (i+2)%3;
        if( pFront[i]!=pFront[i1] && pFront[i1]==pFront[i2] )
        {
            /* compute intersection points *************************************/
            GW_GeodesicVertex::ComputeFrontIntersection( *pVert[i], d[i], *pVert[i1], d[i1], NULL, &lambda );
            GW_Vector2D inter1 = VertPos[i]*lambda + VertPos[i1]*(1-lambda);
            GW_GeodesicVertex::ComputeFrontIntersection( *pVert[i], d[i], *pVert[i2], d[i2], NULL, &lambda );
            GW_Vector2D inter2= VertPos[i]*lambda + VertPos[i2]*(1-lambda);

            /* contribution for front i ****************************************/
            T_Vector2DList Poly1;    // this is a triangle
            Poly1.push_back(VertPos[i]);
            Poly1.push_back(inter1);
            Poly1.push_back(inter2);
            GW_ASSERT( GW_PolygonIntersector::CheckOrientation( Poly1 ) );

            if( pPolyContrib==NULL )    // the whole polygon is contributing
                rContrib = GW_PolygonIntersector::ComputePolygonArea( Poly1 );
            else
            {
                GW_PolygonIntersector PolyInt;
                PolyInt.PerformIntersection( Poly1, *pPolyContrib );
                rContrib = PolyInt.GetAreaIntersection();
            }

            rContribTemp = rContrib;

            nID = pFront[i]->GetID();
            if( Weights.find(nID)==Weights.end() )    //    first time some contribution for this vertex is computed
                Weights[nID] = rContrib;
            else
                Weights[nID] += rContrib;

            /* contribution for front i1 ****************************************/
            T_Vector2DList Poly2;    // this is a quadrilater
            Poly2.push_back(inter1);
            Poly2.push_back(VertPos[i1]);
            Poly2.push_back(VertPos[i2]);
            Poly2.push_back(inter2);
            GW_ASSERT( GW_PolygonIntersector::CheckOrientation( Poly2 ) );

            if( pPolyContrib==NULL )    // the whole polygon is contributing
                rContrib = GW_PolygonIntersector::ComputePolygonArea( Poly2 );
            else
            {
                GW_PolygonIntersector PolyInt;
                PolyInt.PerformIntersection( Poly2, *pPolyContrib );
                rContrib = PolyInt.GetAreaIntersection();
            }

            rContribTemp += rContrib;

            nID = pFront[i1]->GetID();
            if( Weights.find(nID)==Weights.end() )    //    first time some contribution for this vertex is computed
                Weights[nID] = rContrib;
            else
                Weights[nID] += rContrib;
            return rContribTemp;
        }
    }

    /* the 3 fronts are different *************************************************************************/
    GW_ASSERT( pFront[0]!=pFront[1] && pFront[1]!=pFront[2] && pFront[2]!=pFront[0] );
    GW_Vector2D inter[3];
    /* compute intersection */
    for( GW_U32 i=0; i<3; ++i )
    {
        GW_U32 i1 = (i+1)%3;
        GW_U32 i2 = (i+2)%3;
        GW_GeodesicVertex::ComputeFrontIntersection( *pVert[i1], d[i1], *pVert[i2], d[i2], NULL, &lambda );
        inter[i] = VertPos[i1]*lambda + VertPos[i2]*(1-lambda);
    }
    GW_Vector2D center = (inter[0]+inter[1]+inter[2])/3;
    /* compute contribution */
    for( GW_U32 i=0; i<3; ++i )
    {
        GW_U32 i1 = (i+1)%3;
        GW_U32 i2 = (i+2)%3;
        T_Vector2DList Poly;    // this is a quadrilater
        Poly.push_back(VertPos[i]);
        Poly.push_back(inter[i2]);
        Poly.push_back(center);
        Poly.push_back(inter[i1]);
        GW_ASSERT( GW_PolygonIntersector::CheckOrientation( Poly ) );

        if( pPolyContrib==NULL )    // the whole polygon is contributing
            rContrib = GW_PolygonIntersector::ComputePolygonArea( Poly );
        else
        {
            GW_PolygonIntersector PolyInt;
            PolyInt.PerformIntersection( Poly, *pPolyContrib );
            rContrib = PolyInt.GetAreaIntersection();
        }

        nID = pFront[i]->GetID();
        if( Weights.find(nID)==Weights.end() )    //    first time some contribution for this vertex is computed
            Weights[nID] = rContrib;
        else
            Weights[nID] += rContrib;

        rContribTemp += rContrib;
    }
    return rContribTemp;
}




/*------------------------------------------------------------------------------*/
// Name : GetReciprocicalDistanceWeights
/**
*  \param  Weights [T_FloatMap&] The weights for each neighbor.
*  \param  Mesh [GW_GeodesicMesh&] The mesh.
*  \param  Vert [GW_GeodesicVertex&] The vertex where we want to perform the interpolation.
*  \author Gabriel Peyré
*  \date   5-30-2003
*
*  Compute the weights for a reciprocical interpolation.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_VoronoiMesh::GetReciprocicalDistanceWeights( T_FloatMap& Weights, GW_GeodesicMesh& Mesh, GW_GeodesicVertex& Vert )
{
    if( Vert.GetFace()==NULL )
        return;

    /* just correct the distance map if necessary */
    GW_GeodesicVertex* pStartVert = BaseVertexList_.back();
    if( pStartVert->GetFront()!=pStartVert )
    {
        Mesh.RegisterVertexInsersionCallbackFunction( GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersion );
        GW_VoronoiMesh::ResetOnlyVertexState( Mesh );
        Mesh.PerformFastMarching( pStartVert  );
        Mesh.RegisterVertexInsersionCallbackFunction( NULL );
    }

    if( !bInterpolationPreparationDone_ )
    {
        GW_VoronoiMesh::PrepareInterpolation( Mesh );
        bInterpolationPreparationDone_ = GW_True;
    }

    Weights.clear();
    pCurWeights_        = &Weights;
    nNbrBaseVertex_RD_    = 0;

    /* special case for original point */
    for( IT_GeodesicVertexList it=BaseVertexList_.begin(); it!=BaseVertexList_.end(); ++it )
    {
        if( &Vert==*it )
        {
            Weights[ Vert.GetID() ] = 1;
            return;
        }
    }

    /* First step : compute the natural neighbors ************************************************/
    /* perform a Fast Marching from the point */
    Mesh.RegisterVertexInsersionCallbackFunction( GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionRD1 );
    GW_VoronoiMesh::ResetOnlyVertexState( Mesh );
    Mesh.PerformFastMarching( &Vert  );

    /* Second step : compute the weights *********************************************************/
    /* perform a Fast Marching from the point */
    Mesh.RegisterVertexInsersionCallbackFunction( GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionRD2 );
    Mesh.RegisterForceStopCallbackFunction( GW_VoronoiMesh::FastMarchingCallbackFunction_ForceStopRD );
    Mesh.ResetGeodesicMesh();
    Mesh.PerformFastMarching( &Vert  );

    /* scale so that weights sum to 1 */
    GW_Float rTotalWeigth = 0;
    for( IT_FloatMap it=Weights.begin(); it!=Weights.end(); ++it )
        rTotalWeigth += it->second;
    GW_ASSERT( rTotalWeigth>0 );
    for( IT_FloatMap it=Weights.begin(); it!=Weights.end(); ++it )
            it->second /= rTotalWeigth;

    Mesh.RegisterVertexInsersionCallbackFunction( NULL );
    Mesh.RegisterForceStopCallbackFunction( NULL );
}



} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
