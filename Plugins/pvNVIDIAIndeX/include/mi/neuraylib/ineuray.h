/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Main \NeurayApiName interface.

#ifndef MI_NEURAYLIB_INEURAY_H
#define MI_NEURAYLIB_INEURAY_H

#include <mi/base/interface_declare.h>

// X11/Xlib.h defines Status to int
#if defined(_XLIB_H_) || defined(_X11_XLIB_H_)
#undef Status
#endif // _XLIB_H_ || _X11_XLIB_H_

namespace mi
{

namespace neuraylib
{

/** \defgroup mi_neuray_ineuray Main \neurayAdjectiveName Interface and C access function
    \ingroup mi_neuray

    The main \neurayAdjectiveName Interface and the unique public access point.
*/

/** \defgroup mi_neuray_configuration Configuration Interfaces
    \ingroup mi_neuray

    This module encompasses the \link mi_neuray_api_components API components \endlink used to
    configure the \neurayApiName as well as closely related interfaces. API components can be
    obtained from #mi::neuraylib::INeuray::get_api_component() \ifnot MDL_SDK_API or from
    #mi::neuraylib::IPlugin_api::get_api_component().\endif
*/

/** \addtogroup mi_neuray_ineuray
@{
*/

/// This is an object representing the \neurayLibraryName. Only one object of this type will exist
/// at a time. It is used for configuration, startup and shutdown of the \neurayLibraryName.
class INeuray : public mi::base::Interface_declare<0xafdd621e, 0x2918, 0x41f9, 0xae, 0x3d, 0xec,
                  0x36, 0x63, 0x63, 0x86, 0x7a>
{
public:
  /// Returns the interface version of the \neurayLibraryName.
  ///
  /// This number changes whenever the abstract interfaces of the \neurayApiName changes.
  virtual Uint32 get_interface_version() const = 0;

  /// Returns the version of the \neurayLibraryName.
  ///
  /// This string contains the product version, build number, build date, and platform of the
  /// current library.
  virtual const char* get_version() const = 0;

  // Startup and shutdown

  /// Starts the operation of the \neurayLibraryName.
  ///
  /// All configuration which is marked to be done before the start of the library must be done
  /// before calling this function. When calling this function \neurayProductName will start
  /// threads and start network operations etc. The \neurayLibraryName may not be ready for
  /// operation after the call returned if blocking mode is not used.
  ///
  /// \if IRAY_API
  /// \note Starting the \neurayLibraryName multiple times, i.e., calling #shutdown() then calling
  ///       #start() again, is not yet supported. This is true even if the first call to
  ///       #start() fails.
  /// \endif
  ///
  /// \param blocking    \if MDL_SDK_API Unused. The startup is always done in blocking mode.
  ///                    \else Indicates whether the startup should be done in blocking mode. If
  ///                    \c true the method will not return before all initialization was done. If
  ///                    \c false the method will return immediately and the startup is done in a
  ///                    separate thread. The status of the startup sequence can be checked via
  ///                    #get_status(). \endif
  /// \return
  ///                    -  0: Success
  ///                    - -1: Unspecified failure.
  ///                    - -2: Authentication failure (challenge-response).
  ///                    - -3: Authentication failure (SPM).
  ///                    - -4: Provided license expired.
  ///                    - -5: No professional GPU as required by the license in use was found.
  ///                    - -6: Authentication failure (FLEXlm).
  ///                    - -7: No NVIDIA VCA as required by the license in use was found.
  virtual Sint32 start(bool blocking = true) = 0;

  /// The operational status of the library \if DICE_API or additional clusters \endif.
  ///
  /// \if DICE_API \see #mi::neuraylib::ICluster \endif
  enum Status
  {
    /// The library or the cluster has not yet been started.
    PRE_STARTING = 0,
    /// The library or the cluster is starting.
    STARTING = 1,
    /// The library or the cluster is ready for operation.
    STARTED = 2,
    /// The library or the cluster is shutting down.
    SHUTTINGDOWN = 3,
    /// The library or the cluster has been shut down.
    SHUTDOWN = 4,
    /// There was a failure during operation.
    FAILURE = 5,
    //  Undocumented, for alignment only.
    FORCE_32_BIT = 0xffffffffU
  };

  /// Shuts down the library.
  ///
  /// For proper shutdown this may only be called after all transactions have been committed and
  /// all rendering is finished.
  ///
  /// You also need to release all interface pointers related to functionality obtained after
  /// startup before calling this method. In case you use the #mi::base::Handle class (or another
  /// handle class), you need to make sure that all such handles have been reset or destroyed.
  ///
  /// \if IRAY_API
  /// \note Starting the \neurayLibraryName multiple times, i.e., calling #shutdown() then calling
  ///       #start() again, is not yet supported. This is true even if the first call to
  ///       #start() fails.
  /// \endif
  ///
  /// \param blocking    \if MDL_SDK_API Unused. The shutdown is always done in blocking mode.
  ///                    \else Indicates whether the shutdown should be done in blocking mode. If
  ///                    \c true the method will not return before shutdown has completed.
  ///                    If \c false the method will return immediately and the shutdown is done
  ///                    in a separate thread. The status of the shutdown sequence can be checked
  ///                    via #get_status(). \endif
  /// \return            0, in case of success, -1 in case of failure
  virtual Sint32 shutdown(bool blocking = true) = 0;

  /// Returns the status of the library.
  ///
  /// \return                        The status
  virtual Status get_status() const = 0;

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
  virtual base::IInterface* get_api_component(const base::Uuid& uuid) const = 0;

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
  template <class T>
  T* get_api_component() const
  {
    base::IInterface* ptr_iinterface = get_api_component(typename T::IID());
    if (!ptr_iinterface)
      return 0;
    T* ptr_T = static_cast<T*>(ptr_iinterface->get_interface(typename T::IID()));
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
  template <class T>
  Sint32 register_api_component(T* api_component)
  {
    return register_api_component(typename T::IID(), api_component);
  }

  /// Unregisters an API component with the \neurayApiName
  ///
  /// The API component will no longer be accessible via #get_api_component().
  ///
  /// \param uuid        The ID of the API component to unregister.
  /// \return
  ///                    -  0: Success.
  ///                    - -1: There is no API component registered under the ID \p uuid.
  virtual Sint32 unregister_api_component(const base::Uuid& uuid) = 0;

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
  template <class T>
  Sint32 unregister_api_component()
  {
    return unregister_api_component(typename T::IID());
  }
};

mi_static_assert(sizeof(INeuray::Status) == sizeof(Uint32));

/*@}*/ // end group mi_neuray_ineuray

} // namespace neuraylib

} // namespace mi

// X11/Xlib.h defines Status to int
#if defined(_XLIB_H_) || defined(_X11_XLIB_H_)
#define Status int
#endif // _XLIB_H_ || _X11_XLIB_H_

#endif // MI_NEURAYLIB_INEURAY_H
