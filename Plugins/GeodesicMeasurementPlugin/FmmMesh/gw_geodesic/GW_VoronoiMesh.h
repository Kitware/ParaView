
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_VoronoiMesh.h
 *  \brief  Definition of class \c GW_VoronoiMesh
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_VORONOIMESHBUILDER_H_
#define _GW_VORONOIMESHBUILDER_H_

#include "../gw_core/GW_Config.h"
#include "GW_GeodesicMesh.h"
#include "GW_GeodesicPath.h"
#include "GW_VoronoiVertex.h"
#include "../gw_core/GW_ProgressBar.h"

namespace GW {


/*------------------------------------------------------------------------------*/
/** \name a map of GW_U32 */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, GW_U32> T_VertNbr2ArrayNbr;
typedef T_VertNbr2ArrayNbr::iterator IT_VertNbr2ArrayNbr;
typedef T_VertNbr2ArrayNbr::reverse_iterator RIT_VertNbr2ArrayNbr;
typedef T_VertNbr2ArrayNbr::const_iterator CIT_VertNbr2ArrayNbr;
typedef T_VertNbr2ArrayNbr::const_reverse_iterator CRIT_VertNbr2ArrayNbr;
//@}

/*------------------------------------------------------------------------------*/
/** \name a map of T_GeodesicVertexList */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, T_GeodesicVertexList*> T_VertexPathMap;
typedef T_VertexPathMap::iterator IT_VertexPathMap;
typedef T_VertexPathMap::reverse_iterator RIT_VertexPathMap;
typedef T_VertexPathMap::const_iterator CIT_VertexPathMap;
typedef T_VertexPathMap::const_reverse_iterator CRIT_VertexPathMap;
//@}


/*------------------------------------------------------------------------------*/
/** \name a multimap of T_GeodesicVertexList */
/*------------------------------------------------------------------------------*/
//@{
typedef std::multimap<GW_U32, T_GeodesicVertexList*> T_VertexPathMultiMap;
typedef T_VertexPathMultiMap::iterator IT_VertexPathMultiMultiMap;
typedef T_VertexPathMultiMap::reverse_iterator RIT_VertexPathMultiMap;
typedef T_VertexPathMultiMap::const_iterator CIT_VertexPathMultiMap;
typedef T_VertexPathMultiMap::const_reverse_iterator CRIT_VertexPathMultiMap;
//@}


/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_VoronoiMesh
 *  \brief  Iterate an algorithm which find base vertex at Voronoy crossing points.
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Remesh the resulting point in a Delaunay fashion.
 */
/*------------------------------------------------------------------------------*/

class GW_VoronoiMesh: public GW_Mesh
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_VoronoiMesh();
    virtual ~GW_VoronoiMesh();
    //@}

    //-------------------------------------------------------------------------
    /** \name Base mesh construction */
    //-------------------------------------------------------------------------
    //@{
    static GW_U32 AddFurthestPoint( T_GeodesicVertexList& VertList, GW_GeodesicMesh& Mesh, GW_Bool bUseRandomStartVertex = GW_False );
    static GW_U32 AddFurthestPointsIterate( T_GeodesicVertexList& VertList, GW_GeodesicMesh& Mesh, GW_U32 nNbrIteration, GW_Bool bUseRandomStartVertex = GW_False, GW_Bool bUseProgressBar = GW_True );
    void BuildMesh( GW_GeodesicMesh& OriginalMesh, GW_Bool bFixHole = GW_True );
    //@}

    //-------------------------------------------------------------------------
    /** \name Parametrization construction. */
    //-------------------------------------------------------------------------
    //@{
    void BuildGeodesicBoundaries( GW_GeodesicMesh& Mesh );
    void BuildGeodesicParametrization( GW_GeodesicMesh& Mesh );
    //@}


    void Reset();
    T_GeodesicVertexList& GetBaseVertexList();
    GW_U32 GetNbrBasePoints();

    T_VertexPathMap& GetGeodesicBoundariesMap();

    void InterpolatePosition( GW_GeodesicMesh& Mesh, GW_Vector3D& Position,
                              GW_VoronoiVertex& v0, GW_VoronoiVertex& v1, GW_VoronoiVertex& v2,
                              GW_Float a, GW_Float b, GW_Float c );
    void InterpolatePositionExhaustiveSearch( GW_GeodesicMesh& Mesh, GW_Vector3D& Position,
        GW_VoronoiVertex& v0, GW_VoronoiVertex& v1, GW_VoronoiVertex& v2,
        GW_Float a, GW_Float b, GW_Float c );



    //-------------------------------------------------------------------------
    /** \name Surface flattening. */
    //-------------------------------------------------------------------------
    //@{
    void GetNaturalNeighborWeights( T_FloatMap& Weights, GW_GeodesicMesh& Mesh, GW_GeodesicVertex& Vert );
    void FlattenBasePoints( GW_GeodesicMesh& Mesh, T_Vector2DMap& FlatteningMap );
    void GetReciprocicalDistanceWeights( T_FloatMap& Weights, GW_GeodesicMesh& Mesh, GW_GeodesicVertex& Vert );
    //@}


