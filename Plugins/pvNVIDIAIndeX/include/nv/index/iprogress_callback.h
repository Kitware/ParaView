/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for implementing progress callbacks.

#ifndef NVIDIA_INDEX_IPROGRESS_CALLBACK_H
#define NVIDIA_INDEX_IPROGRESS_CALLBACK_H

#include <mi/base/interface_declare.h>
#include <mi/base/types.h>
#include <mi/dice.h>

namespace nv
{
namespace index
{

/// The interface class allows implementing user-defined progress callbacks
/// used for querying the progress of the distributed rendering or
/// computing task. The progress of a tasks, e.g., the uploading and
/// rendering of the data, is measured in percent in the
/// range from 0 to 100.
///
/// The index library's render call provides means to register a progress
/// callback that monitors the progress for uploading and rendering the
/// data in the cluster.
///
/// Compute tasks may benefit from the present interface class as well but
/// need to implement their own means for monitoring the progress of the
/// user-defined process.
///
/// @ingroup nv_index_utilities
class IProgress_callback : public mi::base::Interface_declare<0x93d1b285, 0x5e1c, 0x411f, 0x9f,
                             0x63, 0x8c, 0x2a, 0x72, 0xbf, 0xdc, 0xf5>
{
public:
  /// The updates to the progress will be passed to the present
  /// interface method (i.e., callback) to be processed. A user-defined
  /// implementation of the interface class may, for instance, update
  /// the user-interface to ensure user response on the progress.
  ///
  /// \param[in] progress_values The progress is updated in percent with
  ///                         values in the range [0, 100]. If the task is
  ///                         completed then 100% will be passed to the
  ///                         interface method.
  ///
  virtual void set_progress(mi::Float32 progress_values) = 0;

  /// The current progress of a tasks given in percent can be queried at any time.
  ///
  /// \return current         The current progress returned in the range
  ///                         from 0 to 100.
  ///
  virtual mi::Float32 get_progress() const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IPROGRESS_CALLBACK_H
