/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Parameterization.cpp
 *  \brief  Definition of class \c GW_Parameterization
 *  \author Gabriel Peyré
 *  \date   7-1-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_Parameterization.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_Parameterization.h"

#ifndef GW_USE_INLINE
    #include "GW_Parameterization.inl"
#endif

using namespace GW;


GW_GeodesicVertex* GW_Parameterization::pCurVoronoiDiagram_ = NULL;
T_U32Vector GW_Parameterization::FaceVector;
T_U32Map GW_Parameterization::FaceMap;
T_U32Vector GW_Parameterization::VertexVector;
T_U32Map GW_Parameterization::VertexMap;


GW_Parameterization::GW_Parameterization()
:    nNbrMaxIterLloyd_        ( 20 ),
    nNbrMaxIterLloydDescent_    ( 100 )
{

}

GW_Parameterization::~GW_Parameterization()
{

}


/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::PerformFastMarching
/**
*  \param  VertList [T_GeodesicVertexList&] List of start vertex.
*  \author Gabriel Peyré
*  \date   4-13-2003
*
*  Just an helper method that launch a fire from each base point.
*/
/*------------------------------------------------------------------------------*/
void GW_Parameterization::PerformFastMarching(  GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList )
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

/** We add a new point only if it belongs to the current Voronoi diagram */
GW_Bool GW_Parameterization::FastMarchingCallbackFunction_Centering( GW_GeodesicVertex& CurVert, GW_Float rNewDist )
{
    typedef GW_VoronoiMesh::GW_GeodesicInformationDuplicata T_Duplicata;
    GW_ASSERT( pCurVoronoiDiagram_!=NULL );
    T_Duplicata* pDuplicata = (T_Duplicata*) CurVert.GetUserData();
    GW_ASSERT( pDuplicata!=NULL );
    return pDuplicata->pFront_ == pCurVoronoiDiagram_;
}

GW_GeodesicVertex& GW_Parameterization::PerformCentering( GW_GeodesicVertex& Vert, GW_GeodesicMesh& Mesh )
{
    GW_OutputComment( "Centering a base vertex." );
    /* try a pseudo gradient descent to seek for the new points */
    T_FloatMap ComputedEnergy;    // already computed energy

    GW_GeodesicVertex* pCurVertex = &Vert;
    GW_Float rCurEnergy = ComputeCenteringEnergy( *pCurVertex, Vert, Mesh );
    ComputedEnergy[pCurVertex->GetID()] = rCurEnergy;

    GW_U32 nNbrIter = 0;

    while( true )
    {
        GW_GeodesicVertex* pNextVertex = NULL;

        /* iterate on the 1-ring */
        for( GW_VertexIterator it=pCurVertex->BeginVertexIterator(); it!=pCurVertex->EndVertexIterator(); ++it )
        {
            GW_GeodesicVertex* pNeigbhor = (GW_GeodesicVertex*) *it;
            GW_Float rNeighborEnergy = 0;
            GW_ASSERT( pNeigbhor!=NULL );

            if( ComputedEnergy.find(pNeigbhor->GetID())==ComputedEnergy.end() )
            {
                /* we should compute it's enerdy */
                rNeighborEnergy = ComputeCenteringEnergy( *pNeigbhor, Vert, Mesh );
                ComputedEnergy[pNeigbhor->GetID()] = rNeighborEnergy;
            }
            else
                rNeighborEnergy = ComputedEnergy[ pNeigbhor->GetID() ];

            if( rNeighborEnergy<rCurEnergy )
            {
                rCurEnergy = rNeighborEnergy;
                pNextVertex = pNeigbhor;
            }
        }

        /* test if we are on a minima */
        if( pNextVertex==NULL )
            break;
        pCurVertex = pNextVertex;

        nNbrIter++;
        if( nNbrIter>nNbrMaxIterLloydDescent_ )    // to avoid infinite loop.
            break;
    }

    /* return the new point location */
    return *pCurVertex;
}



/** Return \c true if we have reach a fixed point of the iterations. */
GW_Bool GW_Parameterization::PerformLloydIteration( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList )
{
    /* recompute the whole distance map */
    this->PerformFastMarching( Mesh, VertList );
    /* duplicate the Marching information */
    GW_VoronoiMesh::PrepareInterpolation( Mesh );

    /* we don't want to use some special energy */
    Mesh.RegisterWeightCallbackFunction( GW_GeodesicMesh::BasicWeightCallback );

    T_GeodesicVertexList NewVertList;
    GW_Bool bStableConfigReached = GW_True;
    for( IT_GeodesicVertexList it=VertList.begin(); it!=VertList.end(); ++it )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) *it;
        GW_ASSERT( pVert!=NULL );
        GW_GeodesicVertex& NewVert = PerformCentering( *pVert, Mesh );
        if( &NewVert!=pVert )
            bStableConfigReached = GW_False;
        NewVertList.push_back( &NewVert );
    }
    /* replace the old list by the new one */
    VertList = NewVertList;

    return bStableConfigReached;
}

