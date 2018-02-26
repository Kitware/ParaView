
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Parameterization.h
 *  \brief  Definition of class \c GW_Parameterization
 *  \author Gabriel Peyré
 *  \date   7-1-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_PARAMETERIZATION_H_
#define _GW_PARAMETERIZATION_H_

#include "../gw_maths/GW_SparseMatrix.h"
#include "../gw_maths/GW_MatrixNxP.h"
#include "../gw_core/GW_Config.h"
#include "../gw_core/GW_ProgressBar.h"
#include "GW_GeodesicMesh.h"
#include "GW_VoronoiMesh.h"

namespace GW {

class GW_TrissectorInfo
{
public:
    GW_TrissectorInfo(GW_I32 interior = -1, GW_I32 exterior1 = -1, GW_I32 exterior2 = -1)
    {
        nInterior_ = interior;
        nExterior1_ = exterior1;
        nExterior2_ = exterior2;
    }

    virtual GW_TrissectorInfo& operator=(const GW_TrissectorInfo& v)
    {
        nInterior_ = v.nInterior_;
        nExterior1_ = v.nExterior1_;
        nExterior2_ = v.nExterior2_;
        Pos_ = v.Pos_;
        return *this;
    }
    GW_I32 GetInterior()
    {
        return nInterior_;
    }
    GW_I32 GetExterior(GW_U32 i)
    {
        GW_ASSERT(i<2);
        if( i==0 )
            return nExterior1_;
        if( i==1 )
            return nExterior2_;
        return -1;
    }
    void SetInterior(GW_U32 val)
    {
        nInterior_ = val;
    }
    void SetExterior(GW_U32 i, GW_U32 val)
    {
        GW_ASSERT(i<2);
        if( i==0 )
            nExterior1_ = val;
        if( i==1 )
            nExterior2_ = val;
    }
    GW_U32 GetId(GW_U32 i)
    {
        GW_ASSERT(i<3);
        if( i==0 )
            return nExterior1_;
        if( i==1 )
            return nExterior2_;
        if( i==2 )
            return nInterior_;
        return -1;
    }
    void SetId(GW_U32 i, GW_U32 val)
    {
        GW_ASSERT(i<3);
        if( i==0 )
            nExterior1_ = val;
        if( i==1 )
            nExterior2_ = val;
        if( i==2 )
            nInterior_ = val;
    }

    void SetPosition( GW_Vector2D& pos )
    { Pos_ = pos; }
    GW_Vector2D& GetPosition()
    { return Pos_; }

private:

    GW_I32 nInterior_;
    GW_I32 nExterior1_;
    GW_I32 nExterior2_;

    GW_Vector2D Pos_;
};


/*------------------------------------------------------------------------------*/
/** \name a map of GW_TrissectorInfo */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, GW_TrissectorInfo> T_TrissectorInfoMap;
typedef T_TrissectorInfoMap::iterator IT_TrissectorInfoMap;
typedef T_TrissectorInfoMap::reverse_iterator RIT_TrissectorInfoMap;
typedef T_TrissectorInfoMap::const_iterator CIT_TrissectorInfoMap;
typedef T_TrissectorInfoMap::const_reverse_iterator CRIT_TrissectorInfoMap;
//@}

/*------------------------------------------------------------------------------*/
/** \name a vector of GW_TrissectorInfo */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<GW_TrissectorInfo> T_TrissectorInfoVector;
typedef T_TrissectorInfoVector::iterator IT_TrissectorInfoVector;
typedef T_TrissectorInfoVector::reverse_iterator RIT_TrissectorInfoVector;
typedef T_TrissectorInfoVector::const_iterator CIT_TrissectorInfoVector;
typedef T_TrissectorInfoVector::const_reverse_iterator CRIT_TrissectorInfoVector;
//@}



/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Parameterization
 *  \brief  Build a parameterization of a mesh.
 *  \author Gabriel Peyré
 *  \date   7-1-2003
 *
 *  First segment the mesh using a Lloyd method.
 *    Then parameterize each region.
 */
/*------------------------------------------------------------------------------*/

class GW_Parameterization
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_Parameterization();
    virtual ~GW_Parameterization();
    //@}

    //-------------------------------------------------------------------------
    /** \name Lloyd algorithm */
    //-------------------------------------------------------------------------
    //@{
    void PerformLloydAlgorithm( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList );
    GW_Bool PerformLloydIteration( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList );
    GW_GeodesicVertex& PerformCentering( GW_GeodesicVertex& Vert, GW_GeodesicMesh& Mesh );
    void PerformPseudoLloydIteration( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList );
    //@}

    //-------------------------------------------------------------------------
    /** \name Segmentation. */
    //-------------------------------------------------------------------------
    //@{
    void SegmentRegion( GW_GeodesicMesh& Mesh, GW_GeodesicVertex& Vert, T_TrissectorInfoMap* pTrissectorInfoMap = NULL );
    void SegmentAllRegions( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList, T_TrissectorInfoMap* pTrissectorInfoMap = NULL );
    //@}

    //-------------------------------------------------------------------------
    /** \name Parameterization. */
    //-------------------------------------------------------------------------
    //@{
    enum T_ParameterizationType
    {
        kGeodesicConformal,
        kTutte,
        kGeodesicIsomap,
        kGeodesicLLE,
    };
    enum T_ResolutionType
    {
        kSpectral,
        kBoundaryFree,
        kBoundaryFixed
    };
    void ParameterizeMesh(    GW_GeodesicMesh& Mesh, GW_VoronoiMesh& VoronoiMesh, GW_GeodesicMesh& FlattenedMesh,
                            T_ParameterizationType ParamType = kGeodesicIsomap, T_ResolutionType ResolType = kSpectral,
                            T_GeodesicVertexMap* pBoundaryVertPos = NULL );

