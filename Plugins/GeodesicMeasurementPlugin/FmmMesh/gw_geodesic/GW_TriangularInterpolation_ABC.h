
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_TriangularInterpolation_ABC.h
 *  \brief  Definition of class \c GW_TriangularInterpolation_ABC
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_TRIANGULARINTERPOLATION_H_
#define _GW_TRIANGULARINTERPOLATION_H_

#include "../gw_core/GW_Config.h"
#include "GW_GeodesicVertex.h"

namespace GW {

class GW_GeodesicFace;

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_TriangularInterpolation_ABC
 *  \brief  Data structure to interpolate data on a triangle.
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Use a \c GW_GeodesicFace to compute the coefficient of a 2nd
 *  degree polynom that fit distance function.
 */
/*------------------------------------------------------------------------------*/

class GW_TriangularInterpolation_ABC
{

public:

    virtual ~GW_TriangularInterpolation_ABC() {}
    enum T_TriangulationInterpolationType
    {
        kLinearTriangulationInterpolation,
        kQuadraticTriangulationInterpolation,
        kCubicTriangulationInterpolation
    };

    virtual void SetUpTriangularInterpolation( GW_GeodesicFace& Face ) = 0;
    virtual void ComputeGradient( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2,
                                    GW_Float x, GW_Float y, GW_Float& dx, GW_Float& dy ) = 0;
    virtual GW_Float ComputeValue( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2,
                                    GW_Float x, GW_Float y ) = 0;
    virtual T_TriangulationInterpolationType GetType() = 0;

};

} // End namespace GW

#endif // _GW_TRIANGULARINTERPOLATION_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