void GW_Parameterization::PerformLloydAlgorithm( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList )
{
    GW_U32 nNbrIter = 0;
    while( nNbrIter<nNbrMaxIterLloyd_ )
    {
        nNbrIter++;
        GW_OutputComment( "Performing an iteration of Lloyd algorithm." );
        if( PerformLloydIteration(Mesh, VertList ) )
            break;
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::SegmentRegion
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh to segment.
 *  \param  Vert [GW_GeodesicVertex&] The seed vertex.
 *  \author Gabriel Peyré
 *  \date   7-1-2003
 *
 *  Cut the mesh according to the voronoi diagrams.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::SegmentRegion( GW_GeodesicMesh& Mesh, GW_GeodesicVertex& Vert, T_TrissectorInfoMap* pTrissectorInfoMap )
{
    /* march on the voronoi diagram */
    T_FaceList FaceToProceed;
    FaceToProceed.push_back( Vert.GetFace() );
    T_FaceMap FaceMap;
    FaceMap[ Vert.GetFace()->GetID() ] = Vert.GetFace();


    while( !FaceToProceed.empty() )
    {
        GW_Face* pFace = FaceToProceed.front();
        GW_ASSERT( pFace!=NULL );
        FaceToProceed.pop_front();

        /* cut the face */
        this->CutFace( *pFace, Vert, Mesh, pTrissectorInfoMap );

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
}

/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::SegmentAllRegions
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh.
 *  \param  VertList [T_GeodesicVertexList&] The list of the seed points.
 *  \author Gabriel Peyré
 *  \date   7-1-2003
 *
 *  Segment all the regions of a mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::SegmentAllRegions( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList, T_TrissectorInfoMap* pTrissectorInfoMap )
{
    CutEdgeMap_.clear();
    /* recompute the whole distance map */
    GW_OutputComment("Recomputing the whole Voronoi tessellation.");
    this->PerformFastMarching( Mesh, VertList );
    cout << "  * Segmenting each region ";
    GW_U32 num = 0;
    GW_ProgressBar pb;
    pb.Begin();
    for( IT_GeodesicVertexList it=VertList.begin(); it!=VertList.end(); ++it )
    {
        GW_GeodesicVertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        this->SegmentRegion( Mesh, *pVert, pTrissectorInfoMap );
        num++;
        pb.Update( ((GW_Float)num)/((GW_Float)VertList.size()) );
    }
    pb.End();
    cout << endl;
    Mesh.BuildConnectivity();

    if( pTrissectorInfoMap!=NULL )
    {
        /* build the map that perform conversion from front number to region number */
        T_U32Map FrontNbr2RegionNbr;
        GW_U32 num = 0;
        for( IT_GeodesicVertexList it = VertList.begin(); it!=VertList.end(); ++it )
        {
            GW_GeodesicVertex* pVert = *it; GW_ASSERT( pVert!=NULL );
            FrontNbr2RegionNbr[pVert->GetID()]  =num;
            num++;
        }
        /* replace the number of the vertices by the number of the front */
        for( IT_TrissectorInfoMap it = pTrissectorInfoMap->begin(); it!=pTrissectorInfoMap->end(); ++it )
        {
            GW_TrissectorInfo& tr = it->second;
            for( GW_U32 i=0; i<3; ++i )
            {
                GW_U32 nID = tr.GetId( i );
                GW_ASSERT( FrontNbr2RegionNbr.find(nID)!=FrontNbr2RegionNbr.end() );
                tr.SetId( i, FrontNbr2RegionNbr[nID] );
            }
        }

    }
}



GW_Vector3D TotalNormal;
GW_Vector3D TotalCenter;
void ExplodeRegion_Callback1( GW_Vertex& vert )
{
    TotalCenter += vert.GetPosition();
    TotalNormal += vert.GetNormal();
}

GW_Vector3D Displacement;
void ExplodeRegion_Callback2( GW_Vertex& vert )
{
    vert.GetPosition() += Displacement;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::ExplodeRegion
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh.
 *  \param  Vert [GW_GeodesicVertex&] A seed for the region.
 *  \author Gabriel Peyré
 *  \date   7-2-2003
 *
 *  Explode a given region.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::ExplodeRegion( GW_GeodesicVertex& Vert, GW_Float intensity, GW_Float normal_contrib )
{
    TotalNormal.SetZero();
    TotalCenter.SetZero();
    GW_Mesh::IterateConnectedComponent_Vertex( Vert, ExplodeRegion_Callback1 );

    TotalNormal.Normalize();
    TotalCenter.Normalize();
    Displacement = TotalNormal*normal_contrib + TotalCenter*(1-normal_contrib);
    Displacement.Normalize();
    Displacement *= intensity;
    GW_Mesh::IterateConnectedComponent_Vertex( Vert, ExplodeRegion_Callback2 );
}

/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::ExplodeAllRegions
/**
 *  \param  VertList [T_GeodesicVertexList&] The list of seed.
 *  \param  intensity [GW_Float] intensity of the explode.
 *  \param  normal_contrib [GW_Float] DESCRIPTION
 *  \author Gabriel Peyré
 *  \date   7-2-2003
 *
 *  Explode each region.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::ExplodeAllRegions( T_GeodesicVertexList& VertList, GW_Float intensity, GW_Float normal_contrib )
{
    for( IT_GeodesicVertexList it=VertList.begin(); it!=VertList.end(); ++it )
    {
        GW_GeodesicVertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        GW_Parameterization::ExplodeRegion( *pVert, intensity, normal_contrib );
    }
}


void GW_Parameterization::ExtractSubMeshes_Callback( GW_Face& face )
{
    GW_U32 nNbrFace = (GW_U32) FaceVector.size();
    FaceVector.push_back( face.GetID() );
    FaceMap[face.GetID()] = nNbrFace;
    for( GW_U32 i=0; i<3; ++i )
    {
        GW_Vertex* pVert = face.GetVertex(i); GW_ASSERT( pVert!=NULL );
        if( VertexMap.find(pVert->GetID())==VertexMap.end() )
        {
            GW_U32 nNbrVert = (GW_U32) VertexVector.size();
            VertexMap[pVert->GetID()] = nNbrVert;
            VertexVector.push_back(pVert->GetID());
        }
    }
}
/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::ExtractSubMeshes
/**
 *  \param  Mesh [GW_GeodesicMesh&] The original mesh.
 *  \param  VertList [T_GeodesicVertexList&] A seed for each component
 *  \param  MeshVector [T_MeshVector&] The place where new meshes are put.
 *  \author Gabriel Peyré
 *  \date   1-14-2004
 *
 *  Create a new mesh for each connected component.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::ExtractSubMeshes( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList, T_GeodesicMeshVector& MeshVector, T_U32Vector* pGlobal2LocalID )
{
    if( pGlobal2LocalID!=NULL )
        pGlobal2LocalID->resize(Mesh.GetNbrVertex());
    for( IT_GeodesicVertexList it = VertList.begin(); it!=VertList.end(); ++it )
    {
        GW_GeodesicVertex* pVert = *it; GW_ASSERT(pVert!=NULL);
        FaceVector.clear();
        FaceMap.clear();
        VertexVector.clear();
        VertexMap.clear();
        GW_Mesh::IterateConnectedComponent_Face( * pVert->GetFace(), ExtractSubMeshes_Callback );
        /* create a new mesh */
        GW_U32 nNbrVert = (GW_U32) VertexVector.size();    // already computed
        GW_U32 nNbrFace = (GW_U32) FaceVector.size();
        GW_GeodesicMesh* pNewMesh = new GW_GeodesicMesh;
        MeshVector.push_back(pNewMesh);
        pNewMesh->SetNbrVertex( nNbrVert );
        pNewMesh->SetNbrFace( nNbrFace );
        for( GW_U32 i=0; i<nNbrFace; ++i )
        {
            /* create the face */
            GW_Face& NewFace = pNewMesh->CreateNewFace();
            pNewMesh->SetFace(i, &NewFace);
        }
        for( GW_U32 i=0; i<nNbrVert; ++i )
        {
            /* create the vertex */
            GW_Vertex& NewVert = pNewMesh->CreateNewVertex();
            pNewMesh->SetVertex(i, &NewVert);
            /* copy the vertex */
            GW_U32 nID = VertexVector[i];
            const GW_Vertex& OriginalVert = *Mesh.GetVertex(nID);
            NewVert = OriginalVert;
            NewVert.SetID(i);    // this was changed by the copy
            /* keep track of the correspondence */
            if( pGlobal2LocalID!=NULL )
            {
                GW_ASSERT( nID<pGlobal2LocalID->size() );
                (*pGlobal2LocalID)[nID] = i;
            }
            /* resolve face attachment */
            const GW_Face* pFace = OriginalVert.GetFace();
            if( pFace!=NULL )
            {
                // GW_ASSERT( FaceMap.find(pFace->GetID())!=FaceMap.end() );
                if( FaceMap.find(pFace->GetID())!=FaceMap.end() )
                {
                    GW_U32 nID = FaceMap[ pFace->GetID() ];
                    NewVert.SetFace( * pNewMesh->GetFace( nID ) );
                }
                else
                {
                    /* problem : this vertex is attached to another region. */
                    GW_ASSERT( GW_False );
                }
            }
        }
        /* create the faces */
        for( GW_U32 i=0; i<nNbrFace; ++i )
        {
            /* create the face */
            GW_Face& NewFace = *pNewMesh->GetFace(i);
            /* copy the face */
            GW_U32 nID = FaceVector[i];
            const GW_Face& OriginalFace = *Mesh.GetFace(nID);
            NewFace = OriginalFace;
            NewFace.SetID(i); // this was changed
            /* resolve Vertex and neighbor */
            for( GW_U32 j=0; j<3; ++j )
            {
                GW_U32 VertID = OriginalFace.GetVertex(j)->GetID();
                GW_ASSERT( VertexMap.find(VertID)!=VertexMap.end() );
                GW_U32 nID = VertexMap[VertID];
                NewFace.SetVertex( *pNewMesh->GetVertex(nID), j );
                const GW_Face* pNeigh = OriginalFace.GetFaceNeighbor(j);
                if( pNeigh==NULL )
                    NewFace.SetFaceNeighbor(NULL, j);
                else
                {
                    GW_U32 FaceID = pNeigh->GetID();
                    GW_ASSERT( FaceMap.find(FaceID)!=FaceMap.end() );
                    nID = FaceMap[FaceID];
                    NewFace.SetFaceNeighbor( pNewMesh->GetFace( nID ), j);
                }
            }
        }
    }
}



/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::SplitMesh
/**
 *  \param  Mesh [GW_GeodesicMesh&] Mesh to cut
 *  \param  NewMesh [GW_GeodesicMesh&] 1st resulting mesh
 *  \author Gabriel Peyré
 *  \date   1-19-2004
 *
 *  Split a mesh into 2 equal parts. Work only for cylindricaly (without cap)
 *  shaped mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::SplitMesh( GW_GeodesicMesh& Mesh )
{
    Mesh.CheckIntegrity();
    /* First find a boundary vertex *******************************************************************/
    GW_GeodesicVertex* pStartVert = NULL;
    for( GW_U32 i=0; i<Mesh.GetNbrVertex(); ++i  )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Mesh.GetVertex(i);    GW_ASSERT( pVert!=NULL );
        if( pVert->IsBoundaryVertex() )
        {
            pStartVert = pVert;
            break;
        }
    }
    GW_ASSERT( pStartVert!=NULL );
    if( pStartVert==NULL )
        return;
    /* extract the boundary ****************************************************************************/
    T_GeodesicVertexMap BoundaryVerts;
    GW_GeodesicVertex* pCurVert = pStartVert;
    GW_GeodesicVertex* pPrevVert = NULL;
    do{
        BoundaryVerts[pCurVert->GetID()] = pCurVert;
        // select the next vertex
        GW_GeodesicVertex* pNewVert = NULL;
        for( GW_VertexIterator it=pCurVert->BeginVertexIterator(); it!=pCurVert->EndVertexIterator(); ++it )
        {
            GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) *it;
            if( pVert->IsBoundaryVertex()&& pVert!=pPrevVert )
                pNewVert = pVert;
        }
        GW_ASSERT(pNewVert!=NULL);
        pPrevVert = pCurVert;
        pCurVert = pNewVert;
    }
    while(pCurVert!=pStartVert);
    /* Computing geodesic cut **************************************************************/
    GW_OutputComment("Computing geodesic cut.");
    Mesh.ResetGeodesicMesh();
    Mesh.PerformFastMarching( pStartVert );
    /* find the closest point from another boundary ****************************************************/
    GW_GeodesicVertex* pAwayAnother = NULL;
    GW_GeodesicVertex* pAwaySame = NULL;
    GW_Float rAwayAnotherMinDist = GW_INFINITE;
    GW_Float rAwaySameMaxDist = -GW_INFINITE;
    for( GW_U32 i=0; i<Mesh.GetNbrVertex(); ++i )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Mesh.GetVertex(i);    GW_ASSERT( pVert!=NULL );
        if( pVert->IsBoundaryVertex() )
        {
            if( BoundaryVerts.find(pVert->GetID())==BoundaryVerts.end() )
            {
                /* this is a vertex from another boundary */
                if( pVert->GetDistance()<rAwayAnotherMinDist )
                {
                    rAwayAnotherMinDist = pVert->GetDistance();
                    pAwayAnother = pVert;
                }
            }
            else
            {
                /* from the same boundary */
                if( pVert->GetDistance()>rAwaySameMaxDist )
                {
                    rAwaySameMaxDist = pVert->GetDistance();
                    pAwaySame = pVert;
                }
            }
        }
    }
    if( pAwayAnother==NULL )
        pAwayAnother = pAwaySame;
    GW_ASSERT( pAwayAnother!=NULL );
    /* compute geodesic between these two points (the distance from vertex 1 is alreade computed) */
    GW_GeodesicPath GeodesicPath;
    T_GeodesicVertexList VertPath;
    GeodesicPath.ComputePath( *pAwayAnother, 10000 );
    /* add the path to the mesh */
    GW_OutputComment("Extracting the cut.");
    GW_VoronoiMesh::AddPathToMeshVertex( Mesh, GeodesicPath, VertPath );
    /* remove redundant vertices */
    T_GeodesicVertexList VertPath2;
    Mesh.CheckIntegrity();
    pPrevVert = NULL;
    for( IT_GeodesicVertexList it = VertPath.begin(); it!=VertPath.end(); ++it )
    {
        GW_GeodesicVertex* pVert = *it;
        if( pVert!=pPrevVert )
            VertPath2.push_back(pVert);
        pPrevVert = pVert;
    }
    VertPath = VertPath2;
    GW_U32 nNbrVert = (GW_U32) VertPath.size();
    /* keep track of the cutted faces ***************************************************************/
    GW_GeodesicVertex* pVert1 = NULL;
    GW_Face* pFace1 = NULL;
    GW_Face* pFace2 = NULL;
    T_FaceList* FaceListPool = new T_FaceList[nNbrVert];
    GW_U32 num = 0;
    for( IT_GeodesicVertexList it = VertPath.begin(); it!=VertPath.end(); ++it )
    {
        GW_GeodesicVertex* pVert2 = *it;
        if( pVert1!=NULL )
        {
            if( pFace1==NULL )
            {
                GW_ASSERT( num==1 );
                T_FaceList& FaceList = FaceListPool[0];
                /* we must initialize the faces */
                pVert1->GetFaces( *pVert2, pFace1, pFace2 );
                GW_ASSERT( pFace1!=NULL );
                GW_ASSERT( pFace2!=NULL );
                /* we must initialize the first list */
                GW_Face* pFaceTemp = pFace1;
                GW_GeodesicVertex* pDirection = pVert2;
                while( pFaceTemp!=NULL )
                {
                    FaceList.push_back(pFaceTemp);
                    GW_Face* pNewFaceTemp = pFaceTemp->GetFaceNeighbor( *pDirection );
                    pDirection = (GW_GeodesicVertex*) pFaceTemp->GetVertex(*pVert1, *pDirection);
                    GW_ASSERT( pDirection!=NULL );
                    pFaceTemp = pNewFaceTemp;
                }
            }
            T_FaceList& FaceList = FaceListPool[num];
            /* set to NULL neighbor faces */
            GW_I32 nNumNeigh = pFace1->GetEdgeNumber( *pVert1, *pVert2 );
            GW_ASSERT( nNumNeigh>=0 );
            GW_Face* pNeighFace = pFace1->GetFaceNeighbor( nNumNeigh );
            pFace1->SetFaceNeighbor( NULL, nNumNeigh );
        //    GW_ASSERT( pNeighFace!=NULL );
            if( pNeighFace!=NULL )
            {
                pVert1->SetFace(*pNeighFace);
                pVert2->SetFace(*pNeighFace);
                nNumNeigh = pNeighFace->GetEdgeNumber( *pVert1, *pVert2 );
                GW_ASSERT( nNumNeigh>=0 );
                pNeighFace->SetFaceNeighbor( NULL, nNumNeigh );
            }
            else
            {
                nNbrVert--;
                VertPath.pop_back();
                break;
            }
            /* find the half 1-ring of faces */
            GW_GeodesicVertex* pDirection = pVert1;
            IT_GeodesicVertexList it2 = it;
            it2++;
            GW_GeodesicVertex* pNext = NULL;
            if( it2!=VertPath.end() )
                pNext = *it2;
            GW_Bool bFinished = GW_False;
            do
            {
                FaceList.push_back(pFace1);
                /* test if we are on last face of the 1-ring */
                if( pNext!=NULL )
                    bFinished = GW_Vertex::ComputeUniqueId( *pFace1->GetVertex(0),*pFace1->GetVertex(1),*pFace1->GetVertex(2) )
                    == GW_GeodesicVertex::ComputeUniqueId( *pVert2, *pDirection, *pNext );
                if( !bFinished )
                {
                    /* update direction and new face */
                    GW_ASSERT( pFace1->GetEdgeNumber( *pDirection )>=0 );
                    GW_Face* pNewFace1 = pFace1->GetFaceNeighbor( *pDirection );
                    pDirection = (GW_GeodesicVertex*) pFace1->GetVertex(*pVert2, *pDirection);
                    GW_ASSERT( pDirection!=NULL );
                    pFace1 = pNewFace1;
                }
            }
            while( pFace1!=NULL && !bFinished );
        }
        pVert1 = pVert2;
        num++;
    }
    /* Create new vertices *****************************************************************/
    GW_U32 nOldNbrVertices = Mesh.GetNbrVertex();
    Mesh.SetNbrVertex( nOldNbrVertices+nNbrVert );
    num = 0;
    for( IT_GeodesicVertexList it = VertPath.begin(); it!=VertPath.end(); ++it )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) *it;
        GW_GeodesicVertex* pNewVert = (GW_GeodesicVertex*) &Mesh.CreateNewVertex();
        *pNewVert = *pVert;    // copy information
        Mesh.SetVertex(nOldNbrVertices+num, pNewVert);
        T_FaceList& FaceList = FaceListPool[num];
        for( IT_FaceList it_face = FaceList.begin(); it_face != FaceList.end(); ++it_face )
        {
            GW_Face* pFace = *it_face;
            GW_I32 nNumVert = pFace->GetEdgeNumber( *pVert );
            GW_ASSERT( nNumVert>=0 );
            pFace->SetVertex( *pNewVert, nNumVert );
        }
        num++;
    }
    GW_DELETEARRAY(FaceListPool);
    /* rebuild connectivity for the mesh *************************************************/