    /* parameterization helpers **************************************************/
    static void BuildConformalMatrix( GW_Mesh& VoronoiMesh, GW_SparseMatrix& K, const GW_MatrixNxP* M = NULL);
    static void BuildTutteMatrix( GW_Mesh& VoronoiMesh, GW_SparseMatrix& K );
    static void ResolutionSpectral( GW_MatrixNxP& K, GW_MatrixNxP& L, GW_U32 EIG = 0 );
    static void ResolutionBoundaryFree( GW_Mesh& VoronoiMesh, GW_SparseMatrix& K, GW_MatrixNxP&  L, GW_MatrixNxP* M = NULL );
    static void ResolutionBoundaryFixed( GW_Mesh& Mesh, GW_SparseMatrix& K, GW_MatrixNxP&  L,
            T_Vector2DMap* pInitialPos = NULL, T_TrissectorInfoMap* pTrissectorInfoMap = NULL,
            T_TrissectorInfoVector* pCyclicPosition = NULL );

    void ParameterizeRegion( GW_GeodesicVertex& Seed, GW_GeodesicMesh& BaseDomain );
    void ParameterizeAllRegions( T_GeodesicVertexList& VertList );
    //@}

    static void ExtractSubMeshes( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList,
                                T_GeodesicMeshVector& MeshVector, T_U32Vector* pGlobal2LocalID = NULL );
    static void SplitMesh( GW_GeodesicMesh& Mesh );

    //-------------------------------------------------------------------------
    /** \name Pretty helper. */
    //-------------------------------------------------------------------------
    //@{
    static void ExplodeRegion( GW_GeodesicVertex& Vert, GW_Float intensity, GW_Float normal_contrib );
    static void ExplodeAllRegions( T_GeodesicVertexList& VertList, GW_Float intensity = 0.1, GW_Float normal_contrib=0.5 );
    //@}

    static void PerformFastMarching( GW_GeodesicMesh& Mesh, T_GeodesicVertexList& VertList );

private:


    /* Lloyd helpers *************************************************************************/
    GW_U32 nNbrMaxIterLloyd_;
    GW_U32 nNbrMaxIterLloydDescent_;
    static GW_GeodesicVertex* pCurVoronoiDiagram_;
    GW_Float ComputeCenteringEnergy( GW_GeodesicVertex& Vert, GW_GeodesicVertex& StartVert, GW_GeodesicMesh& Mesh );
    static GW_Bool FastMarchingCallbackFunction_Centering( GW_GeodesicVertex& CurVert, GW_Float rNewDist );

    /* pseudo Lloyd helpers ******************************************************************/
    static GW_Bool PerformPseudoLloydIteration_Insertion( GW_GeodesicVertex& CurVert, GW_Float rNewDist );
    static void PerformPseudoLloydIteration_NeswDead( GW_GeodesicVertex& Vert );

    /* segmentation helpers ******************************************************************/
    void CutFace( GW_Face& Face, GW_GeodesicVertex& BaseVert, GW_GeodesicMesh& Mesh, T_TrissectorInfoMap* pTrissectorInfoMap = NULL  );
    void CutEdge( GW_GeodesicVertex& Vert1, GW_GeodesicVertex& Vert2,
                GW_GeodesicVertex* &pInter_1, GW_GeodesicVertex* &pInter_2, GW_GeodesicMesh& Mesh  );
    /** a table of synonym */
    T_U32Map VertexConnexion_;

    /* ExtractSubMesh helper ****************************************************************/
    static void ExtractSubMeshes_Callback( GW_Face& face );
    static T_U32Vector FaceVector;
    static T_U32Map FaceMap;
    static T_U32Vector VertexVector;
    static T_U32Map VertexMap;

    /* system resolution *********************************************************************/
    static void SolveSystem( GW_SparseMatrix& M, GW_VectorND& x, GW_VectorND& b );

    /** record information about a cut */
    class GW_EdgeCut
    {
    public:
        GW_EdgeCut()
        {
            pBaseVert_[0]    = NULL;
            pBaseVert_[1]    = NULL;
            pNewVert_[0]    = NULL;
            pNewVert_[1]    = NULL;
        }
        GW_EdgeCut( GW_GeodesicVertex& BaseVert1,    GW_GeodesicVertex& BaseVert2,
                         GW_GeodesicVertex& NewVert1,    GW_GeodesicVertex& NewVert2 )
        {
            pBaseVert_[0]    = &BaseVert1;
            pBaseVert_[1]    = &BaseVert2;
            pNewVert_[0]    = &NewVert1;
            pNewVert_[1]    = &NewVert2;
        }
        GW_EdgeCut& operator = (const GW_EdgeCut& c)
        {
            pBaseVert_[0]    = c.pBaseVert_[0];
            pBaseVert_[1]    = c.pBaseVert_[1];
            pNewVert_[0]    = c.pNewVert_[0];
            pNewVert_[1]    = c.pNewVert_[1];
            return *this;
        }
        GW_GeodesicVertex* GetNewVertex( GW_GeodesicVertex& BaseVert )
        {
            if( &BaseVert == pBaseVert_[0] )
                return pNewVert_[0];
            if( &BaseVert == pBaseVert_[1] )
                return pNewVert_[1];
            return NULL;
        }
    private:

        GW_GeodesicVertex* pBaseVert_[2];
        GW_GeodesicVertex* pNewVert_[2];
    };

    typedef std::map<GW_U32, GW_EdgeCut> T_EdgeCutMap;

    /** record each edge split, with corresponding added vertices */
    T_EdgeCutMap CutEdgeMap_;

    /** record each parameterization */
    T_MeshVector MeshVector_;

};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_Parameterization.inl"
#endif


#endif // _GW_PARAMETERIZATION_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
