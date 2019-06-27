/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Interface to inquire network statistics.

#ifndef MI_NEURAYLIB_INETWORK_STATISTICS_H
#define MI_NEURAYLIB_INETWORK_STATISTICS_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface is used to inquire statistics about the network usage etc.
class INetwork_statistics : public
    mi::base::Interface_declare<0xf3706973,0x2dbf,0x45ff,0xb1,0x99,0xcf,0x66,0x09,0x41,0x5e,0xc0>
{
public:
    /// Returns the calculated send bandwidth towards a given host.
    ///
    /// \param host_id    The host in question, or 0 for multicast.
    /// \return           The calculated send bandwidth in bits per second.
    virtual Uint64 get_calculated_send_bandwidth( Uint32 host_id = 0) const = 0;

    /// Returns the actual send bandwidth towards a given host.
    ///
    /// \param host_id    The host in question, or 0 for multicast.
    /// \return           The actual send bandwidth in bits per second.
    virtual Uint64 get_current_send_bandwidth( Uint32 host_id = 0) const = 0;

    /// Returns the current loss rate towards a given host.
    ///
    /// \param host_id    The host in question, or 0 for multicast.
    /// \return           The current loss rate (ratio between lost and all packets).
    virtual Float64 get_loss_rate( Uint32 host_id = 0) const = 0;

    /// Returns the number of packets for which a NACK was received for this host.
    ///
    /// \param host_id    The host in question, or 0 for multicast.
    /// \return           The number of packets for which a NACK was received.
    virtual Uint64 get_nr_of_nacked_packets( Uint32 host_id = 0) const = 0;

    /// Returns the last time a given host was seen.
    ///
    /// \param host_id    The host in question.
    /// \return           The time that passed since the host was seen last (in seconds).
    virtual Float64 get_last_seen( Uint32 host_id) const = 0;

    /// Returns the total number of bytes received so far.
    virtual Uint64 get_received_bytes() const = 0;

    /// Returns the total number of bytes sent so far.
    virtual Uint64 get_sent_bytes() const = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_INETWORK_STATISTICS_H