//    Mesh.BuildConnectivity();
    Mesh.CheckIntegrity();
}




void GW_Parameterization::BuildConformalMatrix( GW_Mesh& Mesh, GW_SparseMatrix& K, const GW_MatrixNxP* M)
{
    GW_U32 p = Mesh.GetNbrVertex();
    for( GW_U32 i=0; i<p; ++i )
    {
        GW_Vertex* pVerti = Mesh.GetVertex(i);    GW_ASSERT( pVerti!=NULL );
        K.SetRowSize(i,pVerti->GetNumberNeighbor()+1);
        GW_Float rTotalWeight = 0;
        for( GW_VertexIterator it = pVerti->BeginVertexIterator(); it!=pVerti->EndVertexIterator(); ++it )
        {
            GW_Vertex* pVertj = *it; GW_ASSERT( pVertj!=NULL );
            GW_U32 j = pVertj->GetID();
            GW_Vertex* pVertL = it.GetLeftVertex();
            GW_Vertex* pVertR = it.GetRightVertex();
            GW_Float rVal = 0;
            if( pVertL!=NULL )
            {
#if 0
                if( M!=NULL )
                {
                    GW_Float a = M->GetData(i,j);
                    /* retrieve distances */
                    GW_U32 k = pVertL->GetID();
                    GW_Float b = M->GetData(i,k);
                    GW_Float c = M->GetData(j,k);
                    GW_Float s = (a+b+c)*0.5;
                    /* compute tan(A/2)² */
                    GW_Float rTan = (s-b)*(s-c)/( s*(s-a) );
                    //                        GW_ASSERT( rTan>0 );
                    if( rTan>0 )
                        rVal += (1-rTan)/( 2*sqrt(rTan) );
                    else
                        cout << " .. One degenerate triangle ..." << endl;
                }
#else
                GW_Vector3D e1 = pVerti->GetPosition()-pVertL->GetPosition();    e1.Normalize();
                GW_Vector3D e2 = pVertj->GetPosition()-pVertL->GetPosition();    e2.Normalize();
                GW_Float dot = e1*e2;
                if( dot!=1 )
                    rVal += dot/sqrt(1-dot*dot);
#endif
            }
            if( pVertR!=NULL )
            {
#if 0
                if( M!=NULL )
                {
                    GW_Float a = M->GetData(i,j);
                    /* retrieve distances */
                    GW_U32 k = pVertR->GetID();
                    GW_Float b = M->GetData(i,k);
                    GW_Float c = M->GetData(j,k);
                    GW_Float s = (a+b+c)*0.5;
                    /* compute tan(A/2)² */
                    GW_Float rTan = (s-b)*(s-c)/( s*(s-a) );
                    //                        GW_ASSERT( rTan>0 );
                    if( rTan>0 )
                        rVal += (1-rTan)/( 2*sqrt(rTan) );
                    else
                        cout << " .. One degenerate triangle ..." << endl;
                }
#else
                GW_Vector3D e1 = pVerti->GetPosition()-pVertR->GetPosition();    e1.Normalize();
                GW_Vector3D e2 = pVertj->GetPosition()-pVertR->GetPosition();    e2.Normalize();
                GW_Float dot = e1*e2;
                if( dot!=1 )
                    rVal += dot/sqrt(1-dot*dot);
#endif
            }
            K.SetData(i,j, rVal);
            rTotalWeight += rVal;
        }
        if( rTotalWeight==0 )        // should not happen
        {
            GW_U32 nNeigSize_i = pVerti->GetNumberNeighbor();
            for( GW_VertexIterator it = pVerti->BeginVertexIterator(); it!=pVerti->EndVertexIterator(); ++it )
            {
                GW_Vertex* pVertj = *it; GW_ASSERT( pVertj!=NULL );
                GW_U32 nNeigSize_j = pVertj->GetNumberNeighbor();
                GW_U32 j = pVertj->GetID();
                rTotalWeight += 1.0/sqrt(nNeigSize_i*nNeigSize_j);
                K.SetData(i,j, 1.0/sqrt(nNeigSize_i*nNeigSize_j) );
            }
        }
        K.SetData(i,i, -rTotalWeight);
    }
}