    //-------------------------------------------------------------------------
    /** \name Class factory methods. */
    //-------------------------------------------------------------------------
    //@{
    virtual GW_Vertex& CreateNewVertex();
    virtual GW_Face& CreateNewFace();
    //@}

    /* \todo Should be private */
    void PerformLocalFastMarching( GW_GeodesicMesh& Mesh, GW_VoronoiVertex& Vert );
    static GW_Bool FastMarchingCallbackFunction_Parametrization( GW_GeodesicVertex& CurVert );
    static GW_VoronoiVertex* pCurrentVoronoiVertex_;
    static void PerformFastMarching( GW_GeodesicMesh& OriginalMesh, T_GeodesicVertexList& VertList );

    /** helpers for furthest point building */
    static GW_Bool FastMarchingCallbackFunction_VertexInsersion( GW_GeodesicVertex& CurVert, GW_Float rNewDist );
    static void ResetOnlyVertexState( GW_GeodesicMesh& Mesh );


    /** helper for natural neighbor interpolation */
    class GW_GeodesicInformationDuplicata: public GW_SmartCounter
    {
    public:
        GW_GeodesicInformationDuplicata( GW_GeodesicVertex& vert )
            :GW_SmartCounter(),
            rDistance_    ( vert.GetDistance() ),
            pFront_        ( vert.GetFront() )
        { vert.SetUserData(this); }
        GW_Float rDistance_;
        GW_GeodesicVertex* pFront_;
    };

    /** intermediate variable for boundaries building */
    static T_VertexPathMultiMap BoundaryEdgeMap_;
    static void AddPathToMeshVertex( GW_GeodesicMesh& Mesh, GW_GeodesicPath& GeodesicPath, T_GeodesicVertexList& VertexPath );

private:

    static GW_Face* FindMaxFace( GW_GeodesicVertex& Vert );
    static GW_GeodesicVertex* FindMaxVertex( GW_GeodesicMesh& Mesh );

    void CreateVoronoiVertex();

    /** list of current base vertex */
    T_GeodesicVertexList BaseVertexList_;

    /** list all geodesic distance */
    T_FloatMap GeodesicDistanceMap_;

    /** helpers for mesh building */
    static T_VoronoiVertexMap VoronoiVertexMap_;
    static void FastMarchingCallbackFunction_MeshBuilding( GW_GeodesicVertex& CurVert );
    static GW_VoronoiVertex* GetVoronoiFromGeodesic( GW_GeodesicVertex& Vert );
    static GW_Bool TestManifoldStructure( GW_VoronoiVertex& Vert1, GW_VoronoiVertex& Vert2 );
    void FixHole();

    /** helper for parametrization building */
    static T_VoronoiVertexList CurrentTargetVertex_;
    static GW_Bool FastMarchingCallbackFunction_Boundaries( GW_GeodesicVertex& CurVert );
    T_VertexPathMap VertexPathMap_;
    void ComputeVertexParameters( GW_GeodesicMesh& Mesh );

    /** helper for interpolation */
    T_GeodesicVertexMap CentralParameterMap_;
    static GW_Bool GetParameterVertex( GW_GeodesicVertex& Vert, GW_Float& a, GW_Float& b, GW_Float& c,
                             const GW_VoronoiVertex& ParamV0, const GW_VoronoiVertex& ParamV1, const GW_VoronoiVertex& ParamV2 );

    static GW_Float NaturalNeighborContribution( GW_Face& Face, GW_GeodesicVertex& Vert, T_FloatMap& Weights );
    GW_Bool bInterpolationPreparationDone_;
    static GW_Float DistributeContribution( GW_Face& Face, T_FloatMap& Weights,
                                    GW_GeodesicVertex* pVert[3], GW_Vector2D VertPos[3],
                                    T_Vector2DList* pPolyContrib  );
public:

    static GW_Bool FastMarchingCallbackFunction_VertexInsersionNN( GW_GeodesicVertex& CurVert, GW_Float rNewDist );
    static void PrepareInterpolation( GW_GeodesicMesh& Mesh );

    /** helpers for reciprocical distance interpolation */
    static GW_U32 nNbrBaseVertex_RD_;
    static T_FloatMap* pCurWeights_;
    static GW_Bool FastMarchingCallbackFunction_VertexInsersionRD1( GW_GeodesicVertex& CurVert, GW_Float rNewDist );
    static GW_Bool FastMarchingCallbackFunction_VertexInsersionRD2( GW_GeodesicVertex& CurVert, GW_Float rNewDist );
    static GW_Bool FastMarchingCallbackFunction_ForceStopRD( GW_GeodesicVertex& Vert );

};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_VoronoiMesh.inl"
#endif


#endif // _GW_VORONOIMESHBUILDER_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
