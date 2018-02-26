#ifndef _GW_GEODESICVERTEX_H_
#define _GW_GEODESICVERTEX_H_

#include "../gw_core/GW_Config.h"
#include "../gw_core/GW_Vertex.h"
#include "fmmtypes.h"

namespace GW {

class GW_VoronoiVertex;

class FMMMESH_EXPORT GW_GeodesicVertex: public GW_Vertex
{

public:

    NarrowBand::iterator ptr;        //ALEX: for finding points by ID in the heap

    enum T_GeodesicVertexState
    {
        kFar,
        kAlive,
        kDead
    };

    GW_GeodesicVertex();
    ~GW_GeodesicVertex() override;
    using GW_Vertex::operator=;
    GW_Float GetDistance();
    void SetDistance( GW_Float rDistance );
    void SetState( T_GeodesicVertexState nState );
    T_GeodesicVertexState GetState();
    GW_GeodesicVertex* GetFront();
    void SetFront( GW_GeodesicVertex* pFront );
    void ResetGeodesicVertex();
    void ResetParametrizationData();

    void SetBoundaryReached( GW_Bool bBoundaryReached = GW_True );
    GW_Bool GetBoundaryReached();

    static GW_Bool CompareVertex(GW_GeodesicVertex* pVert1, GW_GeodesicVertex* pVert2);

    //-------------------------------------------------------------------------
    /** \name Parametrization helpers. */
    //-------------------------------------------------------------------------
    void SetStoppingVertex( GW_Bool bIsStoppingVertex );
    GW_Bool GetIsStoppingVertex();
    void AddParameterVertex( class GW_VoronoiVertex& VornoiVert, GW_Float rParam );
    void SetParameterVertex( GW_U32 nNum, GW_Float rParam );
    void SetParameterVertex( GW_Float a, GW_Float b, GW_Float c  );
    GW_VoronoiVertex* GetParameterVertex( GW_U32 nNum, GW_Float& pParam );
    //@}

    static void ComputeFrontIntersection( GW_GeodesicVertex& v1, GW_GeodesicVertex& v2, GW_Vector3D* pInter, GW_Float* pLambda );
    static void ComputeFrontIntersection( GW_GeodesicVertex& v1, GW_Float d1, GW_GeodesicVertex& v2, GW_Float d2, GW_Vector3D* pInter, GW_Float* pLambda );

    /** overlap management */
    class T_FrontOverlapInfo
    {
    public:
        GW_GeodesicVertex* pFront1_;
        GW_GeodesicVertex* pFront2_;
        GW_Float rDist1_;
        GW_Float rDist2_;
        T_FrontOverlapInfo( GW_GeodesicVertex* pFront1 = NULL, GW_Float rDist1 = GW_INFINITE )
            :    pFront1_        ( pFront1 ),
            pFront2_        ( NULL ),
            rDist1_        ( rDist1 ),
            rDist2_        ( GW_INFINITE )
        {
        }
        T_FrontOverlapInfo& operator= (const T_FrontOverlapInfo& c)
        {
            pFront1_ = c.pFront1_;
            pFront2_ = c.pFront2_;
            rDist1_ = c.rDist1_;
            rDist2_ = c.rDist2_;
            return *this;
        }
        void RecordOverlap( GW_GeodesicVertex& front, GW_Float dist )
        {
            if( pFront1_==NULL )
            {
                pFront1_ = &front;
                rDist1_ = dist;
            }
            else if( pFront1_==&front )
                rDist1_ = GW_MIN( dist, rDist1_ );
            else if( pFront2_==&front )
                rDist2_ = GW_MIN( dist, rDist2_ );
            else
            {
                pFront2_ = &front;
                rDist2_ = GW_MIN( dist, rDist2_ );
            }
        }
        void Reset()
        {
            pFront1_ = pFront2_ = NULL;
            rDist1_  = rDist2_  = GW_INFINITE;
        }
    };
    T_FrontOverlapInfo& GetFrontOverlapInfo()
    {
        return FrontOverlapInfo_;
    }

private:

    /** current distance */
    GW_Float rDistance_;
    /** state of the vertex : can be far/alive/dead */
    T_GeodesicVertexState nState_;
    /** The vertex from which the front this vertex is in started.
        Can be \c NULL if this vertex hasn't be reached by a front. */
    GW_GeodesicVertex* pFront_;


    //-------------------------------------------------------------------------
    /** \name specific for parametrization computation. */
    //-------------------------------------------------------------------------
    //@{
    GW_Float rParameter_[3];
    GW_VoronoiVertex* pParameterVert_[3];
    GW_Bool bIsStoppingVertex_;
    GW_Bool bBoundaryReached_;
    //@}

    /** to store overlap information */
    T_FrontOverlapInfo FrontOverlapInfo_;

};


/*------------------------------------------------------------------------------*/
/** \name a vector of GW_GeodesicVertex */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<class GW_GeodesicVertex*> T_GeodesicVertexVector;
typedef T_GeodesicVertexVector::iterator IT_GeodesicVertexVector;
typedef T_GeodesicVertexVector::reverse_iterator RIT_GeodesicVertexVector;
typedef T_GeodesicVertexVector::const_iterator CIT_GeodesicVertexVector;
typedef T_GeodesicVertexVector::const_reverse_iterator CRIT_GeodesicVertexVector;
//@}


/*------------------------------------------------------------------------------*/
/** \name a list of GW_GeodesicVertex */
/*------------------------------------------------------------------------------*/
//@{
typedef std::list<class GW_GeodesicVertex*> T_GeodesicVertexList;
typedef T_GeodesicVertexList::iterator IT_GeodesicVertexList;
typedef T_GeodesicVertexList::reverse_iterator RIT_GeodesicVertexList;
typedef T_GeodesicVertexList::const_iterator CIT_GeodesicVertexList;
typedef T_GeodesicVertexList::const_reverse_iterator CRIT_GeodesicVertexList;
//@}

/*------------------------------------------------------------------------------*/
/** \name a map of GW_GeodesicVertex */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, class GW_GeodesicVertex*> T_GeodesicVertexMap;
typedef T_GeodesicVertexMap::iterator IT_GeodesicVertexMap;
typedef T_GeodesicVertexMap::reverse_iterator RIT_GeodesicVertexMap;
typedef T_GeodesicVertexMap::const_iterator CIT_GeodesicVertexMap;
typedef T_GeodesicVertexMap::const_reverse_iterator CRIT_GeodesicVertexMap;
//@}

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_GeodesicVertex.inl"
#endif


#endif // _GW_GEODESICVERTEX_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
