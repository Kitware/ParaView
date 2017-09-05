/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Mesh.inl
 *  \brief  Inlined methods for \c GW_Mesh
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_Mesh.h"

namespace GW {

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh constructor
/**
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Mesh::GW_Mesh()
{
    //VertexVector_.resize(0); //LUK:
    FaceVector_.resize(0);

    VertexVector_size = 0;
    VertexVector_ = 0;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh destructor
/**
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  destructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Mesh::~GW_Mesh()
{
    for( GW_U32 i=0; i<this->GetNbrVertex(); i++ )
        GW_SmartCounter::CheckAndDelete( this->GetVertex(i) );
    for( GW_U32 i=0; i<this->GetNbrFace(); i++ )
        GW_SmartCounter::CheckAndDelete( this->GetFace(i) );

    //!!
    delete[] VertexVector_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetNbrVertex
/**
 *  \return [GW_U32] The number
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get the number of vertex *allocated* in the mesh.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_U32 GW_Mesh::GetNbrVertex() const
{
    return (GW_U32) VertexVector_size;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetNbrFace
/**
 *  \return [GW_U32] the number.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get the number of vertex *allocated* in the mesh.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_U32 GW_Mesh::GetNbrFace() const
{
    return (GW_U32) FaceVector_.size();
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::SetFace
/**
 *  \param  nNum [GW_U32] The number of the new face.
 *  \param  pFace [GW_Face*] The new face.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set one of the face.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Mesh::SetFace( GW_U32 nNum, GW_Face* pFace )
{
    GW_ASSERT( nNum<this->GetNbrFace() );
    if( this->GetFace(nNum)!=NULL )
        GW_SmartCounter::CheckAndDelete( this->GetFace(nNum) );
    FaceVector_[nNum] = pFace;
    if( pFace!=NULL )
    {
        pFace->UseIt();
        pFace->SetID(nNum);
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::AddFace
/**
 *  \param  Face [GW_Face&] The new face.
 *  \author Gabriel Peyré
 *  \date   5-16-2003
 *
 *  Add a new face at the end of the list.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Mesh::AddFace( GW_Face& Face )
{
    this->SetNbrFace( this->GetNbrFace()+1 );
    this->SetFace( this->GetNbrFace()-1, &Face );
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::SetVertex
/**
 *  \param  nNum [GW_U32] The number of the vertex.
 *  \param  pVert [GW_Vertex*] The vertex.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Set one of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Mesh::SetVertex( GW_U32 nNum, GW_Vertex* pVert )
{
    GW_ASSERT( nNum<this->GetNbrVertex() );
    if( this->GetVertex(nNum)!=NULL )
        GW_SmartCounter::CheckAndDelete( this->GetVertex(nNum) );
    VertexVector_[nNum] = pVert;
    if( pVert!=NULL )
    {
        pVert->UseIt();
        pVert->SetID(nNum);
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetFace
/**
 *  \param  nNum [GW_U32] The number
 *  \return [GW_Face*] The face.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get one of the faces.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face* GW_Mesh::GetFace( GW_U32 nNum )
{
    GW_ASSERT( nNum<this->GetNbrFace() );
    return FaceVector_[nNum];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetFace
/**
 *  \param  Vert1 [GW_Vertex&] 1st vert
 *  \param  Vert2 [GW_Vertex&] 2nd vert
 *  \param  Vert3 [GW_Vertex&] 3rd vert
 *  \return [GW_Face*] The face
 *  \author Gabriel Peyré
 *  \date   1-19-2004
 *
 *  Get a face from it's 3 vertices.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face* GW_Mesh::GetFace( GW_Vertex& Vert1, GW_Vertex& Vert2, GW_Vertex& Vert3 )
{
    GW_Face* pFace1, *pFace2;
    Vert1.GetFaces(Vert2, pFace1, pFace2);
    if( pFace1!=NULL && pFace1->GetNextVertex(Vert3)!=NULL )
        return pFace1;
    if( pFace2!=NULL && pFace2->GetNextVertex(Vert3)!=NULL )
        return pFace2;
    return NULL;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetVertex
/**
 *  \param  nNum [GW_U32] The number.
 *  \return [GW_Vertex*] The vertex.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  Get one of the vertex.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vertex* GW_Mesh::GetVertex( GW_U32 nNum )
{
    GW_ASSERT( nNum<this->GetNbrVertex() );
    return VertexVector_[nNum];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetFace
/**
*  \param  nNum [GW_U32] The number
*  \return [GW_Face*] The face.
*  \author Gabriel Peyré
*  \date   2-15-2003
*
*  Get one of the faces.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
const GW_Face* GW_Mesh::GetFace( GW_U32 nNum )const
{
    GW_ASSERT( nNum<this->GetNbrFace() );
    return FaceVector_[nNum];
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::GetVertex
/**
*  \param  nNum [GW_U32] The number.
*  \return [GW_Vertex*] The vertex.
*  \author Gabriel Peyré
*  \date   2-15-2003
*
*  Get one of the vertex.
*/
/*------------------------------------------------------------------------------*/
GW_INLINE
const GW_Vertex* GW_Mesh::GetVertex( GW_U32 nNum ) const
{
    GW_ASSERT( nNum<this->GetNbrVertex() );
    return VertexVector_[nNum];
}



/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::SetStaticThis
/**
 *  \author Gabriel Peyré
 *  \date   4-2-2003
 *
 *  Set the global pointer.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Mesh::SetStaticThis()
{
    pStaticThis_ = this;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::UnSetStaticThis
/**
 *  \author Gabriel Peyré
 *  \date   4-2-2003
 *
 *  Set to NULL the global pointer.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Mesh::UnSetStaticThis()
{
    pStaticThis_ = NULL;
}


/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::StaticThis
/**
 *  \return [GW_Mesh&] Current mesh.
 *  \author Gabriel Peyré
 *  \date   4-2-2003
 *
 *  For global access during serialization.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Mesh& GW_Mesh::StaticThis()
{
    GW_ASSERT( pStaticThis_!=NULL );
    return *pStaticThis_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::Reset
/**
 *  \author Gabriel Peyré
 *  \date   4-6-2003
 *
 *  Clear everything.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_Mesh::Reset()
{
    this->SetNbrFace(0);
    this->SetNbrVertex(0);
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::CreateNewVertex
/**
 *  \return [GW_Vertex&] The newly created vertex.
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Allocate memory for a new vertex. You should overload this
 *  method
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Vertex& GW_Mesh::CreateNewVertex()
{
    return *(new GW_Vertex);
}

/*------------------------------------------------------------------------------*/
// Name : GW_Mesh::CreateNewFace
/**
 *  \return [GW_Face&] The newly created face.
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 *
 *  Allocate memory for a new face.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Face& GW_Mesh::CreateNewFace()
{
    return *(new GW_Face);
}


} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
