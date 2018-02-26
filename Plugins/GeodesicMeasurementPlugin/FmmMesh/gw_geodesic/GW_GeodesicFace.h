
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicFace.h
 *  \brief  Definition of class \c GW_GeodesicFace
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_GEODESICFACE_H_
#define _GW_GEODESICFACE_H_

#include "../gw_core/GW_Config.h"
#include "../gw_core/GW_Face.h"
#include "GW_TriangularInterpolation_Quadratic.h"
#include "GW_TriangularInterpolation_Linear.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_GeodesicFace
 *  \brief  A face to make geodesic computations.
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Should contains geodesic vertex.
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_GeodesicFace:    public GW_Face
{

public:

    GW_GeodesicFace();
    ~GW_GeodesicFace() override;
    using GW_Face::operator=;
    void SetUpTriangularInterpolation();
    void ComputeGradient( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2,
                          GW_Float x, GW_Float y, GW_Float& dx, GW_Float& dy );

    static void SetTriangularInterpolationType( GW_TriangularInterpolation_ABC::T_TriangulationInterpolationType TriangulationInterpolationType );
    static GW_TriangularInterpolation_ABC::T_TriangulationInterpolationType GetTriangularInterpolationType();

    GW_TriangularInterpolation_ABC* GetTriangularInterpolation();
    void SetTriangularInterpolation(GW_TriangularInterpolation_ABC& TriangularInterpolation);

private:

    /** the interpolation type we should use */
    static GW_TriangularInterpolation_ABC::T_TriangulationInterpolationType TriangulationInterpolationType_;
    /** the data for interpolation */
    GW_TriangularInterpolation_ABC* pTriangularInterpolation_;

};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_GeodesicFace.inl"
#endif


#endif // _GW_GEODESICFACE_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
