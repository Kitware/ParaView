/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicPoint.cpp
 *  \brief  Definition of class \c GW_GeodesicPoint
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 */
/*------------------------------------------------------------------------------*/


#ifdef GW_SCCSID
    static const char* sccsid = "@(#) GW_GeodesicPoint.cpp(c) Gabriel Peyré2003";
#endif // GW_SCCSID

#include "stdafx.h"
#include "GW_GeodesicPoint.h"
#include "GW_GeodesicFace.h"

#ifndef GW_USE_INLINE
    #include "GW_GeodesicPoint.inl"
#endif

using namespace GW;

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::SetCurFace
/**
*  \param  CurFace [GW_GeodesicFace&] The face.
*  \author Gabriel Peyré
*  \date   5-3-2003
*
*  Set the current face.
*/
/*------------------------------------------------------------------------------*/
void GW_GeodesicPoint::SetCurFace( GW_GeodesicFace& CurFace )
{
    pCurFace_ = &CurFace;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPoint::GetCurFace
/**
*  \return [GW_GeodesicFace*] The face.
*  \author Gabriel Peyré
*  \date   5-3-2003
*
*  Get the current face.
*/
/*------------------------------------------------------------------------------*/
GW_GeodesicFace* GW_GeodesicPoint::GetCurFace()
{
    return pCurFace_;
}

///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
