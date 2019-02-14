/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Vertex.h
 *  \brief  Definition of class \c GW_Vertex
 *  \author Gabriel Peyré
 *  \date   2-12-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_VERTEX_H_
#define _GW_VERTEX_H_

#include "GW_Config.h"
#include "GW_SmartCounter.h"

namespace GW {

class GW_Face;
class GW_FaceIterator;
class GW_VertexIterator;

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Vertex
 *  \brief  A vertex, with it's normal and position.
 *  \author Gabriel Peyré
 *  \date   2-12-2003
 *
 *  This is a simple vertex, with only position, normal, and texture
 *    coordinates.
 *
 *    More elaborate vertex may derive from this base class.
 *
 *  To find it self on the mesh connectivity, a vertex has a pointer
 *    on a face it belongs to. This enable to march on the vertex face-neighborhood, etc.
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_Vertex:    public GW_SmartCounter
{

public:

    static GW_Float rTotalArea_;

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_Vertex();
    ~GW_Vertex() override;
    virtual GW_Vertex& operator=(const GW_Vertex& v);
    //@}

    //-------------------------------------------------------------------------
    /** \name Accessors */
    //-------------------------------------------------------------------------
    //@{
    void SetPosition( const GW_Vector3D& Position );
    GW_Vector3D& GetPosition();
    void SetNormal( GW_Vector3D& Normal );
    void SetTexCoords( GW_Float u, GW_Float v );
    void SetTexCoordU( GW_Float u );
    void SetTexCoordV( GW_Float v );
    GW_Float GetTexCoordU();
    GW_Float GetTexCoordV();

    void SetFace( GW_Face& Face );
    GW_Face* GetFace();
    const GW_Face* GetFace() const;

    void GetFaces( const GW_Vertex& Vert, GW_Face*& pFace1, GW_Face*& pFace2 );

    void    SetID(GW_U32 nID);
    GW_U32    GetID() const;

    GW_U32 GetNumberNeighbor();
    GW_Bool IsBoundaryVertex();
    //@}

    //-------------------------------------------------------------------------
    /** \name Characteristic data management. */
    //-------------------------------------------------------------------------
    //@{
    void BuildRawNormal();
    void BuildCurvatureData();
    //@}

    //-------------------------------------------------------------------------
    /** \name Iterator management. */
    //-------------------------------------------------------------------------
    //@{
    GW_FaceIterator BeginFaceIterator();
    GW_FaceIterator EndFaceIterator();

    GW_VertexIterator BeginVertexIterator();
    GW_VertexIterator EndVertexIterator();
    //@}

    //-------------------------------------------------------------------------
    /** \name Curvature accessors. */
    //-------------------------------------------------------------------------
    //@{
    GW_Float GetMinCurv();
    GW_Float GetMaxCurv();
    GW_Float GetGaussianCurv();
    GW_Float GetMeanCurv();
    GW_Float GetMaxAbsCurv();
    GW_Vector3D& GetNormal();
    GW_Vector3D& GetMinCurvDirection();
    GW_Vector3D& GetMaxCurvDirection();
    //@}

    void SetUserData( GW_SmartCounter* pUserData );
    GW_SmartCounter* GetUserData();

    //-------------------------------------------------------------------------
    /** \name ID helpers */
    //-------------------------------------------------------------------------
    //@{
    static GW_U32 ComputeUniqueId( const GW_Vertex& Vert0, const GW_Vertex& Vert1 );
    static GW_U32 ComputeUniqueId( const GW_Vertex& Vert0, const GW_Vertex& Vert1, const GW_Vertex& Vert2 );
    //@}


private:

    void ComputeNormalAndCurvature( GW_Float& rArea );
    void ComputeCurvatureDirections( GW_Float rArea );

    /** Position of the vertex */
    GW_Vector3D Position_;
    /** Normal of the vertex */
    GW_Vector3D Normal_;
    /** Minimum curvature direction */
    GW_Vector3D CurvDirMin_;
    /** maximum curvature direction */
    GW_Vector3D CurvDirMax_;
    /** Minimum curvature */
    GW_Float rMinCurv_;
    /** Maximum curvature */
    GW_Float rMaxCurv_;
    /** Texture coords */
    GW_Float TexCoords_[2];
    /** A pointer on the face which owned the vertex */
    GW_Face* pFace_;
    /** The ID the face, given by the Mesh. Should be in the range [0,...,NbrVertex] */
    GW_U32 nID_;

    /** a user defined data */
    GW_SmartCounter* pUserData_;

};

/*------------------------------------------------------------------------------*/
/** \name a vector of GW_Vertex */
/*------------------------------------------------------------------------------*/
//@{
//typedef std::vector<class GW_Vertex*> T_VertexVector;
typedef GW_Vertex** T_VertexVector;
//typedef T_VertexVector::iterator IT_VertexVector;
//typedef T_VertexVector::reverse_iterator RIT_VertexVector;
//typedef T_VertexVector::const_iterator CIT_VertexVector;
//typedef T_VertexVector::const_reverse_iterator CRIT_VertexVector;
//@}

/*------------------------------------------------------------------------------*/
/** \name a list of GW_Vertex */
/*------------------------------------------------------------------------------*/
//@{
typedef std::list<class GW_Vertex*> T_VertexList;
typedef T_VertexList::iterator IT_VertexList;
typedef T_VertexList::reverse_iterator RIT_VertexList;
typedef T_VertexList::const_iterator CIT_VertexList;
typedef T_VertexList::const_reverse_iterator CRIT_VertexList;
//@}

/*------------------------------------------------------------------------------*/
/** \name a map of GW_Vertex */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, class GW_Vertex*> T_VertexMap;
typedef T_VertexMap::iterator IT_VertexMap;
typedef T_VertexMap::reverse_iterator RIT_VertexMap;
typedef T_VertexMap::const_iterator CIT_VertexMap;
typedef T_VertexMap::const_reverse_iterator CRIT_VertexMap;
//@}


} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_Vertex.inl"
#endif


#endif // _GW_VERTEX_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
