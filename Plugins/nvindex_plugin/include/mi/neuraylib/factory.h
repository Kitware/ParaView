//*****************************************************************************
// Copyright 2018 NVIDIA Corporation. All rights reserved.
//*****************************************************************************
/// \file
/// \brief Main factory function.
//*****************************************************************************

#ifndef MI_NEURAYLIB_FACTORY_H
#define MI_NEURAYLIB_FACTORY_H

#include <mi/base/types.h>
#include <mi/neuraylib/version.h>

namespace mi
{

namespace neuraylib
{

class IAllocator;
class INeuray;

} // namespace neuraylib

} // namespace mi

/** \addtogroup mi_neuray_ineuray
@{
*/

extern "C" {

/// Unique public access point to the \neurayApiName.
///
/// This factory function is the only public access point to all algorithms and data structures in
/// the \neurayLibraryName. It returns a pointer to an instance of the main #mi::neuraylib::INeuray
/// interface, which is used to configure, to start up, to operate, and to shut down
/// \neurayProductName. The #mi_neuray_factory() function may be called only once.
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
mi::neuraylib::INeuray* mi_neuray_factory(
  mi::neuraylib::IAllocator* allocator = 0, mi::Uint32 version = MI_NEURAYLIB_API_VERSION);
}

/*@}*/ // end group mi_neuray_ineuray

#endif // MI_NEURAYLIB_FACTORY_H
