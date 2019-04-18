/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for general settings.

#ifndef MI_NEURAYLIB_IGENERAL_CONFIGURATION_H
#define MI_NEURAYLIB_IGENERAL_CONFIGURATION_H

#include <mi/base/interface_declare.h>

namespace mi
{

namespace neuraylib
{

class IHost_properties;

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface is used to query and change the general configuration.
class IGeneral_configuration : public mi::base::Interface_declare<0x4df9426f, 0xaab5, 0x4453, 0xbd,
                                 0x53, 0xc1, 0xfb, 0x0d, 0x82, 0xf9, 0x7f>
{
public:
  /// Sets the address of the administrative HTTP server.
  ///
  /// \NeurayProductName has a built-in administrative HTTP server which can be started. It is
  /// meant to be used to monitor the system at runtime. It allows one to inspect aspects of the
  /// database of \NeurayProductName and thus is useful for debugging integrations. Usually it
  /// would not be enabled in customer builds. By default, the administrative HTTP server is
  /// disabled.
  ///
  /// This can only be configured before \NeurayProductName has been started.
  ///
  /// \param listen_address       The address and port to listen on. \c NULL disables the server.
  /// \return                     0, in case of success, -1 in case of failure.
  virtual Sint32 set_admin_http_address(const char* listen_address) = 0;

  /// Returns the host properties for this host.
  ///
  /// Host properties are only available while \NeurayProductName is running.
  ///
  /// \return                     The host properties for this host, or \c NULL on error.
  virtual const IHost_properties* get_host_properties() const = 0;

  /// Sets a host property for this host.
  ///
  /// The change will be propagated to all other hosts in the cluster.
  /// The method only works if networking support is available.
  ///
  /// \param key              The key for the property.
  /// \param value            The property value.
  /// \return                 0, in case of success, -1 in case of failure.
  virtual Sint32 set_host_property(const char* key, const char* value) = 0;

  /// Sets the path for temporary files.
  ///
  /// This can only be configured before \NeurayProductName has been started.
  ///
  /// \param path                The path to be set.
  /// \return                    0, in case of success, -1 in case of failure.
  virtual Sint32 set_temp_path(const char* path) = 0;

  /// Returns the path for temporary files.
  ///
  /// \return                    The currently configured path.
  virtual const char* get_temp_path() const = 0;

  /// Enables or disables the usage of HTTP proxy for WebSocket. In case of
  /// enabling, this function also sets the HTTP proxy address for WebSocket.
  ///
  /// \param proxy_address       If proxy_address is NULL, the usage of HTTP
  ///                            proxy is disabled. Otherwise, this parameter
  ///                            must contain the proxy address and port for
  ///                            WebSocket to use. In this case, the address
  ///                            must have the format \c "address:port".
  ///                            or \c "host:port".
  /// \return                    0, in case of success, -1 in case of failure.
  virtual Sint32 set_http_proxy_address(const char* proxy_address) = 0;

  /// Gets the HTTP proxy address for WebSocket.
  ///
  /// \return                    The proxy address for WebSocket. If no proxy
  ///                            is used, \c NULL is returned.
  virtual const char* get_http_proxy_address() const = 0;

  /// Sets username and password for authentication with the HTTP proxy address
  /// for WebSocket.
  ///
  /// This method is only needed, if the HTTP proxy requires username and password
  /// for authentication.
  ///
  /// \param username            The username for connecting to the HTTP proxy address.
  ///                            If this is parameter is \c NULL, no authentication
  ///                            is required for connecting to an HTTP proxy.
  /// \param password            The password for connecting to the HTTP proxy address.
  ///                            If no password is needed, \c NULL or an empty
  ///                            string must be provided.
  ///
  /// \return                    0, in case of success, -1 in case of failure.
  virtual Sint32 set_http_proxy_authentication(const char* username, const char* password) = 0;

  /// Gets username for authentication with the HTTP proxy address for WebSocket.
  ///
  ///
  /// \return                    The username for connecting to the HTTP proxy address.
  ///                            If no username was previously provided, \c NULL
  ///                            is returned.
  virtual const char* get_http_proxy_username() const = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IGENERAL_CONFIGURATION_H