void GW_Parameterization::BuildTutteMatrix( GW_Mesh& Mesh, GW_SparseMatrix& K )
{
    GW_U32 p = Mesh.GetNbrVertex();
    for( GW_U32 i=0; i<p; ++i )
    {
        GW_Vertex* pVerti = Mesh.GetVertex(i);
        GW_U32 nNeigSize_i = pVerti->GetNumberNeighbor();
        K.SetRowSize(i,nNeigSize_i);
        GW_Float rTotalWeight = 0;
        for( GW_VertexIterator it = pVerti->BeginVertexIterator(); it!=pVerti->EndVertexIterator(); ++it )
        {
            GW_Vertex* pVertj = (GW_Vertex*) *it; GW_ASSERT( pVertj!=NULL );
            GW_U32 nNeigSize_j = pVertj->GetNumberNeighbor();
            GW_U32 j = pVertj->GetID();
            rTotalWeight += 1.0/sqrt(nNeigSize_i*nNeigSize_j);
            K.SetData(i,j, 1.0/sqrt(nNeigSize_i*nNeigSize_j));
        }
        K.SetData(i,i, -rTotalWeight);
    }
}

void GW_Parameterization::ResolutionSpectral( GW_MatrixNxP& K, GW_MatrixNxP& L, GW_U32 EIG )
{
    GW_U32 p = L.GetNbrCols();
    GW_U32 EIG1 = EIG;
    GW_U32 EIG2 = EIG+1;
    GW_MatrixNxP U(p,p);
    GW_MatrixNxP V(p,p);
    GW_VectorND Vp(p);
    K.SVD( U, V, &Vp, NULL );
//    cout << Vp;
    GW_Float rScaleX = Vp[EIG1]; // sqrt(Vp[EIG1]);
    GW_Float rScaleY = Vp[EIG2]; // sqrt(Vp[EIG2]);
    for( GW_U32 i=0; i<p; ++i )
    {
        L.SetData(0,i, U.GetData(i,EIG1)/rScaleX );
        L.SetData(1,i, U.GetData(i,EIG2)/rScaleY );
    }
}

