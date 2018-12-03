#include "stdafx.h"
#include "GW_Mesh.h"
#include "GW_VertexIterator.h"

#ifndef GW_USE_INLINE
    #include "GW_Mesh.inl"
#endif

using namespace GW;

GW_Mesh* GW_Mesh::pStaticThis_ = NULL;


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::operator=
/**
 *  \param  v [GW_Mesh&] Mesh.
 *  \return [GW_Mesh&] *this
 *
 *  Copy operator.
 */
/*------------------------------------------------------------------------------*/
GW_Mesh& GW_Mesh::operator=(const GW_Mesh& Mesh)
{
    this->SetNbrVertex( Mesh.GetNbrVertex() );
    this->SetNbrFace( Mesh.GetNbrFace() );

    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        if( this->GetVertex(i)==NULL )
        {
            GW_Vertex& NewVert = this->CreateNewVertex();
            this->SetVertex(i, &NewVert);
        }
        GW_Vertex& NewVert = *this->GetVertex(i);
        const GW_Vertex& OriginalVert = *Mesh.GetVertex(i);
        NewVert = OriginalVert;
        /* resolve face attachment */
        const GW_Face* pFace = OriginalVert.GetFace();
        if( pFace!=NULL )
            NewVert.SetFace( *this->GetFace( pFace->GetID() ) );
    }
    for( GW_U32 i=0; i<this->GetNbrFace(); ++i )
    {
        if( this->GetFace(i)==NULL )
        {
            GW_Face& NewFace = this->CreateNewFace();
            this->SetFace(i, &NewFace);
        }
        GW_Face& NewFace = *this->GetFace(i);
        const GW_Face& OriginalFace = *Mesh.GetFace(i);
        NewFace = OriginalFace;
        /* resolve Vertex and neighbor */
        for( GW_U32 j=0; j<3; ++j )
        {
            GW_U32 VertID = OriginalFace.GetVertex(j)->GetID();
            NewFace.SetVertex( *this->GetVertex(VertID), j );
            const GW_Face* pNeigh = OriginalFace.GetFaceNeighbor(j);
            if( pNeigh==NULL )
                NewFace.SetFaceNeighbor(NULL, j);
            else
                NewFace.SetFaceNeighbor( this->GetFace( pNeigh->GetID() ), j);
        }
    }

    return *this;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::SetNbrFace
/**
 *  \param  nNum [GW_U32] New number of faces.
 *  Resize the mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::SetNbrFace( GW_U32 nNum )
{
    GW_U32 nOldSize = this->GetNbrFace();
    if( nNum<nOldSize )
    {
        /* check if the vertex at the end should be deleted */
        for( GW_U32 i=nNum; i<nOldSize; ++i )
            GW_SmartCounter::CheckAndDelete( this->GetFace( i ) );
        FaceVector_.resize( nNum );
    }
    if( nNum>nOldSize )
    {
        FaceVector_.resize( nNum );
        /* set to NULL newly appended pointers */
        for( GW_U32 i=nOldSize; i<nNum; ++i )
            this->SetFace( i, NULL );
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::SetNbrVertex
/**
 *  \param  nNum [GW_U32] New number of vertex.
 *
 *  Resize the mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::SetNbrVertex( GW_U32 nNum )
{
    GW_U32 nOldSize = this->GetNbrVertex();
    if( nNum<nOldSize )
    {
        /* check if the vertex at the end should be deleted */
        for( GW_U32 i=nNum; i<nOldSize; ++i )
            GW_SmartCounter::CheckAndDelete( this->GetVertex( i ) );
        //!!VertexVector_.resize( nNum );
        VertexVector_size=nNum;
    }
    if( nNum>nOldSize )
    {
        //!!VertexVector_.resize( nNum );
        GW_Vertex** nv = new GW_Vertex*[VertexVector_size=nNum];            //make a larger vector for vertices
        for(GW_U32 i=0;i<nOldSize;++i) nv[i] = VertexVector_[i];                //copy the old vertices (with whatever use counter they have)
        //if(nOldSize>0)
        delete[] VertexVector_;
        VertexVector_ = nv;

        //for( GW_U32 i=0; i<nNum; ++i )
        //    VertexVector_[i] = new GW_Vertex(); //LUK:

        /* set to NULL newly appended pointers */
        for( GW_U32 i=nOldSize; i<nNum; ++i )
            VertexVector_[i] = NULL;
            //this->SetVertex( i, NULL );
    }
}




