#ifndef _GW_GEODESICPATH_H_
#define _GW_GEODESICPATH_H_

#include "../gw_core/GW_Config.h"
#include "../gw_core/GW_Face.h"
#include "../gw_core/GW_VertexIterator.h"
#include "GW_GeodesicPoint.h"
#include "GW_GeodesicMesh.h"

namespace GW {

/** Choose between using linear interpolation for the gradient or quadratic one */
// #define GW_USE_GRADIENT_LINEAR
#define GW_USE_GRADIENT_QUADRATIC

/*------------------------------------------------------------------------------*/
/** \name a list of GW_GeodesicPoint */
/*------------------------------------------------------------------------------*/
//@{
typedef std::list<class GW_GeodesicPoint*> T_GeodesicPointList;
typedef T_GeodesicPointList::iterator IT_GeodesicPointList;
typedef T_GeodesicPointList::reverse_iterator RIT_GeodesicPointList;
typedef T_GeodesicPointList::const_iterator CIT_GeodesicPointList;
typedef T_GeodesicPointList::const_reverse_iterator CRIT_GeodesicPointList;
//@}


/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_GeodesicPath
 *  \brief  A geodesic path, composed of point located on edge.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Just a linked list of point.
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_GeodesicPath
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_GeodesicPath();
    virtual ~GW_GeodesicPath();
    //@}

    T_GeodesicPointList& GetPointList();

    void InitPath( GW_GeodesicVertex& StartVert );
    GW_I32 AddNewPoint();
    void ComputePath( GW_GeodesicVertex& StartVert, GW_U32 nMaxLength = GW_INFINITE );
    void ResetPath();

    void SetStepSize( GW_Float rStepSize );
    GW_Float GetStepSize();

private:

    void AddVertexToPath( GW_GeodesicVertex& Vert );

    T_GeodesicPointList Path_;

    GW_GeodesicFace* pCurFace_;
    GW_GeodesicFace* pPrevFace_;

    GW_Float rStepSize_;

};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_GeodesicPath.inl"
#endif


#endif // _GW_GEODESICPATH_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