void GW_Parameterization::SolveSystem( GW_SparseMatrix& M, GW_VectorND& x, GW_VectorND& b )
{
#define USE_ERR_APPROX
    GW_U32 nStepIter = 1;
    // GW_Float err = M.IterativeSolve(x, b, GW_SparseMatrix::IterativeSolver_BiCG, GW_SparseMatrix::Preconditioner_SSOR,
     //            1e-10, nMaxIter, 1.2, GW_True );

    LSP_Vector xv;
    LSP_Vector bv;
    size_t Dim = M.GetDim();
    GW_Float Omega = 1.2;

    /* temportary data */
    V_Constr( &xv, "xv", Dim, Normal, True );
    GW_SparseMatrix::Copy( xv, x );
    V_Constr( &bv, "bv", Dim, Normal, True );
    GW_SparseMatrix::Copy( bv, b );

    GW_Float eps = 1e-10;
    SetRTCAccuracy( eps );
    V_SetAllCmp( &xv, 0 );    // initial guess

    /* preconditionner can be : JacobiPrecond, SSORPrecond, ILUPrecond or NULL */
    PrecondProcType pPrecond = SSORPrecond; // PrecondProcType

    GW_SparseMatrix::Copy( x, xv );
    GW_Float err = (b-M*x).Norm2();
    GW_Float err_start = err;

    GW_ProgressBar pb;

    GW_U32 nNbrIter = 0;
    GW_U32 nMaxIter = 2000; // Dim*50
    pb.Begin();
    GW_Float w_prev = 0;
    while( nNbrIter<=nMaxIter && err>eps )
    {
        GW_Float w = 1-(err-eps)/(err_start-eps);
        w = GW_MAX( w, w_prev );
        w = GW_MAX( w, ((GW_Float) nNbrIter)/((GW_Float) nMaxIter) );
        pb.Update(w);
        w_prev = w;

        BiCGIter( &M.M_, &xv, &bv, nStepIter, pPrecond, Omega );
        // test for stop
        nNbrIter += nStepIter;
        if( LASResult()!=LASOK )
        {
            GW_ASSERT(GW_False);
            cout << "Error ";
            WriteLASErrDescr(stdout);
        }
        /* r = b - A * x */
        GW_SparseMatrix::Copy( x, xv );
        err = (b-M*x).Norm2();
    }
    pb.End();
    cout << endl;


    //    std::ofstream str("log.log");
    //    str << M << b << x;
    //    str.close();

    GW_SparseMatrix::Copy( x, xv );


    /* destroy data */
    V_Destr(&xv);
    V_Destr(&bv);

}

/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::ResolutionBoundaryFree
/**
 *  \param  VoronoiMesh [GW_VoronoiMesh&] The coarse mesh to flatten.
 *  \param  K [GW_MatrixNxP&] The weight matrix for the flattening.
 *  \param   L [GW_MatrixNxP&] The position of the vertices.
 *  \param  M [GW_MatrixNxP&] The distance matrix.
 *  \author Gabriel Peyré
 *  \date   1-24-2004
 *
 *  Solve the flattening problem using boundary free formulation.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::ResolutionBoundaryFree( GW_Mesh& Mesh, GW_SparseMatrix& K, GW_MatrixNxP&  L, GW_MatrixNxP* M )
{
    GW_U32 p = Mesh.GetNbrVertex();
    GW_SparseMatrix K1(2*p);

    GW_OutputComment("Building the boundary free matrix.");
    for( GW_U32 i=0; i<p; ++i )
    {
        GW_Vertex* pVert = Mesh.GetVertex(i);    GW_ASSERT( pVert!=NULL );
        /* test if the mesh is a boundary one */
        if( !pVert->IsBoundaryVertex() )
        {
            K1.SetRowSize( i, K.GetRowSize(i) );
            K1.SetRowSize( i+p, K.GetRowSize(i) );
        }
        else
        {
            K1.SetRowSize( i, K.GetRowSize(i)+2 );
            K1.SetRowSize( i+p, K.GetRowSize(i)+2 );
        }
        /* set up X and Y line : make a copy */
        for( GW_U32 entry=0; entry<K.GetRowSize(i); ++entry )
        {
            GW_U32 j;
            GW_Float val = K.AccessEntry( i, entry, j );
            K1.SetData( i, j, val );
            K1.SetData( i+p, j+p, val );
        }
        /* special constraint for boundary */
        if( pVert->IsBoundaryVertex() )
        {
            /* we must adjust the corresponding row */
            GW_Vertex* pVert1 = NULL;
            GW_Vertex* pVert2 = NULL;
            /* special case for boundary vertices : must turn IN CLOUNTERCLOCKWISE sense */
            GW_Face* pFace = pVert->GetFace();    GW_ASSERT( pFace!=NULL );
            // first find FIRST boundary vertex
            GW_Face* pStartFace = pFace;
            GW_Vertex* pDirVertex = pFace->GetNextVertex(*pVert);    GW_ASSERT( pDirVertex!=NULL );
            pDirVertex = pFace->GetNextVertex(*pDirVertex);            GW_ASSERT( pDirVertex!=NULL );
            GW_U32 num = 0;
            while( pFace->GetFaceNeighbor(*pDirVertex)!=NULL && num<100 )
            {
                num++;
                GW_Face* pPrevFace = pFace;
                pFace = pFace->GetFaceNeighbor(*pDirVertex);
                pDirVertex = pPrevFace->GetVertex(*pVert,*pDirVertex);    GW_ASSERT(pDirVertex!=NULL);
            }
            GW_ASSERT( num<100 );
            //    GW_ASSERT( pFace!=pStartFace || pStartFace->GetFaceNeighbor(*pDirVertex)!=NULL );
            pVert1 = pFace->GetVertex(*pVert,*pDirVertex);    GW_ASSERT(pVert1!=NULL);
            // then find LAST vertex
            pDirVertex = pVert1;
            num = 0;
            while( pFace->GetFaceNeighbor(*pDirVertex)!=NULL && num<100 )
            {
                num++;
                GW_Face* pPrevFace = pFace;
                pFace = pFace->GetFaceNeighbor(*pDirVertex);
                pDirVertex = pPrevFace->GetVertex(*pVert,*pDirVertex);    GW_ASSERT(pDirVertex!=NULL);
            }
            GW_ASSERT( num<100 );
            //    GW_ASSERT( pFace!=pStartFace || pStartFace->GetFaceNeighbor(*pDirVertex)!=NULL );
            pVert2 = pFace->GetVertex(*pVert,*pDirVertex);    GW_ASSERT(pVert2!=NULL);
            GW_ASSERT( pVert1!=NULL && pVert2!=NULL );
            GW_Float epsilon = 1;
            /* X coords **********************************/
            K1.SetData( i,pVert1->GetID()+p, +epsilon );        // add a constraint on -Y coordinate
            K1.SetData( i,pVert2->GetID()+p, -epsilon );
            /* Y coords **********************************/
            K1.SetData( i+p,pVert1->GetID(), -epsilon );        // add a constraint on X coordinate
            K1.SetData( i+p,pVert2->GetID(), +epsilon );
        }
    }


    GW_VectorND x(2*p, GW_Float(0));    // solution
    GW_VectorND b(2*p, GW_Float(0));    // rhs


    /* add two more constraints : choose them as far as possible from one another */
    GW_Vertex* pVert[2];
    if( M!=NULL )    // use farthest vertices
    {
        GW_U32 i_max = 0, j_max = 0;
        GW_Float dist_max = 0;
        for( GW_U32 i=0; i<p; ++i )
        for( GW_U32 j=0; j<i; ++j )
        {
            if( M->GetData(i,j)>dist_max )
            {
                dist_max = M->GetData(i,j);
                i_max = i; j_max = j;
            }
        }
        pVert[0] = Mesh.GetVertex(i_max);
        pVert[1] = Mesh.GetVertex(j_max);
    }
    else
    {
        // use random
        pVert[0] = Mesh.GetRandomVertex();
        pVert[1] = Mesh.GetRandomVertex();
    }

    GW_Vector2D arbitrary_pos[2] = { GW_Vector2D(-2.5,0), GW_Vector2D(2.5,0) };
    for( GW_U32 k=0; k<2; ++k )
    {
        GW_U32 i = pVert[k]->GetID();
        /* clear this row */
        K1.SetRowSize(i,0);
        K1.SetRowSize(i,1);
        K1.SetRowSize(i+p,0);
        K1.SetRowSize(i+p,1);
        /* Add a position constraint */
        K1.SetData( i,   i,     1 );        // X constraint
        K1.SetData( i+p, i+p, 1 );        // Y constraint
        b[i]    = arbitrary_pos[k][0];
        b[i+p]    = arbitrary_pos[k][1];
    }

    cout << "  * System resolution.";
    // K1.LUSolve( x, b );    // for small system
    GW_Parameterization::SolveSystem( K1, x, b );


    for( GW_U32 j=0; j<p; ++j )
    {
        L.SetData(0,j, x.GetData(j) );
        L.SetData(1,j, x.GetData(j+p) );
    }
}




