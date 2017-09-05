
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Mesh.h
 *  \brief  Definition of class \c GW_Mesh
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_MESH_H_
#define _GW_MESH_H_

#include "GW_Config.h"
#include "GW_Face.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Mesh
 *  \brief  A basic mesh, composed of vertex and faces.
 *  \author Gabriel Peyré
 *  \date   2-15-2003
 *
 *  This class does not provide any advanded feature. You must build
 *    vertex, faces, and connectivity by your own. Remember :
 *        - That any vertex must have a pointer on one of the face it belongs to.
 *        - You must set up the 3 neighbor faces of each face of the mesh
 *          (this encodes connectivity).
 *
 *    This class use smart pointer management. So it delete any pointer once it no
 *    more used.
 *
 *    The size of the mesh must be *explicitly* managed. You can not add
 *    a face number \c n if the mesh has not been resized to contain \c n faces.
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_Mesh
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
  GW_Mesh();
  virtual ~GW_Mesh();
    virtual GW_Mesh& operator=(const GW_Mesh& v);
    //@}

    //-------------------------------------------------------------------------
    /** \name Resize manager. */
    //-------------------------------------------------------------------------
    //@{
    void SetNbrFace( GW_U32 nNum );
    void SetNbrVertex( GW_U32 nNum );

    GW_U32 GetNbrVertex() const;
    GW_U32 GetNbrFace() const;

    void Reset();
    //@}

    //-------------------------------------------------------------------------
    /** \name Vertex/Face management */
    //-------------------------------------------------------------------------
    //@{
    void AddFace( GW_Face& pFace );
    void SetFace( GW_U32 nNum, GW_Face* pFace );
    void SetVertex( GW_U32 nNum, GW_Vertex* pVert );

    GW_Face* GetFace( GW_U32 nNum );
    GW_Face* GetFace( GW_Vertex& Vert1, GW_Vertex& Vert2, GW_Vertex& Vert3 );
    GW_Vertex* GetVertex( GW_U32 nNum );
    const GW_Face* GetFace( GW_U32 nNum ) const;
    const GW_Vertex* GetVertex( GW_U32 nNum ) const;
    //@}

    GW_Float GetBoundingRadius();
    void GetBoundingBox( GW_Vector3D& min, GW_Vector3D& max );
    GW_Vector3D GetBarycenter();
    void ScaleVertex( GW_Float rScale );
    void TranslateVertex( const GW_Vector3D& Vect );

    void BuildConnectivity();
    void BuildRawNormal();
    void BuildCurvatureData();

    GW_Float GetArea();

    //-------------------------------------------------------------------------
    /** \name Boundary helpers */
    //-------------------------------------------------------------------------
    //@{
    void ExtractBoundary( GW_Vertex& seed, T_VertexList& boundary, T_VertexMap* pExtracted = NULL );
    void ExtractAllBoundaries( std::list<T_VertexList>& boundary_list );
    GW_Float GetPerimeter( GW_U32* nNbrBoundaries = NULL );
    static GW_Float GetPerimeter( T_VertexList& boundary, GW_Bool bCyclic = GW_True );
    //@}


    void SetStaticThis();
    void UnSetStaticThis();
    static GW_Mesh& StaticThis();

    GW_Vertex* GetRandomVertex();

    GW_Vertex* InsertVertexInFace( GW_Face& Face, GW_Float x, GW_Float y, GW_Float z );
    GW_Vertex* InsertVertexInEdge( GW_Vertex& Vert1, GW_Vertex& Vert2, GW_Float x, GW_Bool& bIsNewVertCreated );

    void ReOrientMesh( GW_Face& seed );
    void ReOrientNormals();
    void FlipNormals();
    void FlipOrientation();

    //-------------------------------------------------------------------------
    /** \name Class factory methods. */
    //-------------------------------------------------------------------------
    //@{
    virtual GW_Vertex& CreateNewVertex();
    virtual GW_Face& CreateNewFace();
    //@}

    typedef void (*VertexIterate_Callback)( GW_Vertex& vert );
    static void IterateConnectedComponent_Vertex( GW_Vertex& start_vert, VertexIterate_Callback pCallback );
    typedef void (*FaceIterate_Callback)( GW_Face& face );
    static void IterateConnectedComponent_Face( GW_Face& start_face, FaceIterate_Callback pCallback );


    /* helper*/
    void CheckIntegrity();

protected:

    /** contains all faces of the mesh */
    T_VertexVector VertexVector_;
    int            VertexVector_size;

    /** contains all vertex of the mesh */
    T_FaceVector FaceVector_;

    /** for global access during serialization */
    static GW_Mesh* pStaticThis_;

};

/*------------------------------------------------------------------------------*/
/** \name a vector of GW_Mesh */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<class GW_Mesh*> T_MeshVector;
typedef T_MeshVector::iterator IT_MeshVector;
typedef T_MeshVector::reverse_iterator RIT_MeshVector;
typedef T_MeshVector::const_iterator CIT_MeshVector;
typedef T_MeshVector::const_reverse_iterator CRIT_MeshVector;
//@}


} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_Mesh.inl"
#endif


#endif // _GW_MESH_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
