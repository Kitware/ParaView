/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Parameterization.inl
 *  \brief  Inlined methods for \c GW_Parameterization
 *  \author Gabriel Peyré
 *  \date   7-1-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_Parameterization.h"

namespace GW {

GW_INLINE
GW_Float GW_Parameterization::ComputeCenteringEnergy( GW_GeodesicVertex& Vert, GW_GeodesicVertex& StartVert, GW_GeodesicMesh& Mesh )
{
    /* compute a fast marching and confine it to local voronoi digram */
    pCurVoronoiDiagram_ = &StartVert;
    Mesh.RegisterVertexInsersionCallbackFunction( FastMarchingCallbackFunction_Centering );
    GW_VoronoiMesh::ResetOnlyVertexState( Mesh );
    Mesh.PerformFastMarching( &Vert  );
    pCurVoronoiDiagram_ = NULL;
    Mesh.RegisterVertexInsersionCallbackFunction( NULL );

    /* march on the voronoi diagram */
    GW_Float rTotalEnergy = 0;
    T_FaceList FaceToProceed;
    FaceToProceed.push_back( Vert.GetFace() );
    T_FaceMap FaceMap;
    FaceMap[ Vert.GetFace()->GetID() ] = Vert.GetFace();

    while( !FaceToProceed.empty() )
    {
        GW_Face* pFace = FaceToProceed.front();
        GW_ASSERT( pFace!=NULL );
        FaceToProceed.pop_front();

        /* compute contribution, for the moment, simple method.
        Todo : use area define by previous voronoi regions. */
        GW_Float rArea = pFace->GetArea();
        GW_Float d[3];
        for( GW_U32 i=0; i<3; ++i )    // retrieve distance
        {
            GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) pFace->GetVertex(i);
            GW_ASSERT( pVert!=NULL );
            d[i] = pVert->GetDistance();
        }
        rTotalEnergy += rArea*(d[0]*d[0] + d[1]*d[1] + d[2]*d[2])/3;

        /* add neighbors */
        for( GW_U32 i=0; i<3; ++i )
        {
            GW_Face* pNewFace = pFace->GetFaceNeighbor(i);
            if( pNewFace!=NULL && FaceMap.find(pNewFace->GetID())==FaceMap.end() )
            {
                GW_Bool bAddThisFace = GW_False;
                for( GW_U32 nVert=0; nVert<3; ++nVert )
                {
                    GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) pNewFace->GetVertex(i);
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

    return rTotalEnergy;
}

GW_INLINE
void GW_Parameterization::CutEdge( GW_GeodesicVertex& Vert1, GW_GeodesicVertex& Vert2,
                                   GW_GeodesicVertex* &pInter_1, GW_GeodesicVertex* &pInter_2, GW_GeodesicMesh& Mesh  )
{
    GW_Float eps = 0.0;
    GW_U32 nID = GW_Vertex::ComputeUniqueId( Vert1, Vert2 );
    if( CutEdgeMap_.find(nID)==CutEdgeMap_.end() )
    {
        GW_Float lambda;
        GW_Vector3D pos;
        /* this edge has not been previously cutted : do so ! */
        GW_GeodesicVertex::ComputeFrontIntersection( Vert1, Vert2, &pos, &lambda );
        if( lambda<eps )
            lambda = 0;
        if( lambda>1-eps )
            lambda = 1;
        pos = Vert1.GetPosition()*lambda + Vert2.GetPosition()*(1-lambda);
        GW_Float rNewDist = Vert1.GetDistance()*lambda + Vert2.GetDistance()*(1-lambda);
        GW_Vector3D Normal = Vert1.GetNormal()*lambda + Vert2.GetNormal()*(1-lambda);
        Normal.Normalize();

        GW_Vector3D t1( Vert1.GetTexCoordU(), Vert1.GetTexCoordV(), 0 );
        GW_Vector3D t2( Vert2.GetTexCoordU(), Vert2.GetTexCoordV(), 0 );
        GW_Vector3D tex = t1*lambda + t2*(1-lambda);

        pInter_1 = (GW_GeodesicVertex*) &Mesh.CreateNewVertex();
        Mesh.SetNbrVertex( Mesh.GetNbrVertex()+1 );
        Mesh.SetVertex( Mesh.GetNbrVertex()-1, pInter_1 );
        pInter_1->SetPosition( pos );
        pInter_1->SetDistance( rNewDist );
        pInter_1->SetFront( Vert1.GetFront() );
        pInter_1->SetNormal( Normal );
        pInter_1->SetTexCoords( tex[0],tex[1] );

        pInter_2 = (GW_GeodesicVertex*) &Mesh.CreateNewVertex();
        Mesh.SetNbrVertex( Mesh.GetNbrVertex()+1 );
        Mesh.SetVertex( Mesh.GetNbrVertex()-1, pInter_2 );
        pInter_2->SetPosition( pos );
        pInter_2->SetDistance( rNewDist );
        pInter_2->SetFront( Vert2.GetFront() );
        pInter_2->SetNormal( Normal );
        pInter_2->SetTexCoords( tex[0],tex[1] );

        /* save the cut */
        GW_EdgeCut Cut( Vert1, Vert2, *pInter_1, *pInter_2 );
        CutEdgeMap_[nID] = Cut;
    }
    else
    {
        /* this edge has already been cut. Use this cut */
        GW_EdgeCut& Cut = CutEdgeMap_[nID];
        pInter_1 = Cut.GetNewVertex( Vert1 );
        pInter_2 = Cut.GetNewVertex( Vert2 );
    }
}

GW_INLINE
GW_GeodesicFace* CreateNewFace( GW_GeodesicMesh& Mesh, GW_GeodesicVertex* pV1, GW_GeodesicVertex* pV2, GW_GeodesicVertex* pV3 )
{
    GW_GeodesicFace* pNewFace = (GW_GeodesicFace*) &Mesh.CreateNewFace();
    pNewFace->SetVertex( *pV1, *pV2, *pV3);
    Mesh.SetNbrFace( Mesh.GetNbrFace()+1 );
    Mesh.SetFace( Mesh.GetNbrFace()-1, pNewFace );
    return pNewFace;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Parameterization::CutFace
/**
 *  \param  Face [GW_Face&] The face to cut.
 *  \param  BaseVert [GW_GeodesicVertex&] The origin of the voronoi region we are segmenting.
 *  \author Gabriel Peyré
 *  \date   7-1-2003
 *
 *  Cut a face according to voronoi diagrams.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Parameterization::CutFace( GW_Face& Face, GW_GeodesicVertex& BaseVert, GW_GeodesicMesh& Mesh, T_TrissectorInfoMap* pTrissectorInfoMap )
{
    GW_Vector3D pos;

    /* for each vertex see if there is a contribution */
    GW_U32 nNumContrib = 0;
    GW_U32 nNumOther = 0;
    GW_GeodesicVertex* pVertContrib[3]    =    { NULL, NULL, NULL };    // the vertice contributing
    GW_Float d[3]                        =    { -1, -1, -1 };            // the distance
    for( GW_U32 i=0; i<3; ++i )
    {
        GW_GeodesicVertex* pVert = (GW_GeodesicVertex*) Face.GetVertex(i);    GW_ASSERT( pVert!=NULL );
        if( pVert->GetFront()==&BaseVert )
        {
            pVertContrib[nNumContrib] = pVert;
            d[nNumContrib] = pVert->GetDistance();
            nNumContrib++;
        }
        else
        {
            pVertContrib[2-nNumOther] = pVert;
            d[2-nNumOther] = pVert->GetDistance();
            nNumOther++;
        }
    }

    /* for each special case, Compute the polygon */
    switch(nNumContrib) {
    case 1:
    {
        /* the contributor is pVertContrib[0], and pVertContrib[1] & pVertContrib[2] are not contributing */
        if( pVertContrib[1]->GetFront()==pVertContrib[2]->GetFront() )
        {
            /* compute 1st intersection point. See if we can retrieve from neighbors */
            GW_GeodesicVertex* pInter1_1 = NULL;
            GW_GeodesicVertex* pInter1_2 = NULL;
            this->CutEdge( *pVertContrib[0], *pVertContrib[1], pInter1_1, pInter1_2, Mesh );
            /* compute 2nd intersection point */
            GW_GeodesicVertex* pInter2_1 = NULL;
            GW_GeodesicVertex* pInter2_2 = NULL;
            this->CutEdge( *pVertContrib[0], *pVertContrib[2], pInter2_1, pInter2_2, Mesh );

            /* build the faces. For the moment, don't bother with neighbor relations. */
            Face.SetVertex( *pVertContrib[0], *pInter1_1, *pInter2_1 );
            GW_GeodesicFace* pNewFace1 = CreateNewFace( Mesh,  pVertContrib[1], pVertContrib[2], pInter1_2 );
            GW_GeodesicFace* pNewFace2 = CreateNewFace( Mesh,  pVertContrib[2], pInter2_2, pInter1_2 );
            /* the face belonging of the old vertices can change ! */
            if( pVertContrib[1]->GetFace()==&Face )
                pVertContrib[1]->SetFace( *pNewFace1 );
            if( pVertContrib[2]->GetFace()==&Face )
                pVertContrib[2]->SetFace( *pNewFace2 );
        }
        else
        {
            /* compute 1st intersection point. See if we can retrieve from neighbors */
            GW_GeodesicVertex* pInter1_1 = NULL;
            GW_GeodesicVertex* pInter1_2 = NULL;
            this->CutEdge( *pVertContrib[0], *pVertContrib[1], pInter1_1, pInter1_2, Mesh );
            /* compute 2nd intersection point */
            GW_GeodesicVertex* pInter2_1 = NULL;
            GW_GeodesicVertex* pInter2_2 = NULL;
            this->CutEdge( *pVertContrib[0], *pVertContrib[2], pInter2_1, pInter2_2, Mesh );
            /* compute 3nd intersection point */
            GW_GeodesicVertex* pInter3_1 = NULL;
            GW_GeodesicVertex* pInter3_2 = NULL;
            this->CutEdge( *pVertContrib[1], *pVertContrib[2], pInter3_1, pInter3_2, Mesh );

            /* compute center vertices */
            GW_Float rNewDist = (pInter1_1->GetDistance()+pInter2_1->GetDistance()+pInter3_1->GetDistance())/3;
            GW_Vector3D pos = (pInter1_1->GetPosition()+pInter2_1->GetPosition()+pInter3_1->GetPosition())/3;
            GW_Vector3D Normal = (pInter1_1->GetNormal()+pInter2_1->GetNormal()+pInter3_1->GetNormal())/3;

            GW_GeodesicVertex* pCenter1 = (GW_GeodesicVertex*) &Mesh.CreateNewVertex();
            Mesh.SetNbrVertex( Mesh.GetNbrVertex()+1 );
            Mesh.SetVertex( Mesh.GetNbrVertex()-1, pCenter1 );
            pCenter1->SetDistance(rNewDist);
            pCenter1->SetPosition(pos);
            pCenter1->SetNormal(Normal);
            pCenter1->SetFront( pVertContrib[0]->GetFront() );

            GW_GeodesicVertex* pCenter2 = (GW_GeodesicVertex*) &Mesh.CreateNewVertex();
            Mesh.SetNbrVertex( Mesh.GetNbrVertex()+1 );
            Mesh.SetVertex( Mesh.GetNbrVertex()-1, pCenter2 );
            pCenter2->SetDistance(rNewDist);
            pCenter2->SetPosition(pos);
            pCenter2->SetNormal(Normal);
            pCenter2->SetFront( pVertContrib[1]->GetFront() );

            GW_GeodesicVertex* pCenter3 = (GW_GeodesicVertex*) &Mesh.CreateNewVertex();
            Mesh.SetNbrVertex( Mesh.GetNbrVertex()+1 );
            Mesh.SetVertex( Mesh.GetNbrVertex()-1, pCenter3 );
            pCenter3->SetDistance(rNewDist);
            pCenter3->SetPosition(pos);
            pCenter3->SetNormal(Normal);
            pCenter3->SetFront( pVertContrib[2]->GetFront() );


            /* add trissector information */
            if( pTrissectorInfoMap!=NULL )
            {
                GW_TrissectorInfo trisec1(pVertContrib[0]->GetFront()->GetID(),
                                         pVertContrib[1]->GetFront()->GetID(),
                                         pVertContrib[2]->GetFront()->GetID());
                GW_TrissectorInfo trisec2(pVertContrib[1]->GetFront()->GetID(),
                                        pVertContrib[0]->GetFront()->GetID(),
                                        pVertContrib[2]->GetFront()->GetID());
                GW_TrissectorInfo trisec3(pVertContrib[2]->GetFront()->GetID(),
                                        pVertContrib[0]->GetFront()->GetID(),
                                        pVertContrib[1]->GetFront()->GetID());
                (*pTrissectorInfoMap)[pCenter1->GetID()] = trisec1;
                (*pTrissectorInfoMap)[pCenter2->GetID()] = trisec2;
                (*pTrissectorInfoMap)[pCenter3->GetID()] = trisec3;
            }

            /* Build the faces. For the moment, don't bother with neighbor relations. */
            Face.SetVertex( *pVertContrib[0], *pInter1_1, *pCenter1 );
            CreateNewFace( Mesh,  pVertContrib[0], pCenter1, pInter2_1 );

            GW_GeodesicFace* pNewFace1 = CreateNewFace( Mesh,  pVertContrib[1], pCenter2, pInter1_2 );
            CreateNewFace( Mesh,  pVertContrib[1], pInter3_1, pCenter2 );
            if( pVertContrib[1]->GetFace()==&Face )
                pVertContrib[1]->SetFace( *pNewFace1 );

            GW_GeodesicFace* pNewFace2 = CreateNewFace( Mesh,  pVertContrib[2], pCenter3, pInter3_2 );
            CreateNewFace( Mesh,  pVertContrib[2], pInter2_2, pCenter3 );
            if( pVertContrib[2]->GetFace()==&Face )
                pVertContrib[2]->SetFace( *pNewFace2 );
        }
    }
    break;
    case 2:
    {
        /* compute 1st intersection point. See if we can retrieve from neighbors */
        GW_GeodesicVertex* pInter1_1 = NULL;
        GW_GeodesicVertex* pInter1_2 = NULL;
        this->CutEdge( *pVertContrib[0], *pVertContrib[2], pInter1_1, pInter1_2, Mesh );
        /* compute 2nd intersection point */
        GW_GeodesicVertex* pInter2_1 = NULL;
        GW_GeodesicVertex* pInter2_2 = NULL;
        this->CutEdge( *pVertContrib[1], *pVertContrib[2], pInter2_1, pInter2_2, Mesh );
        /* build the faces. For the moment, don't bother with neighbor relations. */
        Face.SetVertex( *pVertContrib[0], *pInter2_1, *pVertContrib[1] );
        CreateNewFace( Mesh,  pVertContrib[0], pInter1_1, pInter2_1 );
        GW_GeodesicFace* pNewFace2 = CreateNewFace( Mesh,  pInter1_2, pVertContrib[2], pInter2_2 );
        if( pVertContrib[2]->GetFace()==&Face )
            pVertContrib[2]->SetFace( *pNewFace2 );
    }
    break;
    case 3:
        /* no cut should be performed */
        break;
    default:
        GW_ASSERT(GW_False);
        return;
    }
}


} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
