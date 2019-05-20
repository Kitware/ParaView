/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Debug settings to configure NVIDIA IndeX.

#ifndef NVIDIA_INDEX_IINDEX_DEBUG_CONFIGURATION_H
#define NVIDIA_INDEX_IINDEX_DEBUG_CONFIGURATION_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

namespace nv
{
namespace index
{
/// Interface to set debug options for the NVIDIA IndeX library.
/// @ingroup nv_index_configuration
class IIndex_debug_configuration : public mi::base::Interface_declare<0x7d1a4588, 0x9458, 0x4459,
                                     0x87, 0x45, 0x1d, 0xa9, 0x03, 0x95, 0x98, 0x24>
{
public:
  /// Sets a debug option.
  /// The option has to be of the form "key=value".
  ///
  /// \param[in] option The option "key=value" to be set.
  /// \return 0: Success. -1: The option could not be successfully
  /// parsed. This happens for example if the option is not of the
  /// form "key=value".
  virtual mi::Sint32 set_option(const char* option) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IINDEX_DEBUG_CONFIGURATION_H
