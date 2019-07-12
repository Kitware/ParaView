/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Abstract interface for \neurayApiName plugins.

#ifndef MI_NEURAYLIB_IPLUGIN_H
#define MI_NEURAYLIB_IPLUGIN_H

#include <mi/base/interface_declare.h>
#include <mi/base/plugin.h>

namespace mi {

namespace neuraylib {

class IPlugin_api;

/** \addtogroup mi_neuray_plugins
@{
*/

/// The basic interface to be implemented by \neurayApiName plugins.
///
/// \NeurayApiName plugins need to return #MI_NEURAYLIB_PLUGIN_TYPE in
/// #mi::base::Plugin::get_type().
class IPlugin : public mi::base::Plugin
{
public:
    /// Initializes the plugin.
    ///
    /// \param plugin_api   Provides access to API components available for plugins.
    /// \return             \c true in case of success, and \c false otherwise.
    virtual bool init( IPlugin_api* plugin_api) = 0;

    /// De-initializes the plugin.
    ///
    /// \param plugin_api   Provides access to API components available for plugins.
    /// \return             \c true in case of success, and \c false otherwise.
    virtual bool exit( IPlugin_api* plugin_api) = 0;
};

/*@}*/ // end group mi_neuray_plugins

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_INEURAY_H
