
/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_SmartCounter.h
 *  \brief Definition of class \c GW_SmartCounter
 *  \author Gabriel Peyré 2001-09-12
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_SmartCounter_h
#define GW_SmartCounter_h

#include "GW_Config.h"

namespace GW {


/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_SmartCounter
 *  \brief  A smart reference counter.
 *  \author Gabriel Peyré 2001-09-12
 *
 *    Each object that want to use reference management must inherit from this class.
 *
 *    Each time an external object wants to use the object for some period, it should call :
 *    \code
 *    MyObject->UseIt();
 *    \endcode
 *    When the external object stop using this object, it must call :
 *    \code
 *    MyObject->ReleaseIt();
 *    if( MyObject->NoLongerUsed() )
 *        GW_DELETE( MyObject );
 *    \endcode
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_SmartCounter
{

public:

    GW_SmartCounter();
    GW_SmartCounter( const GW_SmartCounter& Dup );
    GW_SmartCounter& operator=( const GW_SmartCounter& Dup );
    virtual ~GW_SmartCounter();

    //-------------------------------------------------------------------------
    /** \name memory management */
    //-------------------------------------------------------------------------
    //@{
    void UseIt();
    void ReleaseIt();
    bool NoLongerUsed();
    GW_I32 GetReferenceCounter();
    //@}



    //-------------------------------------------------------------------------
    /** \name helper */
    //-------------------------------------------------------------------------
    //@{
    static GW_Bool CheckAndDelete(GW_SmartCounter* pCounter);
    //@}

private:

    //-------------------------------------------------------------------------
    /** \name memory management */
    //-------------------------------------------------------------------------
    //@{
    GW_I32 nReferenceCounter_;
    //@}
};


} // End namespace GW

#ifdef GW_USE_INLINE
    #include "GW_SmartCounter.inl"
#endif


#endif // GW_SmartCounter_h

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
