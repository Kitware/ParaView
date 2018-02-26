/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_VoronoiMesh.cpp
 *  \brief  Definition of class \c GW_VoronoiMesh
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_VoronoiMesh.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_VoronoiMesh.h"

#ifndef GW_USE_INLINE
    #include "GW_VoronoiMesh.inl"
#endif

using namespace GW;

T_VoronoiVertexList GW_VoronoiMesh::CurrentTargetVertex_;
GW_VoronoiVertex* GW_VoronoiMesh::pCurrentVoronoiVertex_ = NULL;
T_VoronoiVertexMap GW_VoronoiMesh::VoronoiVertexMap_;
T_VertexPathMultiMap GW_VoronoiMesh::BoundaryEdgeMap_;

GW_U32 GW_VoronoiMesh::nNbrBaseVertex_RD_    = 0;
T_FloatMap* GW_VoronoiMesh::pCurWeights_    = NULL;

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::PerformFastMarching
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh.
 *  \author Gabriel Peyré
 *  \date   4-13-2003
 *
 *  Just an helper method that launch a fire from each base point.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::PerformFastMarching( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList )
{
    /* add the points as starting points */
    Mesh.ResetGeodesicMesh();
    for( IT_GeodesicVertexList it = VertList.begin(); it!=VertList.end(); ++it )
    {
        GW_GeodesicVertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        Mesh.AddStartVertex( *pVert );
    }
    /* perform a firestart */
    Mesh.PerformFastMarching();
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersion
/**
 *  \param  CurVert [GW_GeodesicVertex&] The current vertex.
 *  \param  rNewDist [GW_Float] The new distance.
 *  \return [GW_Bool] Insert this vertex to active list ?
 *  \author Gabriel Peyré
 *  \date   5-13-2003
 *
 *  Add the vertex only if the distance decreases.
 */
/*------------------------------------------------------------------------------*/
GW_Bool GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersion( GW_GeodesicVertex& CurVert, GW_Float rNewDist )
{
    return CurVert.GetDistance() > rNewDist;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::AddFurthestPoint
/**
*  \param  Mesh [GW_GeodesicMesh&] Fine mesh.
*  \author Gabriel Peyré
*  \date   4-14-2003
*
*  Add to the list of point the furthest from all nodes.
*/
/*------------------------------------------------------------------------------*/
GW_U32 GW_VoronoiMesh::AddFurthestPoint( T_GeodesicVertexList& VertList,GW_GeodesicMesh& Mesh, GW_Bool bUseRandomStartVertex )
{
    if( VertList.empty() )
    {
        Mesh.ResetGeodesicMesh();
        /* choose 1 point at random */
        GW_GeodesicVertex* pStartVertex = (GW_GeodesicVertex*) Mesh.GetRandomVertex();
        GW_ASSERT( pStartVertex!=NULL );
        if( pStartVertex==NULL )
            return 0;
        VertList.push_back( pStartVertex );
        if( !bUseRandomStartVertex )
        {
            /* find furthest point from this one */
            GW_VoronoiMesh::PerformFastMarching( Mesh, VertList );
            GW_GeodesicVertex* pSelectedVert = GW_VoronoiMesh::FindMaxVertex( Mesh );
            GW_ASSERT( pSelectedVert!=NULL );
            VertList.push_back( pSelectedVert );
            VertList.pop_front();    // remove random chosen vertex
            Mesh.ResetGeodesicMesh();
        }
    }
    else
    {
        Mesh.RegisterVertexInsersionCallbackFunction( GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersion );

        /* perform marching */
        GW_VoronoiMesh::ResetOnlyVertexState( Mesh );
        GW_GeodesicVertex* pStartVert = VertList.back();
        GW_ASSERT( pStartVert!=NULL );
        Mesh.PerformFastMarching( pStartVert  );

        /* find intersection points */
        GW_GeodesicVertex* pSelectedVert = GW_VoronoiMesh::FindMaxVertex( Mesh );
        GW_ASSERT( pSelectedVert!=NULL );
        /* find the triangle where the maximum occurs */
#if 0
        GW_Face* pMaxFace = GW_VoronoiMesh::FindMaxFace( *pSelectedVert );
        GW_ASSERT( pMaxFace!=NULL );
        /* this is a critical point */
        GW_Float x=1.0f/3, y=1.0f/3, z=1.0f/3;
        GW_GeodesicVertex* pNewVert = (GW_GeodesicVertex*) Mesh.InsertVertexInFace( *pMaxFace, x,y,z );
        pNewVert->SetFront( pSelectedVert->GetFront() );
        pNewVert->SetDistance( pSelectedVert->GetDistance() );
#else
        GW_GeodesicVertex* pNewVert = pSelectedVert;
#endif
        /* compute the real point */
        VertList.push_back( pNewVert );

        Mesh.RegisterVertexInsersionCallbackFunction( NULL );
    }

    return 1;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FindMaxVertex
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh.
 *  \return [GW_GeodesicVertex*] The max distance vertex.
 *  \author Gabriel Peyré
 *  \date   5-14-2003
 *
 *  Find the vertex with maximum distance.
 */
/*------------------------------------------------------------------------------*/
GW_GeodesicVertex* GW_VoronoiMesh::FindMaxVertex( GW_GeodesicMesh& Mesh )
{
    GW_GeodesicVertex* pSelectedVert = NULL;
    GW_Float rMaxDist = -GW_INFINITE;
    for( GW_U32 i=0; i<Mesh.GetNbrVertex(); ++i )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Mesh.GetVertex( i );
        GW_ASSERT( pVert!=NULL );
        if( pVert->GetFace()!=NULL && pVert->GetDistance()>rMaxDist )
        {
            rMaxDist = pVert->GetDistance();
            pSelectedVert = pVert;
        }
    }
    return pSelectedVert;
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FindMaxFace
/**
 *  \param  Vert [GW_GeodesicVertex&] The current vertex.
 *  \return [GW_Face*] The face.
 *  \author Gabriel Peyré
 *  \date   4-15-2003
 *
 *  Find the face of maximum distance.
 */
/*------------------------------------------------------------------------------*/
GW_Face* GW_VoronoiMesh::FindMaxFace( GW_GeodesicVertex& Vert )
{
    GW_Float rBestDistance = -GW_INFINITE;
    GW_Face* pCurFace_ = NULL;
    for( GW_VertexIterator it = Vert.BeginVertexIterator(); it!=Vert.EndVertexIterator(); ++it )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*)  *it;
        if( pVert->GetDistance()>rBestDistance )
        {
            rBestDistance = pVert->GetDistance();
            GW_GeodesicVertex* pVert1 = (GW_GeodesicVertex*) it.GetLeftVertex();
            GW_GeodesicVertex* pVert2 = (GW_GeodesicVertex*) it.GetRightVertex();
            if( pVert1!=NULL && pVert2!=NULL )
            {
                if( pVert1->GetDistance()<pVert2->GetDistance() )
                    pCurFace_ = it.GetLeftFace();
                else
                    pCurFace_ = it.GetRightFace();
            }
            else if( pVert1!=NULL )
            {
                pCurFace_ = it.GetLeftFace();
            }
            else
            {
                GW_ASSERT( pVert2!=NULL );
                pCurFace_ = it.GetRightFace();
            }
        }
    }
    return pCurFace_;
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::AddFurthestPointsIterate
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh.
 *  \param  nNbrIterations [GW_U32] Number of points.
 *  \author Gabriel Peyré
 *  \date   4-14-2003
 *
 *  Add a given number of furthest points.
 */
/*------------------------------------------------------------------------------*/
GW_U32 GW_VoronoiMesh::AddFurthestPointsIterate( T_GeodesicVertexList& VertList,GW_GeodesicMesh& Mesh, GW_U32 nNbrIterations, GW_Bool bUseRandomStartVertex, GW_Bool bUseProgressBar )
{
    GW_U32 nNbrPoints = 0;
    GW_ProgressBar pb;
    if( bUseProgressBar )
        pb.Begin();
    for( GW_U32 i=0; i<nNbrIterations; ++i )
    {
        nNbrPoints += GW_VoronoiMesh::AddFurthestPoint( VertList, Mesh, bUseRandomStartVertex );
        if( bUseProgressBar )
            pb.Update(((GW_Float) i+1)/((GW_Float) nNbrIterations));
    }
    if( bUseProgressBar )
    {
        pb.End();
        cout << endl;
    }
    return nNbrPoints;
}




/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FastMarchingCallbackFunction_MeshBuilding
/**
*  \param  CurVert [GW_GeodesicVertex&] The current vertex.
*  \return [GW_Bool] The new dead vertex.
*  \author Gabriel Peyré
*  \date   5-13-2003
*
*  Test if the vertex is a saddle point.
*/
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::FastMarchingCallbackFunction_MeshBuilding( GW_GeodesicVertex& CurVert )
{
    GW_GeodesicVertex* pFront = CurVert.GetFront();
    GW_ASSERT( pFront!=NULL );
    /* retrieve the voronoi vertex corresponding to the front */
    GW_VoronoiVertex* pVoronoiVert0 = GW_VoronoiMesh::GetVoronoiFromGeodesic( *pFront );
    GW_ASSERT( pVoronoiVert0!=NULL );
    /* test if this point is a saddle point */
    for( GW_VertexIterator it=CurVert.BeginVertexIterator(); it!=CurVert.EndVertexIterator(); ++it )
    {
        GW_GeodesicVertex* pNeighborVert = (GW_GeodesicVertex*) *it;
        GW_GeodesicVertex* pNeighborFront = pNeighborVert->GetFront();
        if( pNeighborFront!=NULL && pNeighborFront!=pFront )
        {
            /* that's it ! */
            GW_VoronoiVertex* pVoronoiVert1 = GW_VoronoiMesh::GetVoronoiFromGeodesic( *pNeighborFront );
            GW_ASSERT( pVoronoiVert1!=NULL );
            if( !pVoronoiVert0->IsNeighbor(*pVoronoiVert1) )
            {
                GW_ASSERT( ! pVoronoiVert1->IsNeighbor(*pVoronoiVert0) );
                /* test for manifold structure for the future edge */
                GW_U32 nNumTriangle = 0;
                for( IT_VoronoiVertexList itVoronoi=pVoronoiVert1->BeginNeighborIterator(); itVoronoi!=pVoronoiVert1->EndNeighborIterator(); ++itVoronoi )
                {
                    GW_VoronoiVertex* pVoronoiVert2 = *itVoronoi;
                    GW_ASSERT( pVoronoiVert2!=NULL );
                    if( pVoronoiVert2!=pVoronoiVert0 && pVoronoiVert0->IsNeighbor(*pVoronoiVert2) )
                        nNumTriangle++;
                }
                if( nNumTriangle<=2 )
                {
                    pVoronoiVert0->AddNeighbor( *pVoronoiVert1 );
                    pVoronoiVert1->AddNeighbor( *pVoronoiVert0 );
                }
                /* test for manifold structure on the newly created edges */
                for( IT_VoronoiVertexList itVoronoi=pVoronoiVert1->BeginNeighborIterator(); itVoronoi!=pVoronoiVert1->EndNeighborIterator(); ++itVoronoi )
                {
                    GW_VoronoiVertex* pVoronoiVert2 = *itVoronoi;
                    GW_ASSERT( pVoronoiVert2!=NULL );
                    if( pVoronoiVert2!=pVoronoiVert0 && pVoronoiVert0->IsNeighbor(*pVoronoiVert2) )
                    {
                        /* we are on a newly created face : test for structure integrity */
                        if( !GW_VoronoiMesh::TestManifoldStructure(*pVoronoiVert0, *pVoronoiVert2) )
                        {
                            pVoronoiVert0->RemoveNeighbor( *pVoronoiVert1 );
                            pVoronoiVert1->RemoveNeighbor( *pVoronoiVert0 );
                            break;
                        }
                        if( !GW_VoronoiMesh::TestManifoldStructure(*pVoronoiVert1, *pVoronoiVert2) )
                        {
                            pVoronoiVert0->RemoveNeighbor( *pVoronoiVert1 );
                            pVoronoiVert1->RemoveNeighbor( *pVoronoiVert0 );
                            break;
                        }
                    }
                }
            }
            else
            {
                /* the graph must be undirected */
                GW_ASSERT( pVoronoiVert1->IsNeighbor(*pVoronoiVert0) );
            }
        }
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::TestManifoldStructure
/**
 *  \param  Vert0 [GW_VoronoiVertex&] 1st vertex.
 *  \param  Vert1 [GW_VoronoiVertex&] 2nd vertex.
 *  \return [GW_Bool] Answer.
 *  \author Gabriel Peyré
 *  \date   5-15-2003
 *
 *  Test if the edge has only 2 neighbors.
 */
/*------------------------------------------------------------------------------*/
GW_Bool GW_VoronoiMesh::TestManifoldStructure( GW_VoronoiVertex& Vert0, GW_VoronoiVertex& Vert1 )
{
    GW_U32 nNumTriangle = 0;
    for( IT_VoronoiVertexList itVoronoi=Vert1.BeginNeighborIterator(); itVoronoi!=Vert1.EndNeighborIterator(); ++itVoronoi )
    {
        GW_VoronoiVertex* pVoronoiVert2 = *itVoronoi;
        GW_ASSERT( pVoronoiVert2!=NULL );
        if( pVoronoiVert2!=&Vert0 && Vert0.IsNeighbor(*pVoronoiVert2) )
            nNumTriangle++;
    }
    return nNumTriangle<=2;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::CreateVoronoiVertex
/**
 *  \author Gabriel Peyré
 *  \date   6-1-2003
 *
 *  Just create the voronoi vertex.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::CreateVoronoiVertex()
{
    /* Create Vornoi vertex and make the inverse map GeodesicVertex->VoronoiVertex */
    this->SetNbrVertex( (GW_U32) BaseVertexList_.size() );
    GW_U32 nNum = 0;
    for( IT_GeodesicVertexList it = BaseVertexList_.begin(); it!=BaseVertexList_.end(); ++it )
    {
        GW_GeodesicVertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        /* create a new voronoi vertex */
        GW_VoronoiVertex* pVornoiVert = new GW_VoronoiVertex;
        pVornoiVert->SetBaseVertex( *pVert );
        this->SetVertex( nNum, pVornoiVert );
        nNum++;
        /* build the inverse map */
        GW_ASSERT( VoronoiVertexMap_.find( pVert->GetID() )==VoronoiVertexMap_.end() );
        VoronoiVertexMap_[ pVert->GetID() ] = pVornoiVert;
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::BuildMesh
/**
 *  \param  Mesh [GW_Mesh&] The mesh that will be build.
 *  \author Gabriel Peyré
 *  \date   4-13-2003
 *
 *  Build a mesh from scratch.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::BuildMesh( GW_GeodesicMesh& Mesh, GW_Bool bFixHole )
{
    /* Create Vornoi vertex and make the inverse map GeodesicVertex->VoronoiVertex */
    this->CreateVoronoiVertex();

#if 1    // simple method

    GW_OutputComment("Recomputing the whole Voronoi diagram.");
    GW_VoronoiMesh::PerformFastMarching( Mesh, BaseVertexList_ );

    /* find the faces */
    GW_OutputComment("Building the faces.");
    T_FaceMap FaceMap;    // to store the faces already built.
    for( GW_U32 i=0; i<Mesh.GetNbrFace(); ++i )
    {
        GW_Face* pFace = Mesh.GetFace(i);    GW_ASSERT( pFace!=NULL );
        GW_GeodesicVertex* pGeo[3];
        GW_GeodesicVertex* pFront[3];
        for( GW_U32 j=0; j<3; ++j )
        {
            pGeo[j] = (GW_GeodesicVertex*) pFace->GetVertex(j); GW_ASSERT( pGeo[j]!=NULL );
            pFront[j] = pGeo[j]->GetFront();
        }
        if( pFront[0]!=pFront[1] && pFront[1]!=pFront[2] && pFront[2]!=pFront[0] )
        {
            GW_U32 nID = GW_Vertex::ComputeUniqueId( *pFront[0], *pFront[1], *pFront[2] );
            if( FaceMap.find(nID)==FaceMap.end() )
            {
                /* create the face */
                GW_Face& Face = this->CreateNewFace();
                FaceMap[nID] = &Face;
                for( GW_U32 j=0; j<3; ++j )        // assign the vertices
                {
                    GW_U32 nID = pFront[j]->GetID();
                    GW_ASSERT( VoronoiVertexMap_.find(nID)!=VoronoiVertexMap_.end() );
                    GW_VoronoiVertex* pVorVert = VoronoiVertexMap_[nID];    GW_ASSERT( pVorVert!=NULL );
                    Face.SetVertex( *pVorVert, j );
                }
            }
        }
    }

#else

    GW_OutputComment("Computing voronoi diagrams.");
    /* perform once more a firestart to set up connectivity */
    Mesh.RegisterNewDeadVertexCallbackFunction( GW_VoronoiMesh::FastMarchingCallbackFunction_MeshBuilding );
    Mesh.ResetGeodesicMesh();
    GW_VoronoiMesh::PerformFastMarching( Mesh, BaseVertexList_ );
    Mesh.RegisterNewDeadVertexCallbackFunction( NULL );

    /* build the faces */
    T_FaceMap FaceMap;    // to store the faces already built.

    GW_OutputComment("Building voronoi mesh faces.");
    for( IT_GeodesicVertexList it = BaseVertexList_.begin(); it!=BaseVertexList_.end(); ++it )
    {
        GW_GeodesicVertex* pVert0 = *it;
        GW_ASSERT( pVert0!=NULL );
        /* retrieve the corresponding voronoi vertex */
        GW_VoronoiVertex* pVoronoiVert0 = GW_VoronoiMesh::GetVoronoiFromGeodesic( *pVert0 );
        GW_ASSERT( pVoronoiVert0!=NULL );
        for( IT_VoronoiVertexList itVoronoi1=pVoronoiVert0->BeginNeighborIterator(); itVoronoi1!=pVoronoiVert0->EndNeighborIterator(); ++itVoronoi1 )
        {
            GW_VoronoiVertex* pVoronoiVert1 = *itVoronoi1;
            GW_ASSERT( pVoronoiVert1!=NULL );
            GW_U32 nNumTriangle = 0;
            for( IT_VoronoiVertexList itVoronoi2=pVoronoiVert1->BeginNeighborIterator(); itVoronoi2!=pVoronoiVert1->EndNeighborIterator(); ++itVoronoi2 )
            {
                GW_VoronoiVertex* pVoronoiVert2 = *itVoronoi2;
                GW_ASSERT( pVoronoiVert2!=NULL );
                if( pVoronoiVert2!=pVoronoiVert0 && pVoronoiVert0->IsNeighbor(*pVoronoiVert2) )
                {
                    /* yes, we find a triangle ! Test if it wasn't already constructed */
                    GW_U32 nUniqueId = GW_Vertex::ComputeUniqueId( *pVoronoiVert0, *pVoronoiVert1, *pVoronoiVert2 );
                    if( FaceMap.find(nUniqueId)==FaceMap.end() )
                    {
                        nNumTriangle++;
                        GW_ASSERT( nNumTriangle<=2 );    // assert manifold structure
                        /* this is the 1st time we encounter this face. */
                        GW_Face* pFace = &Mesh.CreateNewFace();
                        /* set up the face */
                        pFace->SetVertex( *pVoronoiVert0, *pVoronoiVert1, *pVoronoiVert2 );
                        FaceMap[nUniqueId] = pFace;
                    }
                }
            }
        }
    }

#endif


    /* assign the faces */
    this->SetNbrFace( (GW_U32) FaceMap.size() );
    GW_U32 nNum = 0;
    for( IT_FaceMap it = FaceMap.begin(); it!=FaceMap.end(); ++it )
    {
        this->SetFace( nNum, it->second );
        nNum++;
    }
    /* rebuild connectivity */
    GW_OutputComment("Building connectivity.");
    this->BuildConnectivity();
    /* try to fill the holes */
    if( bFixHole )
    {
        GW_OutputComment("Fixing holes.");
        this->FixHole();
        /* re-rebuild connectivity */
        this->BuildConnectivity();
    }

}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FixHole
/**
 *  \author Gabriel Peyré
 *  \date   5-16-2003
 *
 *  Try to fix the hole in the boundary.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::FixHole()
{
    typedef std::pair<GW_VoronoiVertex*, GW_VoronoiVertex*> T_VertexPair;
    typedef std::list<T_VertexPair>    T_VertexPairList;
    typedef T_VertexPairList::iterator    IT_VertexPairList;
    T_VertexPairList VertexPairList;
    for( GW_U32 i=0; i<this->GetNbrFace(); ++i )
    {
        GW_Face* pFace = this->GetFace(i);
        GW_ASSERT( pFace!=NULL );
        for( GW_U32 nV = 0; nV<3; ++nV )
        {
            if( pFace->GetFaceNeighbor(nV)==NULL )
            {
                GW_VoronoiVertex* pVert1 = (GW_VoronoiVertex*) pFace->GetVertex( (nV+1)%3 );
                GW_VoronoiVertex* pVert2 = (GW_VoronoiVertex*) pFace->GetVertex( (nV+2)%3 );
                VertexPairList.push_back( T_VertexPair(pVert1,pVert2) );
            }
        }
    }
    char str[50];
    sprintf( str, "%d boundary edges detected.", VertexPairList.size() );
    GW_OutputComment( str );
    while( !VertexPairList.empty() )
    {
        T_VertexPairList HoleBorder;
        T_VertexPair StartEdge = VertexPairList.front();
        T_VertexPair CurEdge = StartEdge;
        VertexPairList.pop_front();
        HoleBorder.push_back( CurEdge );
        /* try to find the hole border */
        GW_Bool bNextEdgeFound = GW_False;
        while( true )
        {
            bNextEdgeFound = GW_False;
            for( IT_VertexPairList it = VertexPairList.begin(); it!=VertexPairList.end(); ++it )
            {
                T_VertexPair NewEdge = *it;
                if( (NewEdge.first==CurEdge.second) && (NewEdge.second!=CurEdge.first) )
                {
                    CurEdge = NewEdge;
                    HoleBorder.push_back( CurEdge );
                    bNextEdgeFound = GW_True;
                    VertexPairList.erase( it );
                    break;
                }
                if( (NewEdge.second==CurEdge.second)  && (NewEdge.first!=CurEdge.first) )
                {
                    CurEdge = T_VertexPair( NewEdge.second, NewEdge.first);
                    HoleBorder.push_back( CurEdge );
                    bNextEdgeFound = GW_True;
                    VertexPairList.erase( it );
                    break;
                }
            }
            if( !bNextEdgeFound )
                break;        // the hole cannot be completed
            if( StartEdge.first == CurEdge.second )
                break;        // the hole is completed
        }
        if( bNextEdgeFound && HoleBorder.size()>2 )    // that means we have a full hole
        {
            char str[50];
            sprintf( str, "Filing a hole of %d vertex.", HoleBorder.size() );
            GW_OutputComment( str );
            IT_VertexPairList it = HoleBorder.begin();
            GW_VoronoiVertex* pVert0 = it->first;        GW_ASSERT( pVert0!=NULL );
            it++;
            GW_VoronoiVertex* pVert1 = it->first;        GW_ASSERT( pVert1!=NULL );
            it++;
            for( ; it!=HoleBorder.end(); ++it )
            {
                GW_VoronoiVertex* pVert2 = it->first;    GW_ASSERT( pVert2!=NULL );
                /* test for manifold structure before creating a new edge [v0,v2] */
                GW_Bool bManifold = GW_True;
                GW_Face* pFace1, *pFace2;
                pVert0->GetFaces( *pVert1, pFace1, pFace2 );
                if( pFace1!=NULL && pFace2!=NULL )
                    bManifold = GW_False;
                pVert1->GetFaces( *pVert2, pFace1, pFace2 );
                if( pFace1!=NULL && pFace2!=NULL )
                    bManifold = GW_False;
                pVert0->GetFaces( *pVert2, pFace1, pFace2 );
                if( pFace1!=NULL && pFace2!=NULL )
                    bManifold = GW_False;
                if( bManifold )
                {
                    GW_Face& Face = this->CreateNewFace();
                    Face.SetVertex( *pVert0, *pVert1, *pVert2 );
                    this->AddFace( Face );
                }
                pVert1 = pVert2;
            }
        }
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::CreateNewVertex
/**
*  \return [GW_Vertex&] The newly created vertex.
*  \author Gabriel Peyré
*  \date   4-9-2003
*
*  Allocate memory for a new vertex.
*/
/*------------------------------------------------------------------------------*/
GW_Vertex& GW_VoronoiMesh::CreateNewVertex()
{
    return *(new GW_VoronoiVertex);
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::CreateNewFace
/**
*  \return [GW_Face&] The newly created face.
*  \author Gabriel Peyré
*  \date   4-9-2003
*
*  Allocate memory for a new face.
*/
/*------------------------------------------------------------------------------*/
GW_Face& GW_VoronoiMesh::CreateNewFace()
{
    return *(new GW_Face);
}

/*------------------------------------------------------------------------------*/
// Name : FastMarchingCallbackFunction_Boundaries
/**
*  \author Gabriel Peyré
*  \date   4-12-2003
*
*  A callback function for geodesic computations.
*/
/*------------------------------------------------------------------------------*/
GW_Bool GW_VoronoiMesh::FastMarchingCallbackFunction_Boundaries( GW_GeodesicVertex& CurVert )
{
    // \todo for the moment propagate on the full neighborhood
//    for( IT_VoronoiVertexList it=CurrentTargetVertex_.begin(); it!=CurrentTargetVertex_.end(); ++it )
//    {
    GW_ASSERT( pCurrentVoronoiVertex_!=NULL );
    for( GW_VertexIterator it=pCurrentVoronoiVertex_->BeginVertexIterator(); it!=pCurrentVoronoiVertex_->EndVertexIterator(); ++it )
    {
        GW_VoronoiVertex* pVornoiVert = (GW_VoronoiVertex*) *it;
        GW_ASSERT( pVornoiVert!=NULL );
        GW_GeodesicVertex* pVert = pVornoiVert->GetBaseVertex();
        GW_ASSERT( pVert!=NULL );
        if( pVert->GetState()!=GW_GeodesicVertex::kDead )
            return GW_False;
    }
    return GW_True;
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::BuildGeodesicBoundaries
/**
 *  \author Gabriel Peyré
 *  \date   4-21-2003
 *
 *  Compute the geodesic path corresponding to each base mesh vertex.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::BuildGeodesicBoundaries( GW_GeodesicMesh& Mesh )
{
    Mesh.RegisterForceStopCallbackFunction( FastMarchingCallbackFunction_Boundaries );

    VertexPathMap_.clear();
    GeodesicDistanceMap_.clear();
    BoundaryEdgeMap_.clear();

    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        CurrentTargetVertex_.clear();
        pCurrentVoronoiVertex_ = (GW_VoronoiVertex*) this->GetVertex(i);
        GW_ASSERT( pCurrentVoronoiVertex_!=NULL );

        /* set the list of vertex to which the boundary needs to be computed */
        for( GW_VertexIterator it=pCurrentVoronoiVertex_->BeginVertexIterator(); it!=pCurrentVoronoiVertex_->EndVertexIterator(); ++it )
        {
            GW_VoronoiVertex* pVert = (GW_VoronoiVertex*) *it;
            GW_ASSERT( pVert!=NULL );
            GW_U32 nID = GW_Vertex::ComputeUniqueId( *pCurrentVoronoiVertex_, *pVert );
            if( VertexPathMap_.find( nID )==VertexPathMap_.end() )
                CurrentTargetVertex_.push_back( pVert );
        }

        /* compute fast marching */
        GW_GeodesicVertex* pGeodesicVert = pCurrentVoronoiVertex_->GetBaseVertex();
        GW_ASSERT( pGeodesicVert!=NULL );
        Mesh.ResetGeodesicMesh();
        GW_OutputComment("Performing Fast Marching.");
        Mesh.PerformFastMarching( pGeodesicVert );

        /* track back and compute geodesic path */
        for( IT_VoronoiVertexList it=CurrentTargetVertex_.begin(); it!=CurrentTargetVertex_.end(); ++it )
        {
            GW_VoronoiVertex* pVert = *it;
            GW_ASSERT( pVert!=NULL );
            GW_GeodesicVertex* pGeodesicVert = pVert->GetBaseVertex();
            GW_ASSERT( pGeodesicVert!=NULL );
            GW_ASSERT( pGeodesicVert->GetState()==GW_GeodesicVertex::kDead );
            GW_U32 nID = GW_Vertex::ComputeUniqueId( *pCurrentVoronoiVertex_, *pVert );
            GW_ASSERT( VertexPathMap_.find( nID ) ==VertexPathMap_.end() );
            T_GeodesicVertexList* pVertexGeodesicEdge = new T_GeodesicVertexList;
            VertexPathMap_[nID] = pVertexGeodesicEdge;
            /* track back and compute the path */
            GW_OutputComment("Computing geodesic path.");
            GW_GeodesicPath GeodesicPath;
            GeodesicPath.ComputePath( *pGeodesicVert, 5000 );
            /* convert the path into vertex */
            this->AddPathToMeshVertex( Mesh, GeodesicPath, *pVertexGeodesicEdge );
        }
    }

    /* \todo This is just a debug test, remove it */
    for( IT_VertexPathMap it = VertexPathMap_.begin(); it!=VertexPathMap_.end(); ++it )
    {
        T_GeodesicVertexList* pPath = it->second;
        GW_GeodesicVertex* pPrevVert = NULL;
        GW_U32 nNum = 0;
        for( IT_GeodesicVertexList itVert = pPath->begin(); itVert!=pPath->end(); ++itVert )
        {
            GW_GeodesicVertex* pVert = *itVert;
            if( pPrevVert==NULL )
                pPrevVert = pVert;
            else
            {
                GW_Face* pFace1, *pFace2;
                pPrevVert->GetFaces( *pVert, pFace1, pFace2 );
                // GW_ASSERT( pFace1!=NULL || pFace2!=NULL )    // \todo FIX THIS BUG
                if( pFace1!=NULL || pFace2!=NULL )
                {
//                    cout << "Gap in a geodesic boundary." << endl;
                }
                pPrevVert = pVert;
            }
            nNum++;
        }
    }

    Mesh.RegisterForceStopCallbackFunction( NULL );
    Mesh.ResetGeodesicMesh();
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::AddPathToMeshVertex
/**
 *  \param  CurPath [T_GeodesicVertexList&] The path to test.
 *  \author Gabriel Peyré
 *  \date   5-14-2003
 *
 *  Test for path intersection and fix cracks.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::AddPathToMeshVertex( GW_GeodesicMesh& Mesh, GW_GeodesicPath& GeodesicPath, T_GeodesicVertexList& VertexPath )
{
    GW_GeodesicVertex* pPrevVert = NULL;
    T_GeodesicPointList& PointList = GeodesicPath.GetPointList();
    for( IT_GeodesicPointList it=PointList.begin(); it!=PointList.end(); ++it )
    {
        GW_GeodesicPoint* pPoint = *it;                        GW_ASSERT( pPoint!=NULL );
        GW_GeodesicVertex* pVert1 = pPoint->GetVertex1();    GW_ASSERT( pVert1!=NULL );
        GW_GeodesicVertex* pVert2 = pPoint->GetVertex2();    GW_ASSERT( pVert2!=NULL );
        GW_Float rCoord = pPoint->GetCoord();
        GW_Bool bIsNewVertCreated;
        GW_GeodesicVertex* pNewVert = (GW_GeodesicVertex*) Mesh.InsertVertexInEdge( *pPoint->GetVertex1(), *pPoint->GetVertex2(), pPoint->GetCoord(), bIsNewVertCreated );
        if( bIsNewVertCreated )
        {
            pNewVert->SetDistance( pPoint->GetVertex1()->GetDistance()*rCoord +
                                   pPoint->GetVertex2()->GetDistance()*(1-rCoord) );
            pNewVert->SetState( pPoint->GetVertex1()->GetState() );
            pNewVert->SetFront( pPoint->GetVertex1()->GetFront() );
        }
        /* add the vertex to the path */
        VertexPath.push_back(pNewVert);

        /* test for self intersection */
        IT_GeodesicPointList itSelf = it;
        itSelf++;
        if( bIsNewVertCreated )        // fix only if new vertex is added
        while( itSelf!=PointList.end() )
        {
            GW_GeodesicPoint* pPointSelf = *itSelf;    GW_ASSERT( pPointSelf!=NULL );
            GW_GeodesicVertex* pVertSelf1 = pPointSelf->GetVertex1();    GW_ASSERT( pVertSelf1!=NULL );
            GW_GeodesicVertex* pVertSelf2 = pPointSelf->GetVertex2();    GW_ASSERT( pVertSelf1!=NULL );
            GW_Float rCoordSelf = pPointSelf->GetCoord();
            if( pVertSelf2==pVert1 && pVertSelf1==pVert2 )
            {
                /* we should swap the two vertex */
                GW_GeodesicVertex* pTemp = pVertSelf1;
                pVertSelf1 = pVertSelf2;
                pVertSelf2 = pTemp;
                rCoordSelf = 1-rCoordSelf;
            }
            if( pVertSelf2==pVert2 && pVertSelf1==pVert1 )
            {
                /* there is a self intersection */
                if( rCoordSelf>rCoord )
                {
                    pPointSelf->SetVertex1( *pVert1 );
                    pPointSelf->SetVertex2( *pNewVert );
                    pPointSelf->SetCoord( (rCoordSelf-rCoord)/(1-rCoord) );
                }
                else
                {
                    pPointSelf->SetVertex1( *pNewVert );
                    pPointSelf->SetVertex2( *pVert2 );
                    if( rCoord>GW_EPSILON )
                        pPointSelf->SetCoord( rCoordSelf/rCoord );
                    else
                        pPointSelf->SetCoord(0);
                }
            }
            itSelf++;
        }

        /* now test for intersection with previous path */
        GW_U32 nID = GW_Vertex::ComputeUniqueId( *pVert1, *pVert2 );

        /* to avoid to reprocess already processed lists */
        std::list<T_GeodesicVertexList*> PathToProccess;
        IT_VertexPathMultiMultiMap itIntersectedPath = BoundaryEdgeMap_.find(nID);
        if( bIsNewVertCreated )        // fix only if new vertex is added
        while( itIntersectedPath!=BoundaryEdgeMap_.end() &&  itIntersectedPath->first==nID )
        {
            PathToProccess.push_back( itIntersectedPath->second );
            itIntersectedPath++;
        }
        for( std::list<T_GeodesicVertexList*>::iterator itIntersectedPath=PathToProccess.begin(); itIntersectedPath!=PathToProccess.end(); ++itIntersectedPath )
        {
            /* oups, intersection, we should fix it */
            T_GeodesicVertexList* pIntersectedPath = *itIntersectedPath;
            GW_ASSERT( pIntersectedPath!=NULL );
            /* find where the crossing occurs */
            IT_GeodesicVertexList itIntersec = pIntersectedPath->begin();
            GW_GeodesicVertex* pIntersect1 = NULL;
            GW_GeodesicVertex* pIntersect2 = NULL;
            while( !(itIntersec==pIntersectedPath->end()) )
            {
                pIntersect2 = *itIntersec;
                if( (pIntersect1==pVert1 && pIntersect2==pVert2) ||
                    (pIntersect1==pVert2 && pIntersect2==pVert1) )
                    break;        // we have found the position in the path
                pIntersect1 = pIntersect2;
                itIntersec++;
            }
            if( pIntersect1!=NULL && pIntersect2!=NULL && itIntersec!=pIntersectedPath->end() )
            {
                /* insert new vertex in path */
                pIntersectedPath->insert( itIntersec, pNewVert );
                /* divide previous edge in two */
                // BoundaryEdgeMap_.erase( nID );    // \todo find the correct iterator and remove it
                nID = GW_Vertex::ComputeUniqueId( *pVert1, *pNewVert );
                BoundaryEdgeMap_.insert( std::pair<GW_U32, T_GeodesicVertexList*>( nID, pIntersectedPath ) );
                nID = GW_Vertex::ComputeUniqueId( *pVert2, *pNewVert );
                BoundaryEdgeMap_.insert( std::pair<GW_U32, T_GeodesicVertexList*>( nID, pIntersectedPath ) );
            }
            else
                GW_ASSERT( GW_False );
        }
        /* add the small newly created edge in the map */
        if( pPrevVert==NULL )
            pPrevVert = pNewVert;
        else
        {
            nID = GW_Vertex::ComputeUniqueId( *pPrevVert, *pNewVert );
            pPrevVert = pNewVert;
            BoundaryEdgeMap_.insert( std::pair<GW_U32, T_GeodesicVertexList*>( nID, &VertexPath ) );
        }
    }
}

/*------------------------------------------------------------------------------*/
// Name : FastMarchingCallbackFunction_Parametrization
/**
*  \author Gabriel Peyré
*  \date   4-12-2003
*
*  A callback function for geodesic computations.
*/
/*------------------------------------------------------------------------------*/
GW_Bool GW_VoronoiMesh::FastMarchingCallbackFunction_Parametrization( GW_GeodesicVertex& CurVert )
{
    GW_ASSERT( pCurrentVoronoiVertex_!=NULL );

    if( !CurVert.GetIsStoppingVertex() || !CurVert.GetBoundaryReached() )
    {
        /* add the parameter value */
        CurVert.AddParameterVertex( *pCurrentVoronoiVertex_, CurVert.GetDistance() );
        if( CurVert.GetIsStoppingVertex() )
            CurVert.SetBoundaryReached( GW_True );
    }
    /* always continue the computations */
    return GW_False;
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::PerformLocalFastMarching
/**
 *  \param  Mesh [GW_GeodesicMesh&] The vertex around which we perform fast marching.
 *  \param  Vert [GW_VoronoiVertex&] The base mesh.
 *  \author Gabriel Peyré
 *  \date   4-28-2003
 *
 *  Perform a fast marching on the voronoi faces around a given
 *  vertex.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::PerformLocalFastMarching( GW_GeodesicMesh& Mesh, GW_VoronoiVertex& Vert )
{
    Mesh.ResetGeodesicMesh();
    /* set up a boundary to stop the front propagation */
    for( GW_FaceIterator it=Vert.BeginFaceIterator(); it!=Vert.EndFaceIterator(); ++it )
    {
        GW_Face* pFace = *it;
        GW_ASSERT( pFace!=NULL );
        GW_VoronoiVertex* pVert1 = (GW_VoronoiVertex*) pFace->GetNextVertex( Vert );
        GW_VoronoiVertex* pVert2 = (GW_VoronoiVertex*) pFace->GetNextVertex( *pVert1 );
        GW_ASSERT( pVert1!=NULL );
        GW_ASSERT( pVert2!=NULL );
        GW_U32 nID = GW_Vertex::ComputeUniqueId( *pVert1, *pVert2 );
        /* get the geodesic edge */
        GW_ASSERT( VertexPathMap_.find(nID)!=VertexPathMap_.end() );
        T_GeodesicVertexList* pVertexGeodesicEdge = VertexPathMap_[nID];
        GW_ASSERT( pVertexGeodesicEdge!=NULL );
        if( pVertexGeodesicEdge!=NULL )
        for( IT_GeodesicVertexList VertIt = pVertexGeodesicEdge->begin(); VertIt!=pVertexGeodesicEdge->end(); ++VertIt )
        {
            GW_GeodesicVertex* pGeodesicVert = *VertIt;
            pGeodesicVert->SetStoppingVertex( GW_True );
        }
    }
    /* now compute the fast marching */
    GW_ASSERT( Vert.GetBaseVertex()!=NULL );
    Mesh.PerformFastMarching( Vert.GetBaseVertex() );
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::BuildGeodesicParametrization
/**
 *  \author Gabriel Peyré
 *  \date   4-26-2003
 *
 *  Compute the parameter of each vertex of the base mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::BuildGeodesicParametrization( GW_GeodesicMesh& Mesh )
{
    Mesh.RegisterForceStopCallbackFunction( FastMarchingCallbackFunction_Parametrization );
    Mesh.ResetParametrizationData();
    /* for each base vertex, perform front propagation on the surrounding faces */
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        CurrentTargetVertex_.clear();
        pCurrentVoronoiVertex_ = (GW_VoronoiVertex*) this->GetVertex(i);    // set up global data for callback function
        GW_ASSERT( pCurrentVoronoiVertex_!=NULL );

        GW_OutputComment("Performing local fast marching.");
        this->PerformLocalFastMarching( Mesh, *pCurrentVoronoiVertex_ );

        /* register geodesic boundaries length */
        for( GW_VertexIterator it=pCurrentVoronoiVertex_->BeginVertexIterator(); it!=pCurrentVoronoiVertex_->EndVertexIterator(); ++it )
        {
            GW_VoronoiVertex* pNeighbor = (GW_VoronoiVertex*) *it;
            GW_ASSERT( pNeighbor!=NULL );
            GW_GeodesicVertex* pGeodesicVert = pNeighbor->GetBaseVertex();
            GW_ASSERT( pGeodesicVert!=NULL );
            GW_U32 nID = GW_Vertex::ComputeUniqueId( *pCurrentVoronoiVertex_, *pNeighbor );
            if( GeodesicDistanceMap_.find(nID)==GeodesicDistanceMap_.end() )
            {
                GW_ASSERT( pGeodesicVert->GetState()==GW_GeodesicVertex::kDead );
                /* this is the first time the distance is computed */
                if( pGeodesicVert->GetState()==GW_GeodesicVertex::kDead )
                    GeodesicDistanceMap_[nID]     = pGeodesicVert->GetDistance();
            }
            else
            {
                GW_ASSERT( pGeodesicVert->GetState()==GW_GeodesicVertex::kDead );
                if( pGeodesicVert->GetState()==GW_GeodesicVertex::kDead )
                {
                    /* distance already computed in the reverse direction : take the average */
                    GW_Float rPrevDist = GeodesicDistanceMap_[nID];
                    GW_Float rNewDist = pGeodesicVert->GetDistance();
                    GeodesicDistanceMap_[nID] = GW_MIN(rPrevDist, rNewDist); //(rPrevDist+rNewDist)*0.5;
                }
            }
        }

    }

    /* now really compute the 3 parameters for each vertex */
    GW_OutputComment("Computing geodesic parametrisation.");
    this->ComputeVertexParameters( Mesh );

    pCurrentVoronoiVertex_ = NULL;
    Mesh.RegisterForceStopCallbackFunction( NULL );

}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::ComputeVertexParameters
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh.
 *  \author Gabriel Peyré
 *  \date   4-27-2003
 *
 *  Using geodesic distance from 3 base vertex, compute the
 *  parameter for each vertex.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::ComputeVertexParameters( GW_GeodesicMesh& Mesh )
{
    for( GW_U32 i=0; i<Mesh.GetNbrVertex(); ++i )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Mesh.GetVertex( i );
        GW_ASSERT( pVert!=NULL );
        GW_Float d0, d1, d2;
        GW_VoronoiVertex* pVornoiVert0 = pVert->GetParameterVertex( 0, d0 );
        GW_VoronoiVertex* pVornoiVert1 = pVert->GetParameterVertex( 1, d1 );
        GW_VoronoiVertex* pVornoiVert2 = pVert->GetParameterVertex( 2, d2 );
        if( pVornoiVert0!=NULL && pVornoiVert1!=NULL && pVornoiVert2!=NULL )
        {
            GW_ASSERT( pVornoiVert0!=NULL );
            GW_ASSERT( pVornoiVert1!=NULL );
            GW_ASSERT( pVornoiVert2!=NULL );
            GW_GeodesicVertex* pVert0 = pVornoiVert0->GetBaseVertex();
            GW_GeodesicVertex* pVert1 = pVornoiVert1->GetBaseVertex();
            GW_GeodesicVertex* pVert2 = pVornoiVert2->GetBaseVertex();
            GW_ASSERT( pVert0!=NULL );
            GW_ASSERT( pVert1!=NULL );
            GW_ASSERT( pVert2!=NULL );
            GW_U32 nID0 = GW_Vertex::ComputeUniqueId( *pVornoiVert1, *pVornoiVert2 );
            GW_U32 nID1 = GW_Vertex::ComputeUniqueId( *pVornoiVert0, *pVornoiVert2 );
            GW_U32 nID2 = GW_Vertex::ComputeUniqueId( *pVornoiVert0, *pVornoiVert1 );
            if( GeodesicDistanceMap_.find(nID0)!=GeodesicDistanceMap_.end() &&
                GeodesicDistanceMap_.find(nID1)!=GeodesicDistanceMap_.end() &&
                GeodesicDistanceMap_.find(nID2)!=GeodesicDistanceMap_.end() )
            {
                /* length of each triangle segment. */
                GW_Float l0 = GeodesicDistanceMap_[nID0];
                GW_Float l1 = GeodesicDistanceMap_[nID1];
                GW_Float l2 = GeodesicDistanceMap_[nID2];

                /* now use Heron rule to compute parameter */
                GW_Float a0 = GW_Maths::ComputeTriangleArea( l0, d1, d2 );
                GW_Float a1 = GW_Maths::ComputeTriangleArea( d0, l1, d2 );
                GW_Float a2 = GW_Maths::ComputeTriangleArea( d0, d1, l2 );
                GW_Float t = GW_Maths::ComputeTriangleArea( l0, l1, l2 );    // area of the big triangle
                GW_Float a = a0 + a1 + a2;
                GW_ASSERT( a>GW_EPSILON );
                /* test if the point is outside */
                if( a1+a2>t )
                    pVert->SetParameterVertex( 0, a1/(a1+a2), a2/(a1+a2) );
                else if( a0+a2>t )
                    pVert->SetParameterVertex( a0/(a0+a2), 0, a2/(a0+a2) );
                else if( a0+a1>t )
                    pVert->SetParameterVertex( a0/(a0+a1), a1/(a0+a1), 0 );
                else
                {
                    /* the point is inside the triangle */
                    pVert->SetParameterVertex( a0/a, a1/a, a2/a );
                }
                pVert->SetParameterVertex( a0/a, a1/a, a2/a );
            }
            else
            {
                pVert->SetParameterVertex( 0, 0, 0 );
            }
        }
    }

    /* fix parameter for voronoi vertex */
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        GW_VoronoiVertex* pVoronoiVert0 = (GW_VoronoiVertex*) this->GetVertex( i );
        GW_ASSERT( pVoronoiVert0!=NULL );
        GW_GeodesicVertex* pVert = pVoronoiVert0->GetBaseVertex();
        GW_ASSERT( pVert!=NULL );
        GW_Face* pFace = pVoronoiVert0->GetFace();
        GW_ASSERT( pFace!=NULL );
        GW_VoronoiVertex* pVoronoiVert1 = (GW_VoronoiVertex*) pFace->GetNextVertex( *pVoronoiVert0 );
        GW_ASSERT( pVoronoiVert1!=NULL );
        GW_VoronoiVertex* pVoronoiVert2 = (GW_VoronoiVertex*) pFace->GetNextVertex( *pVoronoiVert1 );
        GW_ASSERT( pVoronoiVert2!=NULL );
        pVert->ResetParametrizationData();
        pVert->AddParameterVertex( *pVoronoiVert0, 1 );
        pVert->AddParameterVertex( *pVoronoiVert1, 0 );
        pVert->AddParameterVertex( *pVoronoiVert2, 0 );
    }

    /* find the center of each voronoi face */
    CentralParameterMap_.clear();
    T_FloatMap BestVertexValue;
    for( GW_U32 i=0; i<Mesh.GetNbrVertex(); ++i )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Mesh.GetVertex( i );
        GW_ASSERT( pVert!=NULL );
        GW_Float a,b,c;
        GW_VoronoiVertex* pVert0 = pVert->GetParameterVertex( 0, a );
        GW_ASSERT( pVert0!=NULL );
        GW_VoronoiVertex* pVert1 = pVert->GetParameterVertex( 1, b );
        GW_ASSERT( pVert1!=NULL );
        GW_VoronoiVertex* pVert2 = pVert->GetParameterVertex( 2, c );
        if( pVert2!=NULL )
        {
            GW_Float rBestDist = GW_INFINITE;
            GW_U32 nID = GW_Vertex::ComputeUniqueId( *pVert0, *pVert1, *pVert2 );
            if( BestVertexValue.find(nID)!=BestVertexValue.end() )
                rBestDist = BestVertexValue[nID];
            GW_Float rDist = GW_ABS(a - 1.0/3.0) + GW_ABS(b - 1.0/3.0) + GW_ABS(c - 1.0/3.0);
            if( rDist<rBestDist )
            {
                BestVertexValue[nID] = rDist;
                CentralParameterMap_[nID] = pVert;
            }
        }
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::ResetOnlyVertexState
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh.
 *  \author Gabriel Peyré
 *  \date   5-13-2003
 *
 *  Resst only the state of each vertex of the mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::ResetOnlyVertexState( GW_GeodesicMesh& Mesh )
{
    for( GW_U32 i = 0; i<Mesh.GetNbrVertex(); ++i  )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Mesh.GetVertex(i);
        GW_ASSERT( pVert!=NULL );
        pVert->SetState( GW_GeodesicVertex::kFar );
    }
}



void GW_VoronoiMesh::PrepareInterpolation( GW_GeodesicMesh& Mesh )
{
    /* duplicate the geodesic data for each of the vertex */
    for( GW_U32 i=0; i<Mesh.GetNbrVertex(); ++i )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Mesh.GetVertex(i);
        GW_ASSERT( pVert!=NULL );
        GW_GeodesicInformationDuplicata* pDuplicata = new GW_GeodesicInformationDuplicata( *pVert );
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionNN
/**
*  \param  CurVert [GW_GeodesicVertex&] The current vertex.
*  \param  rNewDist [GW_Float] The new distance.
*  \return [GW_Bool] Insert this vertex to active list ?
*  \author Gabriel Peyré
*  \date   5-13-2003
*
*  Add the vertex only if the distance decreases.
*/
/*------------------------------------------------------------------------------*/
GW_Bool GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionNN( GW_GeodesicVertex& CurVert, GW_Float rNewDist )
{
    GW_GeodesicInformationDuplicata* pDuplicata = (GW_GeodesicInformationDuplicata*) CurVert.GetUserData();
    GW_ASSERT( pDuplicata!=NULL );
    return pDuplicata->rDistance_+GW_EPSILON >= rNewDist;
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FlattenBasePoints
/**
 *  \param  Mesh [GW_GeodesicMesh&] The original mesh.
 *  \param  FlatteningMap [T_Vector2DMap&] A link between the basis geodesic vertex and their 2D positions.
 *  \author Gabriel Peyré
 *  \date   6-1-2003
 *
 *  Compute a 2D position for each base point, using multidimensional scaling.
 */
/*------------------------------------------------------------------------------*/
void GW_VoronoiMesh::FlattenBasePoints( GW_GeodesicMesh& Mesh, T_Vector2DMap& FlatteningMap )
{
    char str[100];
    /* create a voronoi vertex for each basis vertex */
    this->CreateVoronoiVertex();

    GW_U32 n = this->GetNbrVertex();
    GW_MatrixNxP dist_matrix(n,n,-1);


    /* compute the distance matrix */
    for( GW_U32 i=0; i<n; ++i )
    {
        GW_VoronoiVertex* pVoronoiVerti = (GW_VoronoiVertex*) this->GetVertex(i);                    GW_ASSERT( pVoronoiVerti!=NULL );
        GW_GeodesicVertex* pGeodesicVerti = (GW_GeodesicVertex*) pVoronoiVerti->GetBaseVertex();    GW_ASSERT( pGeodesicVerti!=NULL );

        sprintf( str, "Computing distance information for vertex %d.", i );
        GW_OutputComment( str );

        /* compute the distance map */
        Mesh.ResetGeodesicMesh();
        Mesh.PerformFastMarching( pGeodesicVerti );

        for( GW_U32 j=0; j<n; ++j )
        {
            GW_VoronoiVertex* pVoronoiVertj = (GW_VoronoiVertex*) this->GetVertex(j);                    GW_ASSERT( pVoronoiVerti!=NULL );
            GW_GeodesicVertex* pGeodesicVertj = (GW_GeodesicVertex*) pVoronoiVertj->GetBaseVertex();    GW_ASSERT( pGeodesicVerti!=NULL );
            GW_Float rDist = pGeodesicVertj->GetDistance();
            if( dist_matrix[i][j]<0 )
                dist_matrix[i][j] = rDist*rDist;
            else
                dist_matrix[i][j] = (dist_matrix[i][j]+rDist*rDist)*0.5;

            if( dist_matrix[j][i]<0 )
                dist_matrix[j][i] = rDist*rDist;
            else
                dist_matrix[j][i] = (dist_matrix[j][i]+rDist*rDist)*0.5;
        }
    }

    /* center the matrix */
    sprintf( str, "Performing the multidimensional scaling.", i );
    GW_OutputComment( str );
    GW_MatrixNxP J(n,n,-1.0/n);
    for( GW_U32 i=0; i<n; ++i )
        J[i][i] += 1;

    /* center the matrix */
    dist_matrix = J*dist_matrix*J*(-0.5);

    /* perform eigen-decomposition */
    GW_MatrixNxP v(n,n);
    GW_VectorND RealEig(n);
    dist_matrix.Eigenvalue( v, NULL, &RealEig, NULL );

    GW_Float rScaleX = 0;
    GW_Float rScaleY = 0;
    GW_I32 nEigenvectorX = -1;
    GW_I32 nEigenvectorY = -1;
    /* find the two maximum eigenvalues */
    for( GW_U32 i=0; i<n; ++i )
    {
        if( RealEig[i]>=rScaleX )
        {
            rScaleY = rScaleX;
            nEigenvectorY = nEigenvectorX;
            rScaleX = RealEig[i];
            nEigenvectorX = i;
        }
        else if( RealEig[i]>=rScaleY )
        {
            rScaleY = RealEig[i];
            nEigenvectorY = i;
        }
    }
    rScaleX = sqrt(rScaleX);
    rScaleY = sqrt(rScaleY);
    GW_ASSERT( nEigenvectorX>=0 );
    GW_ASSERT( nEigenvectorY>=0 );

    /* compute the 2D position */
    FlatteningMap.clear();
    if( nEigenvectorX>=0 && nEigenvectorY>=0 )
    for( GW_U32 i=0; i<n; ++i )
    {
        GW_VoronoiVertex* pVoronoiVerti = (GW_VoronoiVertex*) this->GetVertex(i);                    GW_ASSERT( pVoronoiVerti!=NULL );
        GW_GeodesicVertex* pGeodesicVerti = (GW_GeodesicVertex*) pVoronoiVerti->GetBaseVertex();    GW_ASSERT( pGeodesicVerti!=NULL );

        GW_Vector2D pos;
        pos[0] = rScaleX*v[i][nEigenvectorX];
        pos[1] = rScaleY*v[i][nEigenvectorY];
        FlatteningMap[ pGeodesicVerti->GetID() ] = pos;
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionRD1
/**
*  \param  CurVert [GW_GeodesicVertex&] The current vertex.
*  \param  rNewDist [GW_Float] The new distance.
*  \return [GW_Bool] Insert this vertex to active list ?
*  \author Gabriel Peyré
*  \date   5-13-2003
*
*  Here we only record the natural neighbors in the weights map.
*/
/*------------------------------------------------------------------------------*/
GW_Bool GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionRD1( GW_GeodesicVertex& CurVert, GW_Float rNewDist )
{
    GW_ASSERT( pCurWeights_!=NULL );
    GW_GeodesicInformationDuplicata* pDuplicata = (GW_GeodesicInformationDuplicata*) CurVert.GetUserData();
    GW_ASSERT( pDuplicata!=NULL );
    GW_U32 nId = pDuplicata->pFront_->GetID();
    (*pCurWeights_)[ nId ] = -1;    // this is one of our natural neighbor, yeah.
    return pDuplicata->rDistance_+GW_EPSILON >= rNewDist;
}


/*------------------------------------------------------------------------------*/
// Name : GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionRD2
/**
*  \param  CurVert [GW_GeodesicVertex&] The current vertex.
*  \param  rNewDist [GW_Float] The new distance.
*  \return [GW_Bool] Insert this vertex to active list ?
*  \author Gabriel Peyré
*  \date   5-13-2003
*
*  Here we record the reciprocical of the distance.
*/
/*------------------------------------------------------------------------------*/
GW_Bool GW_VoronoiMesh::FastMarchingCallbackFunction_VertexInsersionRD2( GW_GeodesicVertex& CurVert, GW_Float rNewDist )
{
    GW_ASSERT( pCurWeights_!=NULL );
    GW_U32 nId = CurVert.GetID();
    if( pCurWeights_->find(nId)!=pCurWeights_->end() )    // this is one of our natural neighbor, yeah.
    {
        if( (*pCurWeights_)[nId]<0 )
        {
            /* first time we encounter this vertex, sounds good */
            if( rNewDist>0 )
                (*pCurWeights_)[nId] = 1.0/(rNewDist);
            else
                (*pCurWeights_)[nId] = GW_INFINITE;
            nNbrBaseVertex_RD_++;
        }
        else
            GW_ASSERT( GW_False );
    }
    return GW_True;
}


GW_Bool GW_VoronoiMesh::FastMarchingCallbackFunction_ForceStopRD( GW_GeodesicVertex& Vert )
{
    return nNbrBaseVertex_RD_>=pCurWeights_->size();
}





///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