/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetBoundingRadius
/**
 *  \return [GW_Float] Radius.
 *
 *  Get the maximum distance from (0,0,0)
 */
/*------------------------------------------------------------------------------*/
GW_Float GW_Mesh::GetBoundingRadius()
{
    GW_Float rRadius = 0;
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        if( this->GetVertex(i)!=NULL )
            rRadius = GW_MAX(this->GetVertex(i)->GetPosition().SquareNorm(), rRadius);
    }

    return (GW_Float) sqrt(rRadius);
}


void GW_Mesh::GetBoundingBox( GW_Vector3D& min, GW_Vector3D& max )
{
    min.SetValue( GW_INFINITE );
    max.SetValue( -GW_INFINITE );
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        if( this->GetVertex(i)!=NULL )
        {
            GW_Vector3D& pos = this->GetVertex(i)->GetPosition();
            min[0] = GW_MIN( min[0], pos[0] );
            min[1] = GW_MIN( min[1], pos[1] );
            min[2] = GW_MIN( min[2], pos[2] );
            max[0] = GW_MAX( max[0], pos[0] );
            max[1] = GW_MAX( max[1], pos[1] );
            max[2] = GW_MAX( max[2], pos[2] );
        }
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetBarycenter
/**
*  \return [GW_Vector3D] The iso-barycenter of the mesh.
*
*  Get the maximum distance from (0,0,0).
*/
/*------------------------------------------------------------------------------*/
GW_Vector3D GW_Mesh::GetBarycenter()
{
    GW_Vector3D Barycenter;
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        if( this->GetVertex(i)!=NULL )
            Barycenter += this->GetVertex(i)->GetPosition();
    }

    if( this->GetNbrVertex()>0 )
        Barycenter /= (GW_Float) this->GetNbrVertex();
    return Barycenter;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::ScaleVertex
/**
 *  \param  rScale [GW_Float] Scaling factor
 *
 *  Scale each vertex.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::ScaleVertex( GW_Float rScale )
{
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        if( this->GetVertex(i)!=NULL )
            this->GetVertex(i)->GetPosition() *= rScale;
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::TranslateVertex
/**
*  \param  Vect [GW_Float] Translation vector.
*
*  Scale each vertex.
*/
/*------------------------------------------------------------------------------*/
void GW_Mesh::TranslateVertex( const GW_Vector3D& Vect )
{
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        if( this->GetVertex(i)!=NULL )
            this->GetVertex(i)->GetPosition() += Vect;
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::BuildNormal
/**
 *  Compute vertex normals form faces.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::BuildRawNormal()
{
    //!!for( IT_VertexVector it=VertexVector_.begin(); it!=VertexVector_.end(); ++it )
    for(GW_Vertex** it = VertexVector_; it-VertexVector_<VertexVector_size; ++it)
    {
        GW_Vertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        pVert->BuildRawNormal();
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::BuildCurvatureData
/**
 *
 *  Compute all curvature data using comlex schemes. This includes
 *  normal, curvatures and curvature directions.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::BuildCurvatureData()
{
    //!!for( IT_VertexVector it=VertexVector_.begin(); it!=VertexVector_.end(); ++it )
    for(GW_Vertex** it = VertexVector_; it-VertexVector_<VertexVector_size; ++it)
    {
        GW_Vertex* pVert = *it;
        GW_ASSERT( pVert!=NULL );
        pVert->BuildCurvatureData();
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::BuildConnectivity
/**
*  Call this method when you have set the vertex and the face.
*    This will set up the neighboorhood for each face.
*/
/*------------------------------------------------------------------------------*/
void GW_Mesh::BuildConnectivity()
{
    T_FaceList* VertexToFaceMap = new T_FaceList[this->GetNbrVertex()];

    // build the inverse map vertex->face
    for(IT_FaceVector it = FaceVector_.begin(); it!=FaceVector_.end(); ++it )
    {
        GW_Face* pFace = *it;
        GW_ASSERT( pFace!=NULL );
        for( GW_U32 i=0; i<3; ++i )
        {
            GW_Vertex* pVert = pFace->GetVertex(i);
            GW_ASSERT(pVert!=NULL);
            GW_ASSERT( pVert->GetID() <= this->GetNbrVertex() );
            VertexToFaceMap[pVert->GetID()].push_back( pFace );
        }
    }

    // now we can set up connectivity
    for( IT_FaceVector it=FaceVector_.begin(); it!=FaceVector_.end(); ++it )
    {
        GW_Face* pFace = *it;
        GW_ASSERT( pFace!=NULL );

        /* set up the neigbooring faces of the 3 vertices */
        T_FaceList* pFaceLists[3];
        for( GW_U32 i=0; i<3; ++i )
        {
            GW_Vertex* pVert = pFace->GetVertex(i);
            pFaceLists[i] = &VertexToFaceMap[pVert->GetID()];
        }

        /* compute neighbor in the 3 directions */
        for( GW_U32 i=0; i<3; ++i )
        {
            GW_Face* pNeighbor = NULL;
            GW_U32 i1 = (i+1)%3;
            GW_U32 i2 = (i+2)%3;
            /* we must find the intersection of the surrounding faces of these 2 vertex */
            GW_Bool bFind = GW_False;
            for( IT_FaceList it1 = pFaceLists[i1]->begin(); it1!=pFaceLists[i1]->end() && bFind!=GW_True; ++it1 )
            {
                GW_Face* pFace1 = *it1;
                for( IT_FaceList it2 = pFaceLists[i2]->begin(); it2!=pFaceLists[i2]->end() && bFind!=GW_True; ++it2 )
                {
                    GW_Face* pFace2 = *it2;
                    if( pFace1==pFace2 && pFace1!=pFace )
                    {
                        pNeighbor = pFace1;
                        bFind=GW_True;
                    }
                }
            }
            pFace->SetFaceNeighbor( pNeighbor, i );
            /* make some test on the neighbor to assure symmetry in the connectivity relationship */
            if( pNeighbor!=NULL )
            {
                GW_I32 nEdgeNumber = pNeighbor->GetEdgeNumber( *pFace->GetVertex(i1),*pFace->GetVertex(i2) );
                GW_ASSERT( nEdgeNumber>=0 );
                pNeighbor->SetFaceNeighbor( pFace, nEdgeNumber );
            }
        }
    }

    GW_DELETEARRAY( VertexToFaceMap );
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetArea
/**
 *  \return [GW_Float] Total area.
 *
 *  Compute the total area of the triangulated mesh.
 */
/*------------------------------------------------------------------------------*/
GW_Float GW_Mesh::GetArea()
{
    GW_Float rArea = 0;

    for( IT_FaceVector it=FaceVector_.begin(); it!=FaceVector_.end(); ++it)
    {
        GW_Face* pFace = *it;
        GW_ASSERT( pFace!=NULL );
        GW_Vertex* v0 = pFace->GetVertex(0);
        GW_Vertex* v1 = pFace->GetVertex(1);
        GW_Vertex* v2 = pFace->GetVertex(2);
        if( v0!=NULL && v1!=NULL && v2!=NULL )
        {
            GW_Vector3D e1 = v1->GetPosition() - v0->GetPosition();
            GW_Vector3D e2 = v2->GetPosition() - v0->GetPosition();
            rArea += ~(e1 ^ e2);
        }
    }

    return (GW_Float) 0.5*rArea;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetRandomVertex
/**
 *  \return [GW_Vertex*] The vertex. NULL if it was impossible.
 *
 *  Get Return a vertex at random.
 */
/*------------------------------------------------------------------------------*/
GW_Vertex* GW_Mesh::GetRandomVertex()
{
    GW_U32 nNumber = 0;
    GW_Vertex* pStartVertex = NULL;
    while( pStartVertex==NULL )
    {
        if( nNumber>=this->GetNbrVertex()/10 )
            return NULL;
        GW_U32 nNumVert = (GW_U32) floor(GW_RAND*this->GetNbrVertex());
        pStartVertex = this->GetVertex( nNumVert );
        if( pStartVertex->GetFace()==NULL )
            pStartVertex = NULL;
        nNumber++;
    }
    return pStartVertex;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::InsertVertexInFace
/**
 *  \param  Face [GW_Face&] The face.
 *  \param  x [GW_Float] x barycentric coordinate.
 *  \param  y [GW_Float] y barycentric coordinate.
 *  \param  z [GW_Float] z barycentric coordinate.
 *    \return The newly created point.
 *
 *  Insert a vertex in a face.
 */
/*------------------------------------------------------------------------------*/
GW_Vertex* GW_Mesh::InsertVertexInFace( GW_Face& Face, GW_Float x, GW_Float y, GW_Float z )
{
    GW_Vertex* pVert0 = Face.GetVertex(0);
    GW_Vertex* pVert1 = Face.GetVertex(1);
    GW_Vertex* pVert2 = Face.GetVertex(2);
    GW_ASSERT( pVert0!=NULL );
    GW_ASSERT( pVert1!=NULL );
    GW_ASSERT( pVert2!=NULL );
    /* create two new-faces */
    GW_Face* pFace1 = &this->CreateNewFace();
    GW_Face* pFace2 = &this->CreateNewFace();
    this->SetNbrFace( this->GetNbrFace()+2 );
    this->SetFace( this->GetNbrFace()-2, pFace1 );
    this->SetFace( this->GetNbrFace()-1, pFace2 );
    /* create one new vertex */
    GW_Vertex* pNewVert = &this->CreateNewVertex();
    GW_Vector3D pos = pVert0->GetPosition()*x + pVert1->GetPosition()*y + pVert2->GetPosition()*z;
    pNewVert->SetPosition( pos );
    pNewVert->BuildRawNormal();
    this->SetNbrVertex( this->GetNbrVertex()+1 );
    this->SetVertex( this->GetNbrVertex()-1, pNewVert );
    /* assign vertex to faces */
    pFace1->SetVertex( *pVert0, *pVert1, *pNewVert );
    pFace2->SetVertex( *pNewVert, *pVert1, *pVert2 );
    Face.SetVertex( *pVert0, *pNewVert, *pVert2 );
    /* assign dependence vertex->mother face */
    pNewVert->SetFace( Face );
    pVert0->SetFace( Face );
    pVert1->SetFace( *pFace1 );
    pVert2->SetFace( Face );
    /* outer faces */
    if( Face.GetFaceNeighbor(2)!=NULL )
    {
        GW_I32 nEdgeNumber = Face.GetFaceNeighbor(2)->GetEdgeNumber( Face );
        GW_ASSERT( nEdgeNumber>=0 );
        Face.GetFaceNeighbor(2)->SetFaceNeighbor( pFace1, nEdgeNumber );
    }
    if( Face.GetFaceNeighbor(0)!=NULL )
    {
        GW_I32 nEdgeNumber = Face.GetFaceNeighbor(0)->GetEdgeNumber( Face );
        GW_ASSERT( nEdgeNumber>=0 );
        Face.GetFaceNeighbor(0)->SetFaceNeighbor( pFace2, nEdgeNumber );
    }
    /* build connectivity of inner faces */
    pFace1->SetFaceNeighbor( pFace2, &Face, Face.GetFaceNeighbor(2) );
    pFace2->SetFaceNeighbor( Face.GetFaceNeighbor(0), &Face,  pFace1 );
    GW_Face* pTempFace = Face.GetFaceNeighbor(1);
    Face.SetFaceNeighbor(  pFace2, pTempFace, pFace1 );

    return pNewVert;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::InsertVertexInEdge
/**
 *  \param  Vert1 [GW_Vertex&] 1st vertex.
 *  \param  Vert2 [GW_Vertex&] 2nd vertex.
 *  \param  x [GW_Float] Barycentric coords in [0,1].
 *  \return [GW_Vertex&] The vertex. This is a newly created one only if 0<<x<<1.
 *
 *  Insert a vertex in the middle of an edge.
 */
/*------------------------------------------------------------------------------*/
GW_Vertex* GW_Mesh::InsertVertexInEdge( GW_Vertex& Vert1, GW_Vertex& Vert2, GW_Float x, GW_Bool& bIsNewVertCreated )
{
    if( x<GW_EPSILON )
    {
        bIsNewVertCreated = GW_False;
        return &Vert2;
    }
    if( x>1-GW_EPSILON )
    {
        bIsNewVertCreated = GW_False;
        return &Vert1;
    }
    bIsNewVertCreated = GW_True;
    /* create the new vertex */
    GW_Vertex* pNewVert = &this->CreateNewVertex();
    this->SetNbrVertex( this->GetNbrVertex()+1 );
    this->SetVertex( this->GetNbrVertex()-1, pNewVert );
    /* set position */
    pNewVert->SetPosition( Vert1.GetPosition()*x + Vert2.GetPosition()*(1-x) );
    /* retrieve the neighbor faces face */
    GW_Face* pFace1 = NULL;
    GW_Face* pFace2 = NULL;
    Vert1.GetFaces( Vert2, pFace1, pFace2 );
    GW_ASSERT( pFace1!=NULL || pFace2!=NULL );
    /* assign the face of new vertex */
    if( pFace1!=NULL )
        pNewVert->SetFace( *pFace1 );
    else if( pFace2!=NULL )
        pNewVert->SetFace( *pFace2 );
    GW_I32 nVert1Num1, nVert1Num2, nVert1Num3, nVert2Num1, nVert2Num2, nVert2Num3;
    GW_Face* pNewFace1 = NULL;
    GW_Face* pNewFace2 = NULL;
    nVert1Num3 = nVert2Num3 = 0;
    if( pFace1!=NULL )
    {
        nVert1Num1 = pFace1->GetEdgeNumber( Vert1 );
        GW_ASSERT( nVert1Num1>=0 );
        nVert1Num2 = pFace1->GetEdgeNumber( Vert2 );
        GW_ASSERT( nVert1Num2>=0 );
        nVert1Num3 = 3-nVert1Num2-nVert1Num1;
        pNewFace1 = &this->CreateNewFace();
        this->SetNbrFace( this->GetNbrFace()+1 );
        this->SetFace( this->GetNbrFace()-1, pNewFace1 );
        pNewFace1->SetVertex( *pFace1->GetVertex(nVert1Num3), nVert1Num3 );
        pNewFace1->SetVertex( Vert2, nVert1Num2 );
        pNewFace1->SetVertex( *pNewVert, nVert1Num1 );
        /* connectivity between Face1 and new face */
        GW_Face* pFaceNeighbor = pFace1->GetFaceNeighbor(nVert1Num1);
        pNewFace1->SetFaceNeighbor( pFaceNeighbor, nVert1Num1 );
        pNewFace1->SetFaceNeighbor( pFace1, nVert1Num2 );
        if( pFaceNeighbor!=NULL )
        {
            GW_I32 nNum = pFaceNeighbor->GetEdgeNumber( Vert2, *pFace1->GetVertex(nVert1Num3) );
            GW_ASSERT( nNum>=0 );
            pFaceNeighbor->SetFaceNeighbor( pNewFace1, nNum );
        }
        /* connectivity for face 1 */
        pFace1->SetFaceNeighbor( pNewFace1, nVert1Num1 );
        pFace1->SetVertex( *pNewVert, nVert1Num2 );
        /* reassign vertex 2 */
        Vert2.SetFace( *pNewFace1 );
    }
    if( pFace2!=NULL )
    {
        nVert2Num1 = pFace2->GetEdgeNumber( Vert1 );
        GW_ASSERT( nVert2Num1>=0 );
        nVert2Num2 = pFace2->GetEdgeNumber( Vert2 );
        GW_ASSERT( nVert2Num2>=0 );
        nVert2Num3 = 3-nVert2Num2-nVert2Num1;
        pNewFace2 = &this->CreateNewFace();
        this->SetNbrFace( this->GetNbrFace()+1 );
        this->SetFace( this->GetNbrFace()-1, pNewFace2 );
        pNewFace2->SetVertex( *pFace2->GetVertex(nVert2Num3), nVert2Num3 );
        pNewFace2->SetVertex( Vert2, nVert2Num2 );
        pNewFace2->SetVertex( *pNewVert, nVert2Num1 );
        /* connectivity between Face2 and new face */
        GW_Face* pFaceNeighbor = pFace2->GetFaceNeighbor(nVert2Num1);
        pNewFace2->SetFaceNeighbor( pFaceNeighbor, nVert2Num1 );
        pNewFace2->SetFaceNeighbor( pFace2, nVert2Num2 );
        if( pFaceNeighbor!=NULL )
        {
            GW_I32 nNum = pFaceNeighbor->GetEdgeNumber( Vert2, *pFace2->GetVertex(nVert2Num3) );
            GW_ASSERT( nNum>=0 );
            pFaceNeighbor->SetFaceNeighbor( pNewFace2, nNum );
        }
        /* connectivity for face 1 */
        pFace2->SetFaceNeighbor( pNewFace2, nVert2Num1 );
        pFace2->SetVertex( *pNewVert, nVert2Num2 );
        /* reassign vertex 2 */
        Vert2.SetFace( *pNewFace2 );
    }
    /* set inter connectivity */
    if( pNewFace1!=NULL )
        pNewFace1->SetFaceNeighbor( pNewFace2, nVert1Num3 );
    if( pNewFace2!=NULL )
        pNewFace2->SetFaceNeighbor( pNewFace1, nVert2Num3 );

    /* last thing : compute normal */
    pNewVert->BuildRawNormal();

    return pNewVert;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::IterateConnectedComponent_Vertex
/**
*  \param  pCallback [VertexIterate_Callback] The function.
*
*  Iterate a callback on each vertex of a connected component.
*/
/*------------------------------------------------------------------------------*/
void GW_Mesh::IterateConnectedComponent_Vertex( GW_Vertex& start_vert, VertexIterate_Callback pCallback )
{
    /* march on the voronoi diagram */
    T_VertexList VertexToProceed;
    VertexToProceed.push_back( &start_vert );
    T_VertexMap VertexDone;
    VertexDone[ start_vert.GetID() ] = &start_vert;


    while( !VertexToProceed.empty() )
    {
        GW_Vertex* pVert = VertexToProceed.front();
        GW_ASSERT( pVert!=NULL );
        VertexToProceed.pop_front();

        /* cut the face */
        pCallback( *pVert );

        /* add neighbors */
        for( GW_VertexIterator it = pVert->BeginVertexIterator(); it!=pVert->EndVertexIterator(); ++it )
        {
            GW_Vertex* pNewVert = (GW_Vertex*) *it;
            if( pNewVert==NULL )
                break;
            GW_ASSERT( pNewVert!=NULL );
            if( VertexDone.find(pNewVert->GetID())==VertexDone.end() )
            {
                VertexToProceed.push_back( pNewVert );
                VertexDone[ pNewVert->GetID() ] = pNewVert;    // so that it won't be added anymore
            }
        }
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::IterateConnectedComponent_Face
/**
*  \param  pCallback [VertexIterate_Callback] The function.
*
*  Iterate a callback on each vertex of a connected component.
*/
/*------------------------------------------------------------------------------*/
void GW_Mesh::IterateConnectedComponent_Face( GW_Face& start_face, FaceIterate_Callback pCallback )
{
    /* march on the voronoi diagram */
    T_FaceList FaceToProceed;
    FaceToProceed.push_back( &start_face );
    T_FaceMap FaceDone;
    FaceDone[ start_face.GetID() ] = &start_face;


    while( !FaceToProceed.empty() )
    {
        GW_Face* pFace = FaceToProceed.front();
        GW_ASSERT( pFace!=NULL );
        FaceToProceed.pop_front();

        /* cut the face */
        pCallback( *pFace );

        /* add neighbors */
        for( GW_U32 i=0; i<3; ++i )
        {
            GW_Face* pNewFace = pFace->GetFaceNeighbor(i);
            if( pNewFace!=NULL && FaceDone.find(pNewFace->GetID())==FaceDone.end() )
            {
                FaceToProceed.push_back( pNewFace );
                FaceDone[ pNewFace->GetID() ] = pNewFace;    // so that it won't be added anymore
            }
        }
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::ReOrientMesh
/**
 *  \param  start_face [GW_Face&] The seed face.
 *
 *  Reorient the mesh using this start vertex.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::ReOrientMesh( GW_Face& start_face )
{
    /* march on the voronoi diagram */
    T_FaceList FaceToProceed;
    FaceToProceed.push_back( &start_face );
    T_FaceMap FaceDone;
    FaceDone[ start_face.GetID() ] = &start_face;


    while( !FaceToProceed.empty() )
    {
        GW_Face* pFace = FaceToProceed.front();
        GW_ASSERT( pFace!=NULL );
        FaceToProceed.pop_front();

        /* add neighbors */
        for( GW_U32 i=0; i<3; ++i )
        {
            GW_Vertex* pVertDir = pFace->GetVertex(i);    GW_ASSERT( pVertDir!=NULL );
            GW_Face* pNewFace = pFace->GetFaceNeighbor(*pVertDir);
            if( pNewFace!=NULL && FaceDone.find(pNewFace->GetID())==FaceDone.end() )
            {
                /* find the two other vertices */
                GW_U32 i1 = (i+1)%3;
                GW_U32 i2 = (i+2)%3;
                GW_Vertex* pNewVert[3];
                pNewVert[0] = pFace->GetVertex(i2);                        GW_ASSERT( pNewVert[0]!=NULL );
                pNewVert[1] = pFace->GetVertex(i1);                        GW_ASSERT( pNewVert[1]!=NULL );
                pNewVert[2] = pNewFace->GetVertex(*pNewVert[0], *pNewVert[1]);    GW_ASSERT( pNewVert[2]!=NULL );
                GW_Face* pNeigh[3];
                pNeigh[0] = pNewFace->GetFaceNeighbor( *pNewVert[0] );
                pNeigh[1] = pNewFace->GetFaceNeighbor( *pNewVert[1] );
                pNeigh[2] = pNewFace->GetFaceNeighbor( *pNewVert[2] );
                /* reorient the face */
                pNewFace->SetVertex( *pNewVert[0], *pNewVert[1], *pNewVert[2] );
                pNewFace->SetFaceNeighbor( pNeigh[0], pNeigh[1], pNeigh[2] );
                FaceToProceed.push_back( pNewFace );
                FaceDone[ pNewFace->GetID() ] = pNewFace;    // so that it won't be added anymore
            }
        }
    }

    /* check for global orientation (just an heuristic) */
    GW_Face* pFace = this->GetFace(0);    GW_ASSERT( pFace!=NULL );
    GW_Vector3D v = pFace->GetVertex(0)->GetPosition() +
                    pFace->GetVertex(1)->GetPosition() +
                    pFace->GetVertex(2)->GetPosition();
    GW_Vector3D n = pFace->ComputeNormal();
    if( n*v<0 )
        this->FlipOrientation();
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::FlipOrientation
/**
 *  Flip each face.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::FlipOrientation()
{
    for( GW_U32 i=0; i<this->GetNbrFace(); ++i )
    {
        GW_Face* pFace = this->GetFace(i);    GW_ASSERT( pFace!=NULL );
        pFace->SetVertex( *pFace->GetVertex(1), *pFace->GetVertex(0), *pFace->GetVertex(2) );
        pFace->SetFaceNeighbor( pFace->GetFaceNeighbor(1), pFace->GetFaceNeighbor(0), pFace->GetFaceNeighbor(2) );
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::ReOrientNormals
/**
 *  Align normals so that they are in the same direction with face
 *  normals.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::ReOrientNormals()
{
    for( GW_U32 i=0; i<this->GetNbrFace(); ++i )
    {
        GW_Face* pFace = this->GetFace(i);    GW_ASSERT( pFace!=NULL );
        GW_Vector3D n = pFace->ComputeNormal();
        for( GW_U32 k=0; k<3; ++k )
        {
            GW_Vector3D& nv = pFace->GetVertex(k)->GetNormal();
            if( nv*n < 0 )
                nv = -nv;
        }
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::FlipNormals
/**
 *  Flip each normal of the mesh.
 */
/*------------------------------------------------------------------------------*/
void GW_Mesh::FlipNormals()
{
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        GW_Vertex* pVert = this->GetVertex(i);    GW_ASSERT( pVert!=NULL );
        GW_Vector3D& n = pVert->GetNormal();
        n = -n;
    }
}


void GW_Mesh::CheckIntegrity()
{
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        GW_Vertex* pVert = this->GetVertex(i); GW_ASSERT( pVert!=NULL );
        GW_Face* pFace = pVert->GetFace();    GW_ASSERT( pFace!=NULL );
        GW_ASSERT(!(pFace!=NULL && pFace->GetVertex(0)!=pVert &&
            pFace->GetVertex(1)!=pVert &&
            pFace->GetVertex(2)!=pVert));
    }
    for( GW_U32 i=0; i<this->GetNbrFace(); ++i )
    {
        GW_Face* pFace = this->GetFace(i);    GW_ASSERT( pFace!=NULL );
        for( GW_U32 k=0; k<3; ++k )
        {
            GW_U32 k1 = (k+1)%3;
            GW_U32 k2 = (k+2)%3;
            GW_Face* pNeighFace = pFace->GetFaceNeighbor(k);
            GW_Vertex* pV1 = pFace->GetVertex(k1);    GW_ASSERT( pV1!=NULL );
            GW_Vertex* pV2 = pFace->GetVertex(k2);    GW_ASSERT( pV2!=NULL );
            if( pNeighFace!=NULL )
            {
                GW_ASSERT( pNeighFace->GetFaceNeighbor(*pV1, *pV2)==pFace );
                GW_ASSERT( pFace->GetFaceNeighbor(*pV1, *pV2)==pNeighFace);
            }
        }
    }
}

void GW_Mesh::ExtractBoundary( GW_Vertex& seed, T_VertexList& boundary, T_VertexMap* pExtracted )
{
    GW_ASSERT( seed.IsBoundaryVertex() );
    GW_Vertex* pPrev = NULL;
    GW_Vertex* pCur = &seed;
    GW_Vertex* pNext = NULL;
    GW_U32 num = 0;
    do
    {
        num++;
        boundary.push_back(pCur);
        if( pExtracted!=NULL )
            (*pExtracted)[ pCur->GetID() ] = pCur;
        pNext = NULL;
        for( GW_VertexIterator it = pCur->BeginVertexIterator();
                (it!=pCur->EndVertexIterator()) && (pNext==NULL); ++it )
        {
            GW_Vertex* pVert = *it;
            if( pVert->IsBoundaryVertex() && pVert!=pPrev )
                pNext = pVert;
        }
        GW_ASSERT( pNext!=NULL );
        pPrev = pCur;
        pCur = pNext;
    }
    while( pCur!=&seed && pNext!=NULL && num<this->GetNbrVertex() );
}


void GW_Mesh::ExtractAllBoundaries( std::list<T_VertexList>& boundary_list )
{
    T_VertexMap AlreadyExtracted;
    for( GW_U32 i=0; i<this->GetNbrVertex(); ++i )
    {
        GW_Vertex* pVert = this->GetVertex(i);    GW_ASSERT( pVert!=NULL );
        if( pVert->IsBoundaryVertex() && AlreadyExtracted.find(i)==AlreadyExtracted.end() )
        {
            T_VertexList boundary;
            this->ExtractBoundary( *pVert, boundary, &AlreadyExtracted );
            boundary_list.push_back( boundary );
        }
    }
}

GW_Float GW_Mesh::GetPerimeter( GW_U32* nNbrBoundaries )
{
    GW_Float rPerimeter = 0;
    std::list<T_VertexList> boundary_list;
    this->ExtractAllBoundaries( boundary_list );
    if( nNbrBoundaries!=NULL )
        *nNbrBoundaries = (GW_U32) boundary_list.size();
    for( std::list<T_VertexList>::iterator it = boundary_list.begin(); it!=boundary_list.end(); ++it )
    {
        T_VertexList& boundary = *it;
        rPerimeter += GW_Mesh::GetPerimeter( boundary );
    }

    return rPerimeter;
}

GW_Float GW_Mesh::GetPerimeter( T_VertexList& boundary, GW_Bool bCyclic )
{
    GW_Float rPerimeter = 0;
    GW_Vertex* pPrev = NULL;
    GW_Vertex* pVert = NULL;
    for( IT_VertexList it = boundary.begin(); it!=boundary.end(); ++it )
    {
        pVert = *it;
        if( pPrev!=NULL )
        {
            rPerimeter += (pPrev->GetPosition()-pVert->GetPosition()).Norm();
        }
        pPrev = pVert;
    }
    if( boundary.size()>1 && bCyclic )
    {
        rPerimeter += (boundary.front()->GetPosition()-pVert->GetPosition()).Norm();
    }
    return rPerimeter;
}
