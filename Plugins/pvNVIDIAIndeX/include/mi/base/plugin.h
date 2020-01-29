/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/plugin.h
/// \brief Base class for all plugins

#include <mi/base/types.h>
#include <mi/base/interface_declare.h>

#ifndef MI_BASE_PLUGIN_H
#define MI_BASE_PLUGIN_H

namespace mi {

namespace base {

/** \defgroup mi_base_plugin Plugin Support
    \ingroup mi_base

    Support for dynamically loaded plugins.
*/

/** \addtogroup mi_base_plugin
@{
*/

/// The abstract base class for plugins.
///
/// Every plugin must be derived from this class. It provides some virtual functions which can be
/// used to get information about the plugin. The #get_type() function is used to be able to have
/// different classes which are derived from #mi::base::Plugin and provide different interfaces in
/// addition to the interface defined by #mi::base::Plugin.
class Plugin
{
public:
    // The currently used plugin system version.
    static const Sint32 s_version = 3;

    /// Returns the version of the plugin system used to compile this.
    ///
    /// This can be useful when the plugin system is extended/changed at some point to be able to
    /// still support older plugins or at least to reject them. The only thing which must not be
    /// changed is that the first virtual function is this one.
    ///
    /// \return The version number of the plugin system.
    virtual Sint32 get_plugin_system_version() const { return s_version; }

    /// Returns the name of the plugin.
    virtual const char* get_name() const = 0;

    /// Returns the type of the plugin.
    ///
    /// See the documentation of derived interfaces for possible values.
    virtual const char* get_type() const = 0;

    /// Returns the version number of the plugin.
    virtual Sint32 get_version() const { return 1; }

    /// Returns the compiler used to compile the plugin.
    virtual const char* get_compiler() const { return "unknown"; }

    /// Destroys the plugin instance.
    ///
    /// This method should not be confused with #mi::base::IInterface::release() which decrements
    /// the reference count. Plugins are not reference counted and this method here rather destroys
    /// the plugin instance.
    virtual void release() = 0;

    /// Returns a plugin property.
    ///
    /// Plugin properties are represented as key-value pairs. The caller can iterate over all such
    /// existing pairs by calling with indexes starting at 0 and increasing by 1 until the call
    /// returns 0.
    ///
    /// \param index   The index to query.
    /// \param value   The property value for \p index.
    /// \return        The property key for \p index.
    virtual const char* get_string_property(
        Sint32 index,
        const char** value) { (void) index; (void) value; return 0; }

};

/// Represents a plugin.
///
/// This interface is used to represent loaded plugins as reference-counted interfaces since
/// #mi::base::Plugin is \c not reference-counted. In addition, it provides the path from where the
/// plugin was loaded.
class IPlugin_descriptor : public
    mi::base::Interface_declare<0x1708ae5a,0xa49e,0x43c4,0xa3,0x94,0x00,0x38,0x4c,0x59,0xe8,0x67>
{
public:
    /// Returns the plugin itself.
    ///
    /// \note The returned pointer is not referenced-counted. It is only valid as long as the plugin
    ///       descriptor is valid.
    ///
    /// \return   The plugin.
    virtual base::Plugin* get_plugin() const = 0;

    /// Returns the library path of the plugin.
    ///
    /// \return   The library path of the plugin, or \c NULL in case of failure.
    virtual const char* get_plugin_library_path() const = 0;
};

/// Typedef for the initializer function to be provided by every plugin.
///
/// The initializer function is used to obtain an instance of every plugin class. Note that the
/// actual definition of the initializer function needs to be marked as \code extern "C"
/// MI_DLL_EXPORT \endcode
///
/// \param index    The index of the plugin.
/// \param context  The execution context for the plugin.
#ifndef MI_FOR_DOXYGEN_ONLY
typedef Plugin* Plugin_factory (unsigned int /*index*/, void* /*context*/);
#else
typedef Plugin* Plugin_factory (unsigned int index, void* context);
#endif

/*@}*/ // end group mi_base_plugin

} // namespace base

} // namespace mi

#endif // MI_BASE_PLUGIN_H
