
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeometryAtlas.h
 *  \brief  Definition of class \c GW_GeometryAtlas
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_GEOMETRYATLAS_H_
#define _GW_GEOMETRYATLAS_H_

#include "../gw_core/GW_Config.h"
#include "GW_GeometryCell.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_GeometryAtlas
 *  \brief  A collection of cells.
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  A collection of cell with interconnections
 */
/*------------------------------------------------------------------------------*/

class GW_GeometryAtlas
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_GeometryAtlas();
    virtual ~GW_GeometryAtlas();
    //@}

    void InitSampling( std::vector<T_TrissectorInfoVector>& CyclicPositionCollection, GW_U32 n );
    void PositionateVertex( T_MeshVector& ParamMeshVector, T_MeshVector& RealMeshVector );

    void SmoothSampling( GW_U32 nNbrIter = 20 );
    static void SmoothSampling( T_GeometryCellVector& CurCellGroup );

    T_GeometryCellVector& GetCellVector()
    { return CellVector_; }
    std::vector<T_GeometryCellVector>& GetCellGroupVector()
    { return CellGroupVector_; }


private:

    T_GeometryCellVector CellVector_;
    std::vector<T_GeometryCellVector> CellGroupVector_;


};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_GeometryAtlas.inl"
#endif


#endif // _GW_GEOMETRYATLAS_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
