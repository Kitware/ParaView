/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Face.inl
 *  \brief  Inlined methods for \c GW_Face
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_Face.h"

namespace GW {

/*------------------------------------------------------------------------------*/
// Name : GW_Face constructor
/**
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face::GW_Face()
{
    for( GW_U32 i=0; i<3; ++i )
    {
        Vertex_[i] = NULL;
        FaceNeighbors_[i] = NULL;
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::virtual ~GW_Face
/**
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face::~GW_Face()
{
    for( GW_U32 i=0; i<3; ++i )
        GW_SmartCounter::CheckAndDelete( Vertex_[i] );
}


/*------------------------------------------------------------------------------*/
// Name : GW_Face::SetFaceNeighbor
/**
 *  \param  pFace [GW_Face*] The face. Can be NULL for border faces.
 *  \param  nEdgeNum [GW_U32] The number of the edge we are sharing with this face.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set one of the neigbhoring faces.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Face::SetFaceNeighbor(GW_Face* pFace, GW_U32 nEdgeNum)
{
    GW_ASSERT( nEdgeNum<3 );
    FaceNeighbors_[nEdgeNum] = pFace;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::SetFaceNeighbor
/**
 *  \param  pFace1 [GW_Face*] face 0
 *  \param  pFace2 [GW_Face*] face 1
 *  \param  pFace3 [GW_Face*] face 2
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Assign the 3 face in one time.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Face::SetFaceNeighbor(GW_Face* pFace0, GW_Face* pFace1, GW_Face* pFace2)
{
    this->SetFaceNeighbor( pFace0, 0 );
    this->SetFaceNeighbor( pFace1, 1 );
    this->SetFaceNeighbor( pFace2, 2 );
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetFaceNeighbor
/**
 *  \param  nEdgeNum [GW_U32] number of the face.
 *  \return [GW_Face*] The face.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get one of the face neighbor
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face* GW_Face::GetFaceNeighbor(GW_U32 nEdgeNum)
{
    GW_ASSERT( nEdgeNum<3 );
    return FaceNeighbors_[nEdgeNum];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetFaceNeighbor
/**
*  \param  nEdgeNum [GW_U32] number of the face.
*  \return [GW_Face*] The face.
*  \author Gabriel Peyré
*  \date   2-15-2003
*
*  Get one of the face neighbor
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
const GW_Face* GW_Face::GetFaceNeighbor(GW_U32 nEdgeNum) const
{
    GW_ASSERT( nEdgeNum<3 );
    return FaceNeighbors_[nEdgeNum];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetFaceNeighbor
/**
 *  \param  Vert [GW_Vertex&] The vertex.
 *  \return [GW_Face*] The face. Can be NULL if correct vertex not found.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Return the neighboring face in front of the given vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face* GW_Face::GetFaceNeighbor(const GW_Vertex& Vert)
{
    for( GW_U32 i=0; i<3; ++i )
    {
        if( Vertex_[i]==&Vert )
            return FaceNeighbors_[i];
    }
    return NULL;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetFaceNeighbor
/**
*  \param  Vert1 [GW_Vertex&] first vertex of the edge.
*  \param  Vert2 [GW_Vertex&] Second one.
*  \return [GW_Face*] The face. Can be NULL if correct vertex not found.
*  \author Gabriel Peyré
*  \date   2-15-2003
*
*  Return the neighboring face in front of the given vertex.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face* GW_Face::GetFaceNeighbor( const GW_Vertex& Vert1, const GW_Vertex& Vert2 )
{
    GW_I32 nEdge = this->GetEdgeNumber( Vert1, Vert2 );
    if( nEdge<0 )
        return NULL;
    return this->GetFaceNeighbor( nEdge );
}


/*------------------------------------------------------------------------------*/
// Name : GW_Face::SetVertex
/**
 *  \param  Vert [GW_Vertex&] The new vertex.
 *  \param  nNum [GW_U32] Its number.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Face::SetVertex(GW_Vertex& Vert, GW_U32 nNum)
{
    GW_ASSERT( nNum<3 );
    /* check the previous one, delete it if needed */
    GW_SmartCounter::CheckAndDelete( Vertex_[nNum] );

    if( Vert.GetFace()==NULL )
        Vert.SetFace( *this );

