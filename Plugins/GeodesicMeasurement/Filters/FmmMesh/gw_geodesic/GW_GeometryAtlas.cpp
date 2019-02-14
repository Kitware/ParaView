/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeometryAtlas.cpp
 *  \brief  Definition of class \c GW_GeometryAtlas
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_GeometryAtlas.cpp(c) Gabriel Peyré2004";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_GeometryAtlas.h"

#ifndef GW_USE_INLINE
    #include "GW_GeometryAtlas.inl"
#endif

using namespace GW;

/*------------------------------------------------------------------------------*/
// Name : GW_GeometryAtlas constructor
/**
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_GeometryAtlas::GW_GeometryAtlas()
{
    /* NOTHING */
}



/*------------------------------------------------------------------------------*/
// Name : GW_GeometryAtlas destructor
/**
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_GeometryAtlas::~GW_GeometryAtlas()
{
    for( IT_GeometryCellVector it = CellVector_.begin(); it!=CellVector_.end(); ++it )
        GW_DELETE(*it);
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeometryAtlas::InitSampling
/**
 *  \param  CyclicPositionCollection [std::vector<T_TrissectorInfoVector>] The position of each base point on each polygon.
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  Initialize the sampling position of each point.
 */
/*------------------------------------------------------------------------------*/
void GW_GeometryAtlas::InitSampling( std::vector<T_TrissectorInfoVector>& CyclicPositionCollection, GW_U32 n )
{
    /* delete previous data */
    for( IT_GeometryCellVector it = CellVector_.begin(); it!=CellVector_.end(); ++it )
        GW_DELETE(*it);
    CellVector_.clear();
    CellGroupVector_.clear();

    for( std::vector<T_TrissectorInfoVector>::iterator it = CyclicPositionCollection.begin();
            it!=CyclicPositionCollection.end(); ++it )
    {
        T_TrissectorInfoVector& TrissectorVector = *it;
        T_GeometryCellVector CurCellGroup;

        /** create the polygon */
        GW_Vector3D Center(0,0,0);    // center is at origin
        GW_TrissectorInfo* tr_prev = NULL;
        for( IT_TrissectorInfoVector it = TrissectorVector.begin(); it!=TrissectorVector.end(); ++it )
        {
            GW_TrissectorInfo* tr = &(*it);
            GW_TrissectorInfo* tr_next = NULL;
            IT_TrissectorInfoVector it2 = it; it2++;
            if( it2!=TrissectorVector.end() )
                tr_next = &(*it2);
            else
                tr_next = &(TrissectorVector.front());
            if( tr_prev==NULL )
                tr_prev = &(TrissectorVector.back());
            GW_GeometryCell* pNewCell = new GW_GeometryCell;
            GW_Vector3D v1 = tr->GetPosition();
            GW_Vector3D v2 = GW_Vector2D( (tr->GetPosition()+tr_next->GetPosition())*0.5 );
            GW_Vector3D v3 = Center;
            GW_Vector3D v4 = GW_Vector2D( (tr->GetPosition()+tr_prev->GetPosition())*0.5 );
            pNewCell->InitSampling( v1, v2, v3, v4 , n, n );

            CellVector_.push_back(pNewCell);    // add to global cell list
            CurCellGroup.push_back(pNewCell);    // add to current group
            tr_prev = tr;
        }


        CellGroupVector_.push_back(CurCellGroup);
    }
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeometryAtlas::PositionateVertex
/**
 *  \param  ParamMeshVector [T_MeshVector&] Parameter spaces
 *  \param  RealMeshVector [T_MeshVector&] Real 3D position
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  Positionate each point on the grid
 */
/*------------------------------------------------------------------------------*/
void GW_GeometryAtlas::PositionateVertex( T_MeshVector& ParamMeshVector, T_MeshVector& RealMeshVector )
{
    GW_ASSERT( ParamMeshVector.size()==RealMeshVector.size() );
    GW_ASSERT( ParamMeshVector.size()==CellGroupVector_.size() );

    IT_MeshVector it_param = ParamMeshVector.begin();
    IT_MeshVector it_real = RealMeshVector.begin();
    std::vector<T_GeometryCellVector>::iterator it_cellsgroup = CellGroupVector_.begin();
    while( it_param!=ParamMeshVector.end() && it_real!=RealMeshVector.end() && it_cellsgroup!=CellGroupVector_.end())
    {
        GW_Mesh* pParamMesh = *it_param;
        GW_Mesh* pRealMesh = *it_real;
        T_GeometryCellVector& GeometryVector = *it_cellsgroup;
        // positionate each square patch
        for( IT_GeometryCellVector it_cell=GeometryVector.begin(); it_cell!=GeometryVector.end(); ++it_cell )
        {
            GW_GeometryCell* pCell = *it_cell;
            pCell->InterpolateAllPositions( *pParamMesh, *pRealMesh );
        }
        it_param++;
        it_real++;
        it_cellsgroup++;
    }

}



/*------------------------------------------------------------------------------*/
// Name : GW_GeometryAtlas::SmoothSampling
/**
*  \param  CyclicPositionCollection [std::vector<T_GeometryCellVector>] collection of cells.
*  \author Gabriel Peyré
*  \date   2-4-2004
*
*  Smooth the sampling into a conformal mapping.
*/
/*------------------------------------------------------------------------------*/
void GW_GeometryAtlas::SmoothSampling( GW_U32 nNbrIter )
{
    for( std::vector<T_GeometryCellVector>::iterator it = CellGroupVector_.begin(); it!=CellGroupVector_.end(); ++it )
    {
        T_GeometryCellVector& CurCellGroup = *it;
        for( GW_U32 i=0; i<nNbrIter; ++i )
            GW_GeometryAtlas::SmoothSampling(CurCellGroup);
    }
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeometryAtlas::SmoothSampling
/**
 *  \param  CurCellGroup [T_GeometryCellVector&] The region.
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  Smooth just one region.
 */
/*------------------------------------------------------------------------------*/
void GW_GeometryAtlas::SmoothSampling( T_GeometryCellVector& CurCellGroup )
{
    GW_Float s = 1.0/8.0;
    GW_Matrix3x3 w(s,s,s,   s,0,s,   s,s,s);
    GW_GeometryCell* cell_prev = NULL;
    for( IT_GeometryCellVector it = CurCellGroup.begin(); it!=CurCellGroup.end(); ++it )
    {
        GW_GeometryCell* cell_cur = *it;
        GW_GeometryCell* cell_next = NULL;
        IT_GeometryCellVector it2 = it; it2++;
        if( it2!=CurCellGroup.end() )
            cell_next = *it2;
        else
            cell_next = CurCellGroup.front();
        if( cell_prev==NULL )
            cell_prev = CurCellGroup.back();

        GW_U32 p = cell_cur->GetWidth();
        GW_U32 q = cell_cur->GetHeigth();


        /* iterate on each point */
        for( GW_U32 i=1; i<p; ++i )
        for( GW_U32 j=1; j<q; ++j )
        {
            GW_Vector3D ac;
            GW_Float nb = 8;
            ac += cell_cur->GetData(i-1,j)        * w.GetData(0,1);
            ac += cell_cur->GetData(i,j-1)        * w.GetData(1,0);
            ac += cell_cur->GetData(i-1,j-1)    * w.GetData(0,0);
            ac += cell_cur->GetData(i,j)        * w.GetData(1,1);

            if( i<p-1 )
                ac = ac + cell_cur->GetData(i+1,j)    * w.GetData(1,0);
            else
                ac = ac + cell_next->GetData(j,cell_next->GetHeigth()-2) * w.GetData(1,0);

            if( j<q-1 )
                ac = ac + cell_cur->GetData(i,j+1) * w.GetData(1,2);
            else
                ac = ac + cell_prev->GetData(cell_prev->GetWidth()-2,i) * w.GetData(2,1);

            if( i<p-1 )
                ac = ac + cell_cur->GetData(i+1,j-1) * w.GetData(2,0);
            else
                ac = ac + cell_next->GetData(j-1,cell_next->GetHeigth()-2) * w.GetData(2,0);

            if( j<q-1 )
                ac = ac + cell_cur->GetData(i-1,j+1) * w.GetData(0,2);
            else
                ac = ac + cell_prev->GetData(cell_prev->GetWidth()-2,i-1) * w.GetData(0,2);

            if( i<p-1 && j<q-1 )
                ac = ac + cell_cur->GetData(i+1,j+1) * w.GetData(2,2);
            else
            {
                if( i==p-1 && j<p-1 )
                    ac = ac + cell_next->GetData(j+1,cell_next->GetHeigth()-2) * w.GetData(2,2);
                else
                {
                    if( i<p-1 && j==q-1 )
                        ac = ac + cell_prev->GetData(cell_prev->GetWidth()-2,i+1) * w.GetData(2,2);
                    else
                    {
                        nb = 0;        // special case
                        ac = 0;
                        for( IT_GeometryCellVector it2 = CurCellGroup.begin(); it2!=CurCellGroup.end(); ++it2 )
                        {
                            GW_GeometryCell* cell = *it2;
                            ac += cell->GetData(cell->GetWidth()-2,cell->GetHeigth()-2) +
                                    cell->GetData(cell->GetWidth()-1,cell->GetHeigth()-2);
                            nb += 2;
                        }
                        ac /= nb;
                    }
                }
            }

            cell_cur->SetData( i,j, ac );
        }

        cell_prev = cell_cur;
    }
}




///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
