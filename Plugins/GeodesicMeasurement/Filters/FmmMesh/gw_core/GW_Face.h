
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Face.h
 *  \brief  Definition of class \c GW_Face
 *  \author Gabriel Peyré
 *  \date   2-12-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_FACE_H_
#define _GW_FACE_H_

#include "GW_Config.h"
#include "GW_SmartCounter.h"
#include "GW_Vertex.h"
#include "GW_FaceIterator.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Face
 *  \brief  A triangle, with its three vertex and its three neighbors.
 *  \author Gabriel Peyré
 *  \date   2-12-2003
 *
 *  We use smart counter to check whether the face should deallocate a
 *    vertex.
 *
 *    For vertex processing, a face is responsible for the vertex that caries
 *    a pointer on it. In fact, each vertex has a pointer on a single face
 *    that is responsible for that vertex.
 *
 *    Vertex, face and edge are labeled in a consistent way.
 *    This means :
 *        - edge are labeled by the number of the opposite vertex.
 *        - neighbor faces are labeled by the edge number, i.e. by the opposite vertex number.
 *
 *    We refer to these number as "edge number".
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_Face:    public GW_SmartCounter
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_Face();
    ~GW_Face() override;
    virtual GW_Face& operator=(const GW_Face& Face);
    //@}

    //-------------------------------------------------------------------------
    /** \name Accessors */
    //-------------------------------------------------------------------------
    //@{
    void SetFaceNeighbor(GW_Face* pFace, GW_U32 nEdgeNum);
    void SetFaceNeighbor(GW_Face* pFace1, GW_Face* pFace2, GW_Face* pFace3);
    GW_Face* GetFaceNeighbor( GW_U32 nEdgeNum );
    const GW_Face* GetFaceNeighbor( GW_U32 nEdgeNum ) const;
    GW_Face* GetFaceNeighbor( const GW_Vertex& Vert);
    GW_Face* GetFaceNeighbor( const GW_Vertex& Vert1, const GW_Vertex& Vert2 );

    void SetVertex(GW_Vertex& Vert, GW_U32 nNum);
    void SetVertex(GW_Vertex& Vert1, GW_Vertex& Vert2, GW_Vertex& Vert3);
    GW_Vertex* GetVertex( GW_U32 nNum );
    const GW_Vertex* GetVertex( GW_U32 nNum ) const;
    GW_Vertex* GetVertex( const GW_Vertex& Vert1, const GW_Vertex& Vert2 );
    GW_Vertex* GetNextVertex( const GW_Vertex& Vert );

    GW_I32 GetEdgeNumber( const GW_Face& Face );
    GW_I32 GetEdgeNumber( const GW_Vertex& Vert );
    GW_I32 GetEdgeNumber( const GW_Vertex& Vert1, const GW_Vertex& Vert2 );

    GW_Bool IsResponsibleFor( GW_U32 nNum );

    void    SetID(GW_U32 nID);
    GW_U32    GetID() const;

    GW_Float GetArea();
    GW_Vector3D ComputeNormal();
    //@}

private:

    /** our defining vertex */
    GW_Vertex*        Vertex_[3];
    /** the 2 faces around us */
    GW_Face*        FaceNeighbors_[3];
    /** The ID the face, given by the Mesh. Should be in the range [0,...,NbrVertex] */
    GW_U32 nID_;

};

/*------------------------------------------------------------------------------*/
/** \name a list of GW_Face */
/*------------------------------------------------------------------------------*/
//@{
typedef std::list<class GW_Face*> T_FaceList;
typedef T_FaceList::iterator IT_FaceList;
typedef T_FaceList::reverse_iterator RIT_FaceList;
typedef T_FaceList::const_iterator CIT_FaceList;
typedef T_FaceList::const_reverse_iterator CRIT_FaceList;
//@}


/*------------------------------------------------------------------------------*/
/** \name a vector of GW_Face */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<class GW_Face*> T_FaceVector;
typedef T_FaceVector::iterator IT_FaceVector;
typedef T_FaceVector::reverse_iterator RIT_FaceVector;
typedef T_FaceVector::const_iterator CIT_FaceVector;
typedef T_FaceVector::const_reverse_iterator CRIT_FaceVector;
//@}

/*------------------------------------------------------------------------------*/
/** \name a map of GW_Face */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, class GW_Face*> T_FaceMap;
typedef T_FaceMap::iterator IT_FaceMap;
typedef T_FaceMap::reverse_iterator RIT_FaceMap;
typedef T_FaceMap::const_iterator CIT_FaceMap;
typedef T_FaceMap::const_reverse_iterator CRIT_FaceMap;
//@}



} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_Face.inl"
#endif


#endif // _GW_FACE_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
