/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicFace.cpp
 *  \brief  Definition of class \c GW_GeodesicFace
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_GeodesicFace.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_GeodesicFace.h"

#ifndef GW_USE_INLINE
    #include "GW_GeodesicFace.inl"
#endif

using namespace GW;

GW_TriangularInterpolation_ABC::T_TriangulationInterpolationType GW_GeodesicFace::TriangulationInterpolationType_ = GW_TriangularInterpolation_ABC::kQuadraticTriangulationInterpolation;

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicFace constructor
/**
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_GeodesicFace::GW_GeodesicFace()
:    GW_Face(),
    pTriangularInterpolation_    ( NULL )
{
    /* nothing */
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicFace destructor
/**
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_GeodesicFace::~GW_GeodesicFace()
{
    GW_DELETE( pTriangularInterpolation_ );
}


/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicFace::SetUpTriangularInterpolation
/**
 *  \author Gabriel Peyré
 *  \date   5-2-2003
 *
 *  Create a triangular interpolation if necessary, and initialize
 *  it.
 */
/*------------------------------------------------------------------------------*/
void GW_GeodesicFace::SetUpTriangularInterpolation()
{
    if( pTriangularInterpolation_==NULL || pTriangularInterpolation_->GetType()!=TriangulationInterpolationType_ )
    {
        GW_DELETE( pTriangularInterpolation_ );
        switch(TriangulationInterpolationType_)
        {
        case GW_TriangularInterpolation_ABC::kLinearTriangulationInterpolation:
            pTriangularInterpolation_ = new GW_TriangularInterpolation_Linear;
            break;
        case GW_TriangularInterpolation_ABC::kQuadraticTriangulationInterpolation:
            pTriangularInterpolation_ = new GW_TriangularInterpolation_Quadratic;
            break;
        case GW_TriangularInterpolation_ABC::kCubicTriangulationInterpolation:
            GW_ASSERT( GW_False );
            break;
        default:
            GW_ASSERT( GW_False );
            pTriangularInterpolation_ = new GW_TriangularInterpolation_Quadratic;
        }
    }
    pTriangularInterpolation_->SetUpTriangularInterpolation( *this );
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicFace::ComputeGradient
/**
*  \param  v0 [GW_GeodesicVertex&] 1st vertex of local frame.
*  \param  v1 [GW_GeodesicVertex&] 2nd vertex.
*  \param  v2 [GW_GeodesicVertex&] 3rd vertex.
*  \param  x [GW_Float] x local coord.
*  \param  y [GW_Float] y local coord.
*  \param  dx [GW_Float&] x coord of the gradient in local coord.
*  \param  dy [GW_Float&] y coord of the gradient in local coord.
*  \author Gabriel Peyré
*  \date   5-2-2003
*
*  Compute the gradient at given point in local frame.
*/
/*------------------------------------------------------------------------------*/
void GW_GeodesicFace::ComputeGradient( GW_GeodesicVertex& v0, GW_GeodesicVertex& v1, GW_GeodesicVertex& v2, GW_Float x, GW_Float y, GW_Float& dx, GW_Float& dy )
{
    GW_ASSERT( pTriangularInterpolation_!=NULL );
    pTriangularInterpolation_->ComputeGradient( v0, v1, v2, x, y, dx, dy );
}



///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
