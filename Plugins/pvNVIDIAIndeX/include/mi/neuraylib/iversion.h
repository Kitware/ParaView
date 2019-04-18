/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Interface for accessing version information.

#ifndef MI_NEURAYLIB_IVERSION_H
#define MI_NEURAYLIB_IVERSION_H

#include <mi/base/interface_declare.h>

namespace mi
{

namespace neuraylib
{

/// Abstract interface for accessing version information.
class IVersion : public mi::base::Interface_declare<0xe8f929df, 0x6c1e, 0x4ed5, 0xa6, 0x17, 0x29,
                   0xa6, 0xb, 0x12, 0xdb, 0x48>
{
public:
  /// Returns the product name.
  virtual const char* get_product_name() const = 0;

  /// Returns the product version.
  virtual const char* get_product_version() const = 0;

  /// Returns the build number.
  virtual const char* get_build_number() const = 0;

  /// Returns the build date.
  virtual const char* get_build_date() const = 0;

  /// Returns the platform the library was built on.
  virtual const char* get_build_platform() const = 0;

  /// Returns the full version string.
  virtual const char* get_string() const = 0;

  /// Returns the neuray interface id.
  virtual base::Uuid get_neuray_iid() const = 0;
};

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IVERSION_H
