
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_TriangularInterpolation_Linear.h
 *  \brief  Definition of class \c GW_TriangularInterpolation_Linear
 *  \author Gabriel Peyré
 *  \date   5-5-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_TRIANGULARINTERPOLATION_LINEAR_H_
#define _GW_TRIANGULARINTERPOLATION_LINEAR_H_

#include "../gw_core/GW_Config.h"
#include "GW_TriangularInterpolation_ABC.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_TriangularInterpolation_Linear
 *  \brief  Perform linear interpolation
 *  \author Gabriel Peyré
 *  \date   5-5-2003
 *
 *  No data is stored in this class.
 */
/*------------------------------------------------------------------------------*/

class GW_TriangularInterpolation_Linear: public GW_TriangularInterpolation_ABC
{

public:

    void SetUpTriangularInterpolation( GW_GeodesicFace& /*Face*/ ) override {};
    void ComputeGradient( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2,
            GW_Float x, GW_Float y, GW_Float& dx, GW_Float& dy ) override;
    GW_Float ComputeValue( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2,
        GW_Float x, GW_Float y ) override;


    T_TriangulationInterpolationType GetType() override
    { return kLinearTriangulationInterpolation; }

private:

};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_TriangularInterpolation_Linear.inl"
#endif


#endif // _GW_TRIANGULARINTERPOLATION_LINEAR_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
