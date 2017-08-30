/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_SmartCounter.inl
 *  \brief Inlined methods for \c GW_SmartCounter
 *  \author Gabriel Peyré 2001-09-12
 */
/*------------------------------------------------------------------------------*/
#include "GW_SmartCounter.h"

namespace GW {


/*------------------------------------------------------------------------------*/
/**
 * Name : GW_SmartCounter constructor
 *
 *  \author Gabriel Peyré 2001-09-12
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_SmartCounter::GW_SmartCounter()
:    nReferenceCounter_    ( 0 )
{
    /* NOTHING */
}


/*------------------------------------------------------------------------------
 * Name : GW_SmartCounter constructor
 *
 *  \param  Dup EXPLANATION
 *  \return PUT YOUR RETURN VALUE AND ITS EXPLANATION
 *  \author Antoine Bouthors 2001-11-24
 *
 * PUT YOUR COMMENTS HERE
 *------------------------------------------------------------------------------*/
GW_INLINE
GW_SmartCounter::GW_SmartCounter( const GW_SmartCounter& /*Dup*/ )
:    nReferenceCounter_(0)
{
    /* NOTHING */
}


/*------------------------------------------------------------------------------
 * Name : GW_SmartCounter::operator
 *
 *  \param  Dup EXPLANATION
 *  \return PUT YOUR RETURN VALUE AND ITS EXPLANATION
 *  \author Antoine Bouthors 2001-11-24
 *
 * PUT YOUR COMMENTS HERE
 *------------------------------------------------------------------------------*/
GW_INLINE
GW_SmartCounter& GW_SmartCounter::operator=( const GW_SmartCounter& /*Dup*/ )
{
    nReferenceCounter_ = 0;
    return (*this);
}



/*------------------------------------------------------------------------------*/
/**
 * Name : GW_SmartCounter destructor
 *
 *  \author Gabriel Peyré 2001-09-12
 *
 *    Check that nobody is still using the object.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
GW_SmartCounter::~GW_SmartCounter()
{
    GW_ASSERT( nReferenceCounter_==0 );
}



/*------------------------------------------------------------------------------*/
/**
 * Name : GW_SmartCounter::UseIt
 *
 *  \author Gabriel Peyré 2001-09-10
 *
 *    Declare that we use this object. We must call \c ReleaseIt when we no longer use this object.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_SmartCounter::UseIt()
{
    GW_ASSERT( nReferenceCounter_<=50000 );
    nReferenceCounter_++;
}


/*------------------------------------------------------------------------------*/
/**
 * Name : GW_SmartCounter::ReleaseIt
 *
 *  \author Gabriel Peyré 2001-09-10
 *
 *    Declare that we no longer use this object.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
void GW_SmartCounter::ReleaseIt()
{
    GW_ASSERT( nReferenceCounter_>0 );
    nReferenceCounter_--;
}


/*------------------------------------------------------------------------------*/
/**
 * Name : GW_SmartCounter::NoLongerUsed
 *
 *  \return true if no one use this object anymore.
 *  \author Gabriel Peyré 2001-09-10
 *
 *    We can delete the object only if \c NoLongerUsed return true.
 */
/*------------------------------------------------------------------------------*/
GW_INLINE
bool GW_SmartCounter::NoLongerUsed()
{
    GW_ASSERT( nReferenceCounter_>=0 );
    return nReferenceCounter_==0;
}



/*------------------------------------------------------------------------------
 * Name : GW_SmartCounter::GetReferenceCounter
 *
 *  \return the value of the reference counter
 *  \author Antoine Bouthors 2001-11-30
 *
 *------------------------------------------------------------------------------*/
GW_INLINE
GW_I32 GW_SmartCounter::GetReferenceCounter()
{
    return nReferenceCounter_;
}


} // End namespace GW

///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2000-2001 The Orion3D Rewiew Board                         //
//---------------------------------------------------------------------------//
//  This file is under the Orion3D licence.                                  //
//  Refer to orion3d_licence.txt for more details about the Orion3D Licence. //
//---------------------------------------------------------------------------//
//  Ce fichier est soumis a la Licence Orion3D.                              //
//  Se reporter a orion3d_licence.txt pour plus de details sur cette licence.//
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
