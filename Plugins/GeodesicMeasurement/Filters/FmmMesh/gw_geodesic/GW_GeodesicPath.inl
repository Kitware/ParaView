/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_GeodesicPath.inl
 *  \brief  Inlined methods for \c GW_GeodesicPath
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 */
/*------------------------------------------------------------------------------*/

#include "GW_GeodesicPath.h"

namespace GW {

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath constructor
/**
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Constructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicPath::GW_GeodesicPath()
:    pCurFace_    ( NULL ),
    pPrevFace_    ( NULL ),
    rStepSize_    ( 0.01f )
{
    /* NOTHING */
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath destructor
/**
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Destructor.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_GeodesicPath::~GW_GeodesicPath()
{
    this->ResetPath();
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath::GetPointList
/**
 *  \return [T_GeodesicPointList&] The list.
 *  \author Gabriel Peyré
 *  \date   4-10-2003
 *
 *  Get the list of point composing the path.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
T_GeodesicPointList& GW_GeodesicPath::GetPointList()
{
    return Path_;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath::SetStepSize
/**
 *  \param  rStepSize [GW_Float] The new size.
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Set the size of the step. This measured in barycentric coords.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_GeodesicPath::SetStepSize( GW_Float rStepSize )
{
    rStepSize_ = rStepSize;
}

/*------------------------------------------------------------------------------*/
// Name : GW_GeodesicPath::GetStepSize
/**
 *  \return [GW_Float] The size.
 *  \author Gabriel Peyré
 *  \date   4-12-2003
 *
 *  Get the current size of the steps.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_Float GW_GeodesicPath::GetStepSize()
{
    return rStepSize_;
}


} // End namespace GW


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
