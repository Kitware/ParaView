/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component that provides access to the database configuration.

#ifndef MI_NEURAYLIB_IDATABASE_CONFIGURATION_H
#define MI_NEURAYLIB_IDATABASE_CONFIGURATION_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface is used to query and change the database configuration.
class IDatabase_configuration : public
    mi::base::Interface_declare<0x8f725100,0xf66c,0x4fa3,0xbf,0x89,0xb5,0xbf,0xea,0x6c,0x4c,0x30>
{
public:
    /// Sets the limits for memory usage of the database.
    ///
    /// The database attempts to keep the total amount of memory used by database elements and jobs
    /// below a configurable limit called \c high \c water \c mark. If that limit is exceeded the
    /// database takes actions to reduce the memory usage until another limit, the \c low \c water
    /// \c mark, is reached (or no further memory reduction is possible). An internal binary format
    /// is used which can be read into the database again very fast. This method can be called at
    /// any time from any thread but no guarantee can be given if and when the new memory limits
    /// will be reached.
    ///
    /// Possible actions taken by the database to reduce memory usage are:
    /// - Discarding results of database jobs.
    /// - Discarding local copies of elements and jobs that are not required (some copies might be
    ///   required to meet the configured redundancy level).
    /// - If disk swapping is enabled, offload database elements to disk.
    ///
    /// \note Memory limits are ignored if memory usage tracking is not enabled (\see
    ///      #set_memory_usage_tracking()).
    ///
    /// \see #get_memory_limits(), #set_disk_swapping(), #get_disk_swapping(),
    ///      #mi::neuraylib::INetwork_configuration::set_redundancy_level(),
    ///      #mi::neuraylib::INetwork_configuration::get_redundancy_level()
    ///
    /// \if DICE_API \see #mi::neuraylib::IElement::get_size(), #mi::neuraylib::IJob::get_size()
    /// \endif
    ///
    /// \param low_water         Flushing stops when memory usage (in bytes) drops below this value.
    /// \param high_water        Flushing starts when memory usage (in bytes) exceeds this value.
    ///                          The value 0 disables flushing completely.
    /// \return                  0 in case of success, -1 in case of failure (\p low_water not less
    ///                          than \p high_water unless \p high_water equals zero).
    virtual Sint32 set_memory_limits( Size low_water, Size high_water) = 0;

    /// Returns the limits for memory usage of the database.
    ///
    /// \see #set_memory_limits(), #set_disk_swapping(), #get_disk_swapping()
    ///
    /// \param[out] low_water    The current low water mark (in bytes).
    /// \param[out] high_water   The current high water mark (in bytes).
    virtual void get_memory_limits( Size* low_water, Size* high_water) const = 0;

    /// Configures the directory for disk swapping.
    ///
    /// If no directory is configured (the default) disk swapping is disabled.
    ///
    /// \note The configured directory must be used exclusively from one instance of this API
    /// component. Sharing the directory between multiple nodes in a network, or between different
    /// instances of this API component on the same node (same or different processes) leads to
    /// wrong behavior.
    ///
    /// \see #get_disk_swapping(), #set_memory_limits(), #get_memory_limits()
    ///
    /// \if DICE_API \see #mi::neuraylib::IElement::get_offload_to_disk() \endif
    ///
    /// \param path              The path to the directory to use for disk swapping.
    /// \return                  0, in case of success, -1 in case of failure.
    virtual Sint32 set_disk_swapping( const char* path) = 0;

    /// Returns the configured directory for disk swapping.
    ///
    /// \see #set_disk_swapping(), #set_memory_limits(), #get_memory_limits()
    ///
    /// \if DICE_API \see #mi::neuraylib::IElement::get_offload_to_disk() \endif
    ///
    /// \return                  The configured directory, or \c NULL if disabled.
    virtual const char* get_disk_swapping() const = 0;

    /// Configures tracking of memory usage of the database.
    ///
    /// Tracking the memory usage of the database incurs a small overhead, and is disabled by
    /// default. It is only needed if memory limits are enabled (\see set_memory_limits()), or for
    /// debugging.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \see #get_memory_usage_tracking()
    ///
    /// \param flag                 \c true to enable, \c false to disable memory usage tracking.
    /// \return                     0, in case of success, -1 in case of failure.
    virtual Sint32 set_memory_usage_tracking( bool flag) = 0;

    /// Indicates whether tracking of memory usage of the database is enabled.
    ///
    /// \see #set_memory_usage_tracking()
    virtual bool get_memory_usage_tracking() const = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IDATABASE_CONFIGURATION_H
