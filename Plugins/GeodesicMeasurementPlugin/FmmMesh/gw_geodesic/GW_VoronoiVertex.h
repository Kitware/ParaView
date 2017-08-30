
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_VoronoiVertex.h
 *  \brief  Definition of class \c GW_VoronoiVertex
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_VORONOIVERTEX_H_
#define _GW_VORONOIVERTEX_H_

#include "../gw_core/GW_Config.h"
#include "../gw_core/GW_Vertex.h"
#include "GW_GeodesicVertex.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/** \name a list of GW_VoronoiVertex */
/*------------------------------------------------------------------------------*/
//@{
typedef std::list<class GW_VoronoiVertex*> T_VoronoiVertexList;
typedef T_VoronoiVertexList::iterator IT_VoronoiVertexList;
typedef T_VoronoiVertexList::reverse_iterator RIT_VoronoiVertexList;
typedef T_VoronoiVertexList::const_iterator CIT_VoronoiVertexList;
typedef T_VoronoiVertexList::const_reverse_iterator CRIT_VoronoiVertexList;
//@}

/*------------------------------------------------------------------------------*/
/** \name a map of GW_VoronoiVertex */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, class GW_VoronoiVertex*> T_VoronoiVertexMap;
typedef T_VoronoiVertexMap::iterator IT_VoronoiVertexMap;
typedef T_VoronoiVertexMap::reverse_iterator RIT_VoronoiVertexMap;
typedef T_VoronoiVertexMap::const_iterator CIT_VoronoiVertexMap;
typedef T_VoronoiVertexMap::const_reverse_iterator CRIT_VoronoiVertexMap;
//@}


/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_VoronoiVertex
 *  \brief  A vertex involved in a voronoi mesh.
 *  \author Gabriel Peyré
 *  \date   4-19-2003
 *
 *  Contains a pointer to a classical vertex.
 */
/*------------------------------------------------------------------------------*/

class GW_VoronoiVertex:    public GW_Vertex
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_VoronoiVertex();
    virtual ~GW_VoronoiVertex();
    using GW_Vertex::operator=;
    //@}

    GW_GeodesicVertex* GetBaseVertex();
    const GW_GeodesicVertex* GetBaseVertex() const;
    void SetBaseVertex( GW_GeodesicVertex& BaseVertex );

    //-------------------------------------------------------------------------
    /** \name Graph management. */
    //-------------------------------------------------------------------------
    //@{
    void AddNeighbor( GW_VoronoiVertex& Node );
    void RemoveNeighbor( GW_VoronoiVertex& Node );
    GW_Bool IsNeighbor( GW_VoronoiVertex& Node );
    IT_VoronoiVertexList BeginNeighborIterator();
    IT_VoronoiVertexList EndNeighborIterator();
    //@}


private:

    GW_GeodesicVertex* pBaseVertex_;

    /** data to build the graph before building the triangulation */
    T_VoronoiVertexList NeighborList_;

};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_VoronoiVertex.inl"
#endif


#endif // _GW_VORONOIVERTEX_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
