/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicFace.inl
 *  \brief  Inlined methods for \c GW_GeodesicFace
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_GeodesicFace.h"

namespace GW {


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicFace::SetTriangularInterpolationType
/**
 *  \param  TriangulationInterpolationType [GW_TriangularInterpolation_ABC::T_TriangulationInterpolationType] The type
 *  \author Gabriel Peyré
 *  \date   5-5-2003
 *
 *  Set the way interpolation is performed on triangles.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicFace::SetTriangularInterpolationType( GW_TriangularInterpolation_ABC::T_TriangulationInterpolationType TriangulationInterpolationType )
{
    TriangulationInterpolationType_ = TriangulationInterpolationType;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicFace::GetTriangularInterpolationType
/**
 *  \return [GW_TriangularInterpolation_ABC::T_TriangulationInterpolationType] The mode.
 *  \author Gabriel Peyré
 *  \date   5-5-2003
 *
 *  Return the mode of interpolation.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_TriangularInterpolation_ABC::T_TriangulationInterpolationType GW_GeodesicFace::GetTriangularInterpolationType()
{
    return TriangulationInterpolationType_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicFace::GetTriangularInterpolation
/**
 *  \return [GW_TriangularInterpolation_ABC*] The interpolator.
 *  \author Gabriel Peyré
 *  \date   5-6-2003
 *
 *  Get the interpolator used.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_TriangularInterpolation_ABC* GW_GeodesicFace::GetTriangularInterpolation()
{
    return pTriangularInterpolation_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicFace::GetTriangularInterpolation
/**
 *  \param  pTriangularInterpolation [GW_TriangularInterpolation_ABC*] The interpolator.
 *  \author Gabriel Peyré
 *  \date   5-6-2003
 *
 *  Set the interpolator used.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicFace::SetTriangularInterpolation(GW_TriangularInterpolation_ABC& TriangularInterpolation)
{
    GW_DELETE( pTriangularInterpolation_ );
    pTriangularInterpolation_ = &TriangularInterpolation;
}



} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
