/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Information about a local or remote host.

#ifndef MI_NEURAYLIB_IHOST_PROPERTIES_H
#define MI_NEURAYLIB_IHOST_PROPERTIES_H

#include <mi/base/interface_declare.h>

namespace mi
{

class IString;

namespace neuraylib
{

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface contains information about a local or remote host.
///
/// \see #mi::neuraylib::IGeneral_configuration, #mi::neuraylib::IHost_callback
class IHost_properties : public mi::base::Interface_declare<0xac76eb08, 0x1c7e, 0x43d7, 0x9e, 0xea,
                           0xec, 0x8d, 0x9a, 0x10, 0x23, 0x80>
{
public:
  /// Returns the address of the host described by this property instance
  ///
  /// The method always returns \c NULL if networking support is not available.
  ///
  /// \return                The address, or \c NULL in case of failure.
  virtual const IString* get_address() const = 0;

  /// Returns the amount of RAM memory in MB.
  virtual Size get_amount_of_memory() const = 0;

  /// Returns the number of CPUs.
  virtual Size get_cpu_count() const = 0;

  /// Returns the number of virtual CPUs in a hyperthreaded CPU.
  virtual Size get_siblings_per_cpu() const = 0;

  /// Returns the number of GPUs.
  virtual Size get_gpu_count() const = 0;

  /// Generic access to properties.
  ///
  /// This method provides access to a string map of additional information. To access it, call
  /// it with the key for the desired value. The current list of supported keys is as follows:
  ///
  /// - \c "admin_http_server_address" \n
  ///   The admin %http server address:port of the host.
  /// - \c "host_name" \n
  ///   The host name of the host.
  /// - \c "memory_size" \n
  ///   The amount of RAM memory in MB on the host.
  /// - \c "cpu_count" \n
  ///   The number of CPUs on the host.
  /// - \c "siblings_per_cpu" \n
  ///   The number of virtual CPUs in a hyperthreaded CPU on the host.
  /// - \c "gpu_count" \n
  ///   The number of GPUs on the host.
  /// - \c "cuda_device_count" \n
  ///   Number of CUDA-capable devices on the host.
  /// - \c "uptime" \n
  ///   The number of seconds since \neurayProductName was started.
  /// - \c "rtmp_port" (*) \n
  ///   The port which the last RTMP server started is listening to.
  ///
  /// (*) These properties are only supported for the local host, not for remote hosts.
  ///
  /// The method always returns \c NULL if networking support is not available.
  ///
  /// \param key             The key for the property wished for
  /// \return                The value of the property, or \c NULL in case of failure.
  virtual const IString* get_property(const char* key) const = 0;

  /// Returns the host ID of this host.
  virtual Uint32 get_host_id() const = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IHOST_PROPERTIES_H