    Vertex_[nNum] = &Vert;
    Vert.UseIt();
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::SetVertex
/**
 *  \param  Vert0 [GW_Vertex&] vertex 0
 *  \param  Vert1 [GW_Vertex&] vertex 1
 *  \param  Vert2 [GW_Vertex&] vertex 2
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set all vertex in once.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Face::SetVertex(GW_Vertex& Vert0, GW_Vertex& Vert1, GW_Vertex& Vert2)
{
    this->SetVertex( Vert0, 0 );
    this->SetVertex( Vert1, 1 );
    this->SetVertex( Vert2, 2 );
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetVertex
/**
 *  \param  nNum [GW_U32] The number of the vertex.
 *  \return [GW_Vertex*] the vertex.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get one of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vertex* GW_Face::GetVertex(GW_U32 nNum)
{
    GW_ASSERT( nNum<3 );
    return Vertex_[nNum];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetVertex
/**
*  \param  nNum [GW_U32] The number of the vertex.
*  \return [GW_Vertex*] the vertex.
*  Get one of the vertex.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
const GW_Vertex* GW_Face::GetVertex(GW_U32 nNum) const
{
    GW_ASSERT( nNum<3 );
    return Vertex_[nNum];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetVertex
/**
 *  \param  Vert1 [GW_Vertex&] first vertex of the edge.
 *  \param  Vert2 [GW_Vertex&] Second one.
 *  \return [GW_Vertex*] the vertex. can be NULL if edge doesn't exist.
 *  Get the vertex in front of a given edge.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vertex* GW_Face::GetVertex( const GW_Vertex& Vert1, const GW_Vertex& Vert2 )
{
    GW_I32 nEdge = this->GetEdgeNumber( Vert1, Vert2 );
    if( nEdge<0 )
        return NULL;
    return this->GetVertex( nEdge );
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetNextVertex
/**
 *  \param  Vert [GW_Vertex&] The vertex.
 *  \return [GW_Vertex*] The next one.
 *  \author Gabriel Peyré
 *  \date   4-1-2003
 *
 *  Get the vertex following the one given by the user.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vertex* GW_Face::GetNextVertex( const GW_Vertex& Vert )
{
    for( GW_U32 i=0; i<3; ++i )
    {
        if( Vertex_[i]==&Vert )
            return Vertex_[(i+1)%3];
    }
    return NULL;
}



/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetEdgeNumber
/**
 *  \param  Face [GW_Face&] The neighbor face.
 *  \return [GW_I32] The number. Return -1 if not found.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get the number of the edge whose neighbor face is given.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_I32 GW_Face::GetEdgeNumber( const GW_Face& Face )
{
    for( GW_U32 i=0; i<3; ++i )
    {
        if( FaceNeighbors_[i]==&Face )
            return i;
    }
    return -1;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetEdgeNumber
/**
 *  \param  Vert [GW_Vertex&] The vertex.
 *  \return [GW_I32] The number. Return -1 if not found.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Return the number of the edge in front of the given vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_I32 GW_Face::GetEdgeNumber( const GW_Vertex& Vert )
{
    for( GW_U32 i=0; i<3; ++i )
    {
        if( Vertex_[i]==&Vert )
            return i;
    }
    return -1;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetEdgeNumber
/**
 *  \param  Vert1 [GW_Vertex&] vertex 1
 *  \param  Vert2 [GW_Vertex&] vertex 2
 *  \return [GW_I32] The number. Return -1 if not found.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Return the number of the edge corresponding to the two given
 *  vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_I32 GW_Face::GetEdgeNumber( const GW_Vertex& Vert1, const GW_Vertex& Vert2 )
{
    for( GW_U32 i=0; i<3; ++i )
    {
        if( Vertex_[i]==&Vert1 )
        {
            if( Vertex_[(i+1)%3]==&Vert2 )
                return (i+2)%3;
            if( Vertex_[(i+2)%3]==&Vert2 )
                return (i+1)%3;
        }
    }
    return 0;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Face::IsResponsibleFor
/**
 *  \param  nNum [GW_U32] number of the vertex
 *  \return [GW_Bool] yes/no ?
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Return true if we are the parent of a given vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Bool GW_Face::IsResponsibleFor( GW_U32 nNum )
{
    GW_ASSERT( nNum<3 );;
    if( Vertex_[nNum]==NULL )
        return GW_False;
    return Vertex_[nNum]->GetFace()==this;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Face::SetID
/**
*  \param  nID [GW_U32] New ID
*  \author Gabriel Peyré
*  \date   3-31-2003
*
*  Set the number of the face in the mesh.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Face::SetID(GW_U32 nID)
{
    nID_ = nID;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Face::GetID
/**
*  \return [GW_U32] The ID
*  \author Gabriel Peyré
*  \date   3-31-2003
*
*  Get the ID of the face.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_U32 GW_Face::GetID() const
{
    return nID_;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Face::ComputeNormal
/**
 *  \return [GW_Vector3D] The normal.
 *  \author Gabriel Peyré
 *  \date   7-6-2003
 *
 *  Compute the normal of the face.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vector3D GW_Face::ComputeNormal()
{
    GW_ASSERT( Vertex_[0]!=NULL );
    GW_ASSERT( Vertex_[1]!=NULL );
    GW_ASSERT( Vertex_[2]!=NULL );
    GW_Vector3D& v0 = Vertex_[0]->GetPosition();
    GW_Vector3D& v1 = Vertex_[1]->GetPosition();
    GW_Vector3D& v2 = Vertex_[2]->GetPosition();
    GW_Vector3D n = (v1-v0)^(v2-v0);
    n.Normalize();
    return n;
}

} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
