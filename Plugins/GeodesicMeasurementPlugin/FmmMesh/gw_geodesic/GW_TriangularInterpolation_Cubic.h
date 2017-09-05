
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_TriangularInterpolation_ABC.h
 *  \brief  Definition of class \c GW_TriangularInterpolation_ABC
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_TRIANGULARINTERPOLATIONCUBIC_H_
#define _GW_TRIANGULARINTERPOLATIONCUBIC_H_

#include "../gw_core/GW_Config.h"
#include "../gw_core/GW_VertexIterator.h"
#include "GW_TriangularInterpolation_ABC.h"

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

class GW_TriangularInterpolation_Cubic: public GW_TriangularInterpolation_ABC
{

public:

    GW_TriangularInterpolation_Cubic();
    virtual ~GW_TriangularInterpolation_Cubic();

    void SetUpTriangularInterpolation( GW_GeodesicFace& Face ) override;
    void ComputeGradient( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2,
                          GW_Float x, GW_Float y, GW_Float& dx, GW_Float& dy ) override;
    GW_Float ComputeValue( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2,
                            GW_Float x, GW_Float y ) override;


    T_TriangulationInterpolationType GetType() override
    { return kQuadraticTriangulationInterpolation; }



private:

    static void ComputeLocalGradient( GW_GeodesicVertex& Vert );
    void SetLocalGradient( GW_Vector3D& grad, GW_GeodesicFace& Face, GW_GeodesicVertex& Vert );

    void ComputeLocalBasis( GW_GeodesicFace& Face );

    GW_Float Coeffs[9];
    GW_Vector3D u, v;        // orthogonal basis axis
    GW_Vector3D w;            // orthogonal coord system origin

    GW_Bool bIsLocalBasisComputed_;

    GW_Float LocalGradient_[3][2];


};

} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_TriangularInterpolation_Cubic.inl"
#endif


#endif // _GW_TRIANGULARINTERPOLATIONCUBIC_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
