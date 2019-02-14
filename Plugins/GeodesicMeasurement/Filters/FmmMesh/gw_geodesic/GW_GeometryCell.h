
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeometryCell.h
 *  \brief  Definition of class \c GW_GeometryCell
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_GEOMETRYCELL_H_
#define _GW_GEOMETRYCELL_H_

#include "../gw_core/GW_Config.h"
#include "GW_Parameterization.h"
#include "../gw_maths/GW_Matrix2x2.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_GeometryCell_Template
 *  \brief  In fact this is just a 2D array.
 *  \author Gabriel Peyré
 *  \date   2-4-2004
 *
 *  A 2D array, a patch of a 3D mesh.
 */
/*------------------------------------------------------------------------------*/
template<class base_type>
class GW_GeometryCell_Template
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_GeometryCell_Template( GW_U32 nWidth = 0, GW_U32 nHeight = 0 )
    :    pData_( NULL )
    {
        nSize_[0] = nSize_[1] = 0;
        if( nWidth>0 )
            this->Reset(nWidth, nHeight);

    }
    virtual ~GW_GeometryCell_Template()
    {
        GW_DELETEARRAY(pData_);
    }
    //@}

    void Reset( GW_U32 nWidth, GW_U32 nHeight )
    {
        GW_DELETEARRAY(pData_);
        pData_ = new base_type[nWidth*nHeight];
        nSize_[0] = nWidth;
        nSize_[1] = nHeight;
    }

    base_type& GetData( GW_U32 i, GW_U32 j )
    {
        GW_ASSERT( i<nSize_[0] && j<nSize_[1] );
        return pData_[i+nSize_[0]*j];
    }
    void SetData( GW_U32 i, GW_U32 j, base_type& val )
    {
        GW_ASSERT( i<nSize_[0] && j<nSize_[1] );
        if( i<nSize_[0] && j<nSize_[1] )
            pData_[i+nSize_[0]*j] = val;
    }
    void SetNum( GW_U32 num )
    {
        nNum_ = num;
    }
    void GetNum()
    {
        return nNum_;
    }

    GW_U32 GetWidth()
    { return nSize_[0]; }
    GW_U32 GetHeigth()
    { return nSize_[1]; }

protected:

    /** raw data */
    base_type* pData_;
    /** height and width */
    GW_U32 nSize_[2];
    /** number of cell in the atlas */
    GW_U32 nNum_;

};

class GW_GeometryCell: public GW_GeometryCell_Template<GW_Vector3D>
{
public:
    GW_GeometryCell( GW_U32 nWidth = 0, GW_U32 nHeight = 0 )
    :    GW_GeometryCell_Template<GW_Vector3D>(nWidth,nHeight)
    { }


    void Reset( GW_U32 nWidth, GW_U32 nHeight )
    {
        GW_GeometryCell_Template<GW_Vector3D>::Reset(nWidth,nHeight);
        Normal_.Reset(nWidth,nHeight);
    }

    void InitSampling( GW_Vector3D& v1, GW_Vector3D& v2, GW_Vector3D& v3, GW_Vector3D& v4, GW_U32 n, GW_U32 p );

    static GW_Vector3D GetInterpolatedPosition( GW_Mesh& ParamMesh, GW_Mesh& RealMesh, GW_Vector3D& param_pos,
                GW_Face* &seed, GW_Vector3D& Normal );
    void InterpolateAllPositions( GW_Mesh& ParamMesh, GW_Mesh& RealMesh  );

    void SetNormal( GW_U32 i, GW_U32 j, GW_Vector3D& val )
    {
        Normal_.SetData(i,j,val);
    }
    GW_Vector3D& GetNormal(GW_U32 i,GW_U32 j)
    {
        return Normal_.GetData(i,j);
    }

private:

    GW_GeometryCell_Template<GW_Vector3D> Normal_;
};

/*------------------------------------------------------------------------------*/
/** \name a vector of GW_GeometryCell */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<class GW_GeometryCell*> T_GeometryCellVector;
typedef T_GeometryCellVector::iterator IT_GeometryCellVector;
typedef T_GeometryCellVector::reverse_iterator RIT_GeometryCellVector;
typedef T_GeometryCellVector::const_iterator CIT_GeometryCellVector;
typedef T_GeometryCellVector::const_reverse_iterator CRIT_GeometryCellVector;
//@}


} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_GeometryCell.inl"
#endif


#endif // _GW_GEOMETRYCELL_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