/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::ResolutionBoundaryFixed
/**
*  \param  VoronoiMesh [GW_VoronoiMesh&] The coarse mesh to flatten.
*  \param  K [GW_MatrixNxP&] The weight matrix for the flattening.
*  \param   L [GW_MatrixNxP&] The position of the vertices.
*  \param  M [GW_MatrixNxP&] The distance matrix.
*  \author Gabriel Peyré
*  \date   1-24-2004
*
*  Solve the flattening problem using boundary fixed formulation.
*/
/*------------------------------------------------------------------------------*/
void GW_Parameterization::ResolutionBoundaryFixed( GW_Mesh& Mesh, GW_SparseMatrix& K, GW_MatrixNxP&  L,
                                T_Vector2DMap* pInitialPos, T_TrissectorInfoMap* pTrissectorInfoMap,
                                T_TrissectorInfoVector* pCyclicPosition )
{
    /* extract boundary *******************************************************/
    std::list<T_VertexList> boundary_list;
    Mesh.ExtractAllBoundaries( boundary_list );
    if( boundary_list.size()==0 )
    {
        GW_OutputComment("Closed mesh, can't flatten this mesh.");
        GW_ASSERT( GW_False );
        return;
    }
    if( boundary_list.size()>1 )
        GW_OutputComment("Warning, this mesh has more than 1 boundary.");
    T_VertexList boundary = boundary_list.front();
    /* find vertex position **************************************************/
    T_Vector2DMap Positions;    // position of boundary points
    if( pInitialPos!=NULL )
    {
        Positions = *pInitialPos;
    }
    else if( pTrissectorInfoMap==NULL )
    {
        GW_Float perimeter = Mesh.GetPerimeter( boundary );
        /* flatten into a disk */
        GW_Float curv_abs = 0;    // curvilinear absice
        GW_Vertex* pPrev = NULL;
        for( IT_VertexList it = boundary.begin(); it!=boundary.end(); ++it )
        {
            GW_Vertex* pVert = *it;    GW_ASSERT( pVert!=NULL );
            if( pPrev!=NULL )
                curv_abs += (pVert->GetPosition()-pPrev->GetPosition()).Norm();
            GW_Float theta = GW_TWOPI*curv_abs/perimeter;
            Positions[ pVert->GetID() ] = GW_Vector2D( cos(theta), sin(theta) );
            pPrev = pVert;
        }
    }
    else
    {
        GW_ASSERT( pCyclicPosition!=NULL );
        /* flatten using a convex polyhedron, first find stating vertex */
        IT_VertexList it_start = boundary.end();
        for( IT_VertexList it = boundary.begin(); it!=boundary.end(); ++it )
        {
            GW_Vertex* pVert = *it; GW_ASSERT( pVert!=NULL );
            if( pTrissectorInfoMap->find(pVert->GetID())!=pTrissectorInfoMap->end() )
            {
                it_start = it;
                break;
            }
        }
        GW_ASSERT( it_start != boundary.end() );
        if( it_start==boundary.end() )
            return;
        /* now extract each sub-boundary */
        std::list<T_VertexList> boundary_pieces;
        IT_VertexList it_cur = it_start;
        GW_Float rPerimeter = 0;
        T_FloatVector Perimeters;
        Perimeters.resize( pTrissectorInfoMap->size() );
        GW_U32 num = 0;
        while( GW_True )
        {
            T_VertexList new_piece;
            /* feed the piece */
            new_piece.push_back( *it_cur );
            GW_Vertex* pVert = NULL;
            do
            {
                it_cur++;
                if( it_cur==boundary.end() )    // treat the boundary as cyclic
                    it_cur = boundary.begin();
                new_piece.push_back( *it_cur );
                pVert = *it_cur;    GW_ASSERT(pVert!=NULL);
            }
            while( pTrissectorInfoMap->find(pVert->GetID())==pTrissectorInfoMap->end() );
            /* add this to the list */
            boundary_pieces.push_back( new_piece );
            /* compute perimeter */
            GW_Float perim = GW_Mesh::GetPerimeter( new_piece, GW_False );
            Perimeters[num] = perim;
            rPerimeter += perim;
            /* check for stop */
            if( it_cur == it_start )
                break;
            num++;
        };
        /* normally we should have extracted as many piece as they were set position */
        GW_U32 nNbrPieces = (GW_U32) boundary_pieces.size();
        GW_ASSERT( nNbrPieces==pTrissectorInfoMap->size() );
        /* set up the position for each base point */
        pCyclicPosition->resize( nNbrPieces );
        /* linearly interpolate the position */
        num = 0;
        GW_Float theta = 0;
        for( std::list<T_VertexList>::iterator it_piece = boundary_pieces.begin(); it_piece!=boundary_pieces.end(); ++it_piece )
        {
            T_VertexList& piece = *it_piece;
            /* first compute the position of start & end */
            GW_Vector2D pos1 = GW_Vector2D( cos(theta), sin(theta) );
            theta += GW_TWOPI * Perimeters[num]/rPerimeter;
            if( num==nNbrPieces-1 )
                theta = 0;
            GW_Vector2D pos2 = GW_Vector2D( cos(theta), sin(theta) );
            /* set up the cyclic ordering of vertices */
            GW_ASSERT( pTrissectorInfoMap->find(piece.front()->GetID())!=pTrissectorInfoMap->end()  );
            GW_TrissectorInfo& tr = (*pTrissectorInfoMap)[piece.front()->GetID()];
            tr.SetPosition( pos1 );
            (*pCyclicPosition)[num] = tr;
            /* compute the position of each point */
            GW_Float rLength = Perimeters[num];
            GW_Float curv_abs = 0;    // curvilinear absice
            GW_Vertex* pPrev = NULL;
            for( IT_VertexList it = piece.begin(); it!=piece.end(); ++it )
            {
                GW_Vertex* pVert = *it;    GW_ASSERT( pVert!=NULL );
                if( pPrev!=NULL )
                    curv_abs += (pVert->GetPosition()-pPrev->GetPosition()).Norm();
                GW_Float lambda = curv_abs/rLength;
                GW_Vector2D pos = pos1*(1-lambda) + pos2*lambda;
                Positions[ pVert->GetID() ] = pos;
                pPrev = pVert;
            }
            num++;
        }

    }
    /* set up the new matrix ****************************************************/
    GW_U32 p = Mesh.GetNbrVertex();
    for( IT_Vector2DMap it=Positions.begin(); it!=Positions.end(); ++it )
    {
        GW_U32 i = it->first;
        K.SetRowSize(i,0);
        K.SetRowSize(i,1);
        K.SetData(i,i,1);
    }

    /* solve the system *********************************************************/
    GW_VectorND x(p, GW_Float(0));    // solution
    for( GW_U32 coord = 0; coord<2; ++coord )
    {
        /* build RHS */
        GW_VectorND b(p, GW_Float(0));    // rhs
        for( IT_Vector2DMap it=Positions.begin(); it!=Positions.end(); ++it )
        {
            GW_U32 i = it->first;
            GW_Vector2D    pos = it->second;
            b.SetData(i, pos[coord]);
        }

        /* solve system */
        cout << "  * System resolution.";
        GW_Parameterization::SolveSystem( K, x, b );

        for( GW_U32 j=0; j<p; ++j )
            L.SetData(coord,j, x.GetData(j) );
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::ParameterizeMesh
/**
*  \param  Mesh [GW_GeodesicMesh&] The mesh to flatten.
*  \param  BaseVertex [T_GeodesicVertexList&] The base vertex that are really flatten.
*  \author Gabriel Peyré
*  \date   1-13-2004
*
*  Parameterize a whole mesh using conformal-like flattening.
*/
/*------------------------------------------------------------------------------*/
void GW_Parameterization::ParameterizeMesh( GW_GeodesicMesh& Mesh, GW_VoronoiMesh& VoronoiMesh, GW_GeodesicMesh& FlattenedMesh,
                                            T_ParameterizationType ParamType, T_ResolutionType ResolType,
                                            T_GeodesicVertexMap* pBoundaryVertPos )
{
    T_GeodesicVertexList& BaseVertex = VoronoiMesh.GetBaseVertexList();
    GW_U32 n = Mesh.GetNbrVertex();
    GW_U32 p = (GW_U32) BaseVertex.size();
    GW_MatrixNxP DistMatrix(p,n,-1);
    T_U32Map BaseVertNum;    // vertex number of each base vertex

    /* Gather distance information between base vertex and the rest **********************************************/
    GW_U32 i = 0;
    cout << "  * Computing distance information ";
    GW_ProgressBar pb;
    pb.Begin();
    for( IT_GeodesicVertexList it = BaseVertex.begin(); it!=BaseVertex.end(); ++it )
    {
        GW_GeodesicVertex* pVert = *it; GW_ASSERT( pVert!=NULL );
        BaseVertNum[i] = pVert->GetID();
        Mesh.ResetGeodesicMesh();
        Mesh.PerformFastMarching( pVert );
        for( GW_U32 j=0; j<n; ++j )
        {
            GW_GeodesicVertex* pVert2 = (GW_GeodesicVertex*) Mesh.GetVertex(j); GW_ASSERT( pVert2!=NULL );
            DistMatrix.SetData(i,j, pVert2->GetDistance()*pVert2->GetDistance() );
        }
        i++;
        pb.Update(((GW_Float)i)/((GW_Float)BaseVertex.size()));
    }
    pb.End();
    cout << endl;
    /* flatten the base points *************************************************************************************/
    /* build the distance matrix */
    GW_MatrixNxP M(p,p,0.0);
    for( GW_U32 i=0; i<p; ++i )
    for( GW_U32 j=0; j<i; ++j )
    {
        GW_U32 ii = BaseVertNum[i];
        GW_U32 jj = BaseVertNum[j];
        GW_Float dist = 0.5*( DistMatrix.GetData(i,jj) + DistMatrix.GetData(j,ii) );
        M.SetData( i,j, dist );
        M.SetData( j,i, dist );
    }

    GW_U32 EIG = 0;
    GW_MatrixNxP K_full(p,p,0.0);    // a symmetric matrix whose eigenvalues are the embedding
    GW_SparseMatrix    K_sparse(p);
    GW_Bool bUseSparse = GW_False;

    switch(ParamType)
    {
    case kGeodesicIsomap:
        {
            GW_MatrixNxP J(p,p,-1.0/p);            // centering matrix
            for( GW_U32 i=0; i<p; ++i )
                J[i][i] += 1;
            K_full = (J*M*J)*(-0.5);
            EIG = 0;    // first eigenvalue
        }
        break;
    case kTutte:
        {
            /* build the weight matrix */
            BuildTutteMatrix(VoronoiMesh, K_sparse);
            EIG = p-2;    // first eigenvalue
            bUseSparse = GW_True;
        }
        break;
    case kGeodesicConformal:
        {
            /* build the weight matrix */
            BuildConformalMatrix(VoronoiMesh, K_sparse, &M);
            EIG = p-2;    //    first eigenvalue
            bUseSparse = GW_True;
        }
        break;
    default:
        GW_ASSERT( GW_False );
        return;
    }

    /* contains position of points */
    GW_MatrixNxP L(2,p);

    switch(ResolType) {
    case kSpectral:
    //    if( bUseSparse )
    //        K_full.BuildFromSparse(K_sparse);
        ResolutionSpectral( K_full, L, EIG );
        break;
    case kBoundaryFree:
        if( !bUseSparse )
            K_sparse.BuildFromFull(K_full);
        ResolutionBoundaryFree( VoronoiMesh, K_sparse, L, &M );
        break;
    case kBoundaryFixed:
        if( !bUseSparse )
            K_sparse.BuildFromFull(K_full);
        ResolutionBoundaryFixed( VoronoiMesh, K_sparse,L );
    default:
        break;
    };

    /* interpolate position ****************************************************************************************/
    GW_ASSERT( Mesh.GetNbrVertex()==FlattenedMesh.GetNbrVertex() );
    GW_VectorND DeltaMean(p);
    /* construct mean distance to landmark */
    for( GW_U32 i=0; i<p; ++i )
    {
        GW_Float dist = 0;
        for( GW_U32 j=0; j<n; ++j )
            dist += DistMatrix.GetData(i,j);
        dist = dist/n;
        DeltaMean.SetData( i, dist );
    }
    /* find position for each point */
    for( GW_U32 j=0; j<n; ++j )
    {
        /* construct vector of distance to landmark */
        GW_VectorND Delta(p);
        for( GW_U32 i=0; i<p; ++i )
            Delta.SetData(i,DistMatrix.GetData(i,j) );
        /* assign position */
        GW_VectorND Pos = 0.5 * (L*(DeltaMean-Delta));
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) FlattenedMesh.GetVertex(j);    GW_ASSERT( pVert!=NULL );
        pVert->SetPosition( GW_Vector3D(Pos[0], Pos[1], 0) );
    }
    // rescale the mesh.
    FlattenedMesh.ScaleVertex( 5/FlattenedMesh.GetBoundingRadius() );
}








/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::ParameterizeAllRegions
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh. Must have been cut in separated component.
 *  \param  VertList [T_GeodesicVertexList&] Seed points.
 *  \author Gabriel Peyré
 *  \date   7-3-2003
 *
 *  Parameterize each region of the mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::ParameterizeAllRegions( T_GeodesicVertexList& VertList )
{
    /* clear previous mesh list */
    for( IT_MeshVector itMesh=MeshVector_.begin(); itMesh!=MeshVector_.end(); ++itMesh )
    {
        GW_GeodesicMesh* pMesh = (GW_GeodesicMesh*) *itMesh;
        GW_DELETE( pMesh );
    }
    MeshVector_.clear();
    MeshVector_.resize( VertList.size() );
    IT_GeodesicVertexList it = VertList.begin();
    for( GW_U32 i=0; i<VertList.size(); ++i )
    {
        GW_GeodesicMesh* pMesh = new GW_GeodesicMesh;
        MeshVector_[i] = pMesh;
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) *it;        GW_ASSERT( pVert!=NULL );
        this->ParameterizeRegion( *pVert, *pMesh );
        it++;
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::ParameterizeRegion
/**
 *  \param  Seed [GW_GeodesicVertex&] Seed of the region.
 *  \param  BaseDomain [GW_GeodesicMesh&] The domain of the parameterization.
 *  \author Gabriel Peyré
 *  \date   7-3-2003
 *
 *  Parameterize a given part of the mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::ParameterizeRegion( GW_GeodesicVertex& Seed, GW_GeodesicMesh& BaseDomain )
{

}
















/* assign to each vertex a pointer to the center of its Voronoi cell */
GW_GeodesicVertex** VertexVoronoiCells;
GW_GeodesicVertex* pMaxDistVert;
GW_Float rMaxDist;
/** We add a new point only if it belongs to the current Voronoi diagram */
GW_Bool GW_Parameterization::PerformPseudoLloydIteration_Insertion( GW_GeodesicVertex& CurVert, GW_Float rNewDist )
{
    GW_ASSERT( pCurVoronoiDiagram_!=NULL );
    GW_ASSERT( VertexVoronoiCells!=NULL );
    GW_GeodesicVertex* pVertVoronoiDiagram = VertexVoronoiCells[CurVert.GetID()];
    return pVertVoronoiDiagram == pCurVoronoiDiagram_;
}
void GW_Parameterization::PerformPseudoLloydIteration_NeswDead( GW_GeodesicVertex& Vert )
{
    if( Vert.GetDistance()>=rMaxDist )
    {
        rMaxDist = Vert.GetDistance();
        pMaxDistVert = &Vert;
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::PerformPseudoLloydIteration
/**
 *  \param  Mesh [GW_GeodesicMesh&] The mesh.
 *  \param  VertList [T_GeodesicVertexList&] List of center of the Voronoi cells.
 *  \author Gabriel Peyré
 *  \date   1-14-2004
 *
 *  Perform a centering step for each Voronoi region.
 */
/*------------------------------------------------------------------------------*/
void GW_Parameterization::PerformPseudoLloydIteration( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList )
{
    VertexVoronoiCells = new GW_GeodesicVertex*[Mesh.GetNbrVertex()];
    /* recompute the whole distance map */
    GW_OutputComment("Recomputing the whole original Voronoi diagram.");
    this->PerformFastMarching( Mesh, VertList );

    /* duplicate the Marching information */
    for( GW_U32 i=0; i<Mesh.GetNbrVertex(); ++i )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Mesh.GetVertex(i); GW_ASSERT( pVert!=NULL );
        VertexVoronoiCells[i] = pVert->GetFront();
    }

    /* compute the border of each cell */
    std::map<GW_U32,T_GeodesicVertexList*> CellBoundaryMap;
    for( GW_U32 i=0; i<Mesh.GetNbrFace(); ++i )
    {
        GW_Face* pFace = Mesh.GetFace(i); GW_ASSERT( pFace!=NULL );
        for( GW_U32 j1=0; j1<3; ++j1 )
        {
            GW_U32 j2 = (j1+1)%3;
            GW_GeodesicVertex* pVert1 = (GW_GeodesicVertex*) pFace->GetVertex(j1); GW_ASSERT( pVert1!=NULL );
            GW_GeodesicVertex* pVert2 = (GW_GeodesicVertex*) pFace->GetVertex(j2); GW_ASSERT( pVert2!=NULL );
            if( pVert1->GetFront()!=pVert2->GetFront() && pVert1->GetFront()!=NULL )
            {
                GW_U32 nID = pVert1->GetFront()->GetID();
                if( CellBoundaryMap.find(nID)==CellBoundaryMap.end() )
                    CellBoundaryMap[nID] = new T_GeodesicVertexList;    // we should create a new map
                T_GeodesicVertexList* pCurMap = CellBoundaryMap[nID];
                pCurMap->push_back(pVert1);
            }
        }
    }

    /* perform the centering for each cell */
    T_GeodesicVertexList NewVertList;
    Mesh.RegisterWeightCallbackFunction( GW_GeodesicMesh::BasicWeightCallback );    // we don't want to use some special energy
    Mesh.RegisterVertexInsersionCallbackFunction( PerformPseudoLloydIteration_Insertion );
    Mesh.RegisterNewDeadVertexCallbackFunction( PerformPseudoLloydIteration_NeswDead );
    GW_U32 num = 0;
    cout << "  * Centering vertices ";
    GW_ProgressBar pb;
    pb.Begin();
    for( IT_GeodesicVertexList it = VertList.begin(); it!=VertList.end(); ++it )
    {
        GW_GeodesicVertex* pCenterVert = *it; GW_ASSERT( pCenterVert!=NULL );
        GW_U32 nID = pCenterVert->GetID();
        /* retrieve the list of boundary vertices */
        GW_ASSERT( CellBoundaryMap.find(nID)!=CellBoundaryMap.end() );
        if( CellBoundaryMap.find(nID)!=CellBoundaryMap.end() )        // in case this is a closed region ...
        {
            T_GeodesicVertexList* pBoundaryVerts = CellBoundaryMap[nID];
            /* perform a local propagation, by restricting it to the cell */
            pCurVoronoiDiagram_ = pCenterVert;
            pMaxDistVert = NULL;
            rMaxDist = 0;
            GW_Parameterization::PerformFastMarching( Mesh, *pBoundaryVerts );
            /* perhaps check here if we can find better */
            GW_ASSERT( pMaxDistVert!=NULL );
            NewVertList.push_back( pMaxDistVert );
            num++;
            pb.Update( ((GW_Float)num)/((GW_Float)VertList.size()) );
        }
        else
            NewVertList.push_back( pCenterVert );
    }
    pb.End();
    cout << endl;
    pCurVoronoiDiagram_ = NULL;
    Mesh.RegisterVertexInsersionCallbackFunction( NULL );
    Mesh.RegisterNewDeadVertexCallbackFunction( NULL );


    /* replace the old list by the new one */
    VertList = NewVertList;

    GW_DELETEARRAY(VertexVoronoiCells);
    /* clear the maps */
    for( std::map<GW_U32,T_GeodesicVertexList*>::iterator it=CellBoundaryMap.begin(); it!=CellBoundaryMap.end(); ++it  )
        GW_DELETE(it->second);
}



///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
