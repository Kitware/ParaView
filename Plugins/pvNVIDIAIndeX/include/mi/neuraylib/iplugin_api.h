/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component provided to plugins.

#ifndef MI_NEURAYLIB_IPLUGIN_API_H
#define MI_NEURAYLIB_IPLUGIN_API_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace base { class ILogger; }

namespace neuraylib {

/** \addtogroup mi_neuray_plugins
@{
*/

/// This abstract interface gives access to the \neurayApiName to plugins. It offers functionality
/// similar to INeuray, but does not allow to start or shutdown the library. On the other hand it
/// allows the registration of user-defined classes.
class IPlugin_api : public
    mi::base::Interface_declare<0xf237d52c,0xf146,0x40e4,0xb0,0x35,0x99,0xcb,0xc9,0x77,0x64,0x6e>
{
public:
    /// Returns the interface version of the \neurayLibraryName.
    ///
    /// This number changes whenever the abstract interfaces of the \neurayApiName change.
    virtual Uint32 get_interface_version() const = 0;

    /// Returns the product version of the \neurayLibraryName.
    ///
    /// This string contains the product version, build number, build date, etc. of the current
    /// library.
    virtual const char* get_version() const = 0;

    /// Returns an API component from the \neurayApiName.
    ///
    /// \see \ref mi_neuray_api_components for a list of built-in API components.
    ///
    /// \see #register_api_component(), #unregister_api_component()
    ///
    /// \param uuid        The UUID under which the API components was registered. For built-in
    ///                    API components this is the interface ID of the corresponding interface.
    /// \return            A pointer to the API component or \c NULL if the API component is not
    ///                    supported or currently not available.
    virtual base::IInterface* get_api_component( const base::Uuid& uuid) const = 0;

    /// Returns an API component from the \neurayApiName.
    ///
    /// This template variant requires that the API component is registered under the interface ID
    /// of the corresponding interface (which is the case for built-in API components).
    ///
    /// \see \ref mi_neuray_api_components for a list of built-in API components.
    ///
    /// \see #register_api_component(), #unregister_api_component()
    ///
    /// \tparam T          The type of the API components to be queried.
    /// \return            A pointer to the API component or \c NULL if the API component is not
    ///                    supported or currently not available.
    template<class T>
    T* get_api_component() const
    {
        base::IInterface* ptr_iinterface = get_api_component( typename T::IID());
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Registers an API component with the \neurayApiName
    ///
    /// API components are a way for plugins to provide access to their functionality. The
    /// registration makes the API component available for subsequent calls of #get_api_component().
    ///
    /// \param uuid            The ID of the API component to register, e.g., the interface ID of
    ///                        the corresponding interface.
    /// \param api_component   The API component to register.
    /// \return
    ///                        -  0: Success.
    ///                        - -1: Invalid parameters (\c NULL pointer).
    ///                        - -2: There is already an API component registered under the
    ///                              ID \p uuid.
    virtual Sint32 register_api_component(
        const base::Uuid& uuid, base::IInterface* api_component) = 0;

    /// Registers an API component with the \neurayApiName
    ///
    /// API components are a way for plugins to provide access to their functionality. The
    /// registration makes the API component available for subsequent calls of #get_api_component().
    ///
    /// This template variant registers the API component under the interface ID of the
    /// corresponding interface.
    ///
    /// \param api_component   The API component to register.
    /// \return
    ///                        -  0: Success.
    ///                        - -1: Invalid parameters (\c NULL pointer).
    ///                        - -2: There is already an API component registered under the
    ///                              \c ID T::IID().
    template<class T>
    Sint32 register_api_component( T* api_component)
    {
        return register_api_component( typename T::IID(), api_component);
    }

    /// Unregisters an API component with the \neurayApiName
    ///
    /// The API component will no longer be accessible via #get_api_component().
    ///
    /// \param uuid        The ID of the API component to unregister.
    /// \return
    ///                    -  0: Success.
    ///                    - -1: There is no API component registered under the ID \p uuid.
    virtual Sint32 unregister_api_component( const base::Uuid& uuid) = 0;

    /// Unregisters an API component with the \neurayApiName
    ///
    /// The API component will no longer be accessible via #get_api_component().
    ///
    /// This template variant requires that the API component was registered under the interface ID
    /// of the corresponding interface (which is the case for the template variant of
    /// #register_api_component()).
    ///
    /// \return
    ///                    -  0: Success.
    ///                    - -1: There is no API component registered under the ID \c T::IID().
    template<class T>
    Sint32 unregister_api_component()
    {
        return unregister_api_component( typename T::IID());
    }
};

/*@}*/ // end group mi_neuray_plugins

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IPLUGIN_API_H
