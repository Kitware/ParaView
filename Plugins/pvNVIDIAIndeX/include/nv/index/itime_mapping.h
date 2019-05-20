/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Base class representing clock pulses and time mapping vor time varying data.

#ifndef NVIDIA_INDEX_ITIME_MAPPING_H
#define NVIDIA_INDEX_ITIME_MAPPING_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <mi/base/types.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// The interface class \c IClock_pulse_generator generates the time for playing animations.
///
/// The user has the ability to specify and implement a custom clock pulse generator to steer the
/// time-dependent visualization of large-scale data.
///
class IClock_pulse_generator : public mi::base::Interface_declare<0xaaf6c278, 0xf36f, 0x4d00, 0x83,
                                 0xd6, 0xff, 0x9e, 0x4a, 0xb3, 0x2d, 0x57>
{
public:
  /// The point in time.
  ///
  /// \return     Returns the absolute point in time defined within the set interval.
  ///
  virtual mi::Float64 get_tick() = 0;

  /// The start point of the time internal in which a time-dependent visualization is defined.
  ///
  /// \return     Returns the start time of the time interval.
  ///
  virtual mi::Float64 get_start() const = 0;

  /// The end point of the time internal in which a time-dependent visualization is defined.
  ///
  /// \return     Returns the end time of the time interval.
  ///
  virtual mi::Float64 get_end() const = 0;
};

/// @ingroup nv_index_scene_description_attribute

/// The interface class \c ITime_step_assignment maps absolute time to time steps (or key frames).
///
/// The attribute defines how many time steps shall be imported for a large-scale dataset in the
/// scene.
/// The attribute also allows deriving various/custom mapping techniques.
///
class ITime_step_assignment : public mi::base::Interface_declare<0x9202feb1, 0x4c0f, 0x442a, 0xfd,
                                0x34, 0x4d, 0x1b, 0x30, 0x3d, 0xa1, 0xad, nv::index::IAttribute>
{
public:
  /// The number of time steps (or key frames) a distributed large-scale dataset shall provide.
  ///
  /// \return     Returns the number of time steps.
  ///
  virtual mi::Uint64 get_nb_time_steps() const = 0;
  virtual void set_nb_time_steps(mi::Uint64) = 0;

  /// Maps a absolute point in time to a time step.
  ///
  /// \param[in] current  The absolute point in time that maps to a time stamp.
  /// \param[in] start    The start time of the global time interval.
  /// \param[in] end      The end time of the global time interval.
  ///
  /// \return             Returns the time step for the given time stamp.
  ///
  virtual mi::Uint64 get_time_step(
    mi::Float64 current, mi::Float64 start, mi::Float64 end) const = 0;

  /// Experimental, to be moved.
  ///
  /// \param[in] t0       The beginning of the time interval.
  ///
  /// \param[in] t1       The end of the time interval.
  ///
  virtual void set_time_interval(mi::Float64 t0, mi::Float64 t1) = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ITIME_MAPPING_H
