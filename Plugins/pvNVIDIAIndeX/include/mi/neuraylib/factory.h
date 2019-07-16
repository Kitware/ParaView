/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Main factory function.

#ifndef MI_NEURAYLIB_FACTORY_H
#define MI_NEURAYLIB_FACTORY_H

#include <mi/base/types.h>
#include <mi/base/uuid.h>
#include <mi/base/handle.h>
#include <mi/neuraylib/version.h>

namespace mi {

namespace neuraylib {

class IAllocator;
class INeuray;

} // namespace neuraylib

namespace base {
    
class IInterface;

} // namespace base

} // namespace mi

/** \addtogroup mi_neuray_ineuray
@{
*/

extern "C"
{

/// Unique public access point to the \neurayApiName.
///
/// This factory function is the only public access point to all algorithms and data structures in
/// the \neurayLibraryName. It returns a pointer to an instance of the main #mi::neuraylib::INeuray
/// interface, which is used to configure, to start up, to operate, and to shut down
/// \neurayProductName. The #mi_neuray_factory_deprecated() function may be called only once.
/// This function is deprecated. Please use #mi_factory() instead.
///
/// \param allocator   The memory allocator to be used. This feature is not yet supported.
/// \param version     The desired version of #mi::neuraylib::INeuray. The parameter is an integer
///                    number that specifies the desired API version, which is set by default to the
///                    current API version given in the symbolic constant #MI_NEURAYLIB_API_VERSION.
///                    This parameter supports the use case where an application uses an older
///                    \neurayApiName version but links with a newer \neurayLibraryName. In this
///                    case, the newer library can still support the older API in a binary
///                    compatible fashion. Only in rare circumstances do you need to set the API
///                    version number explicitly.
/// \return            A pointer to an instance of the main #mi::neuraylib::INeuray interface, or
///                    \c NULL in case of failures. Possible reasons for failures are
///                    - \p allocator is not \c NULL,
///                    - the valued passed for \p version is not supported by this library, or
///                    - the function is called a second time.
///                    A typical cause for the second reason is that the library used at runtime
///                    does not match the headers used at compile time.
MI_DLL_EXPORT
mi::neuraylib::INeuray* mi_neuray_factory_deprecated(
    mi::neuraylib::IAllocator* allocator = 0, mi::Uint32 version = MI_NEURAYLIB_API_VERSION);
    
/// Unique public access point to the \neurayApiName.
///
/// This factory function is the only public access point to all algorithms and data structures in
/// the \neurayLibraryName. It returns a pointer to an instance of the class identified by the 
/// given UUID. Currently the function supports the following interfaces:
/// - an instance of the main #mi::neuraylib::INeuray interface, which is used to configure,  
/// to start up, to operate and to shut down \neurayProductName. This interface can be requested 
/// only once.
/// - an instance of the #mi::neuraylib::IVersion class.
///
///
/// \param iid         UUID of the requested interface
/// \return            A pointer to an instance of the requested interface, or \c NULL if there is 
///                    no interface with the requested UUID. This can happen if the library used at
///                    runtime does not match the headers used at compile time. In addition, 
///                    \c NULL is returned if the interface #mi::neuraylib::INeuray is requested a
///                    second time.
MI_DLL_EXPORT
mi::base::IInterface* mi_factory(const mi::base::Uuid& iid);

}

namespace mi {

namespace neuraylib {

/// Convenience function to ease the use of #mi_factory().
/// \param symbol pointer to the mi_factory symbol.
/// \return a pointer to an interface of type \c T or \c NULL if the interface could not be 
///         retrieved. See #mi_factory for supported interfaces.
template <class T>
T* mi_factory( void* symbol)
{
    typedef mi::base::IInterface* INeuray_factory( const mi::base::Uuid& iid);
    INeuray_factory* factory = reinterpret_cast<INeuray_factory*>( symbol);
    mi::base::Handle<mi::base::IInterface> iinterface( factory( typename T::IID()));
    if( !iinterface)
        return 0;
    return iinterface->get_interface<T>();
}
}

}
/*@}*/ // end group mi_neuray_ineuray

#endif // MI_NEURAYLIB_FACTORY_H
