
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicPoint.h
 *  \brief  Definition of class \c GW_GeodesicPoint
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_GEODESICPOINT_H_
#define _GW_GEODESICPOINT_H_

#include "../gw_core/GW_Config.h"
#include "GW_GeodesicVertex.h"

namespace GW {

class GW_GeodesicFace;

/*------------------------------------------------------------------------------*/
/** \name a vector of GW_Vector3D */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<GW_Vector3D> T_SubPointVector;
typedef T_SubPointVector::iterator IT_SubPointVector;
typedef T_SubPointVector::reverse_iterator RIT_SubPointVector;
typedef T_SubPointVector::const_iterator CIT_SubPointVector;
typedef T_SubPointVector::const_reverse_iterator CRIT_SubPointVector;
//@}


/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_GeodesicPoint
 *  \brief  A point on a geodesic path.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Must lie on a edge.
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_GeodesicPoint
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_GeodesicPoint();
    virtual ~GW_GeodesicPoint();
    //@}

    void SetVertex1( GW_GeodesicVertex& Vert1 );
    void SetVertex2( GW_GeodesicVertex& Vert2 );
    void SetCoord( GW_Float rCoord );
    GW_GeodesicVertex* GetVertex1();
    GW_GeodesicVertex* GetVertex2();
    GW_Float GetCoord();

    void SetCurFace( GW_GeodesicFace& Cur );
    GW_GeodesicFace* GetCurFace();

    T_SubPointVector& GetSubPointVector();

private:

    /** starting point of the edge */
    GW_GeodesicVertex* pVert1_;
    /** ending point */
    GW_GeodesicVertex* pVert2_;
    /** coordinate of point is c*P1+(1-c)*P2 */
    GW_Float rCoord_;

    /** the current face */
    GW_GeodesicFace* pCurFace_;

    T_SubPointVector SubPointVector_;

};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_GeodesicPoint.inl"
#endif


#endif // _GW_GEODESICPOINT_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
