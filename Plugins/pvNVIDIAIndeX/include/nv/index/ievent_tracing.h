/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file ievent_tracing.h
/// \brief API for recording and reading tracing events.
///
/// \ingroup nv_index_performance_measurement
///
/// The IndeX event tracing API is suitable for logging performance values or
/// monitoring. Besides logging individual values, namespaces exist to provide
/// groupding for individual events.
///
/// Event tracing works in 2 steps:
///     - Recording phase: record an event of format <namespace, namespace id, event name, event value>
///     - Collection phase: all events are collected once per render call inside
///       the library. The application is given the trace list through the registered
///       callback (see \c ITrace_collection_handler).
///
/// \attention The strings given to the record calls will be accessed during the
///            collection phase as well, so they should have a longer lifetime.
///            It is strongly recommended to use string literals:
/// \code
/// // Usage:
/// event_tracing->record("mynamespace", 33, "my_event", 200);
/// \endcode

#ifndef NVIDIA_INDEX_ITRACE_EVENTS_H
#define NVIDIA_INDEX_ITRACE_EVENTS_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>
#include <mi/base/types.h>

namespace nv {
namespace index {

/// Value types for trace events.
/// \ingroup nv_index_performance_measurement
enum ITrace_event_type
{
    NONE            = 0x00,
    VALUE_U64       = 0x01, ///< Uint64 value
    VALUE_S64       = 0x02, ///< Sint64 value
    VALUE_F64       = 0x03, ///< Float64 value
    VALUE_DURATION  = 0x04, ///< Time duration value
    NEW_NS          = 0x06, ///< Namespace scope start
    CLEAR_NS        = 0x07  ///< Namespace scope end
};

/// Iterator for accessing individual trace events.
/// \ingroup nv_index_performance_measurement
class ITrace_event_iterator
    : public mi::base::Interface_declare<0x55d30b9a,0x4927,0x4150,0x90,0x63,0xd6,0xd6,0x8a,0x40,0xbf,0xf0>
{
public:

    /// Move iterator to next event.
    virtual void next() = 0;

    /// Is the iterator pointing to the beginning?
    virtual bool begin() = 0;

    /// Has the iterator reached the end of the trace collection?
    virtual bool end() = 0;

    /// Namespace name for this trace event.
    virtual const char* get_namespace() const = 0;

    /// Namespace id for this trace event.
    virtual mi::Sint32 get_namespace_id() const = 0;

    /// Name for trace event.
    virtual const char* get_name() const = 0;

    /// Timestamp of event recording.
    virtual mi::Uint64 get_timestamp() const = 0;

    /// Host id for where this event was collected.
    virtual mi::Uint32 get_host_id() const = 0;

    /// Type of event.
    virtual ITrace_event_type get_type() const = 0;

    /// Get event value as Uint64.
    virtual void get_value(mi::Uint64& value) const = 0;

    /// Get event value as Sint64.
    virtual void get_value(mi::Sint64& value) const = 0;

    /// Get event value as Float64.
    virtual void get_value(mi::Float64& value) const = 0;
};

/// \ingroup nv_index_performance_measurement

/// Interface for reading traces from a collection.
class ITrace_collection
    : public mi::base::Interface_declare<0xe05ed2f7,0x71da,0x4211,0x87,0x1b,0x46,0xfd,0x3f,0xb8,0x8a,0xbd>
{
public:

    /// Collection id.
    virtual mi::Size get_collection_id() const = 0;

    /// Time when collection happened.
    virtual mi::Uint64 get_collection_timestamp() const = 0;

    /// Host id where collection happened.
    virtual mi::Uint32 get_local_host_id() const = 0;

    /// Number of trace events in this collection.
    virtual mi::Size get_nb_traces() const = 0;

    /// Get iterator for accessing individual trace events.
    virtual ITrace_event_iterator* get_trace_iterator() const = 0;
};

/// Callback interface for handling a trace collection.
/// \ingroup nv_index_performance_measurement
class ITrace_collection_handler
    : public mi::base::Interface_declare<0xb9b64b1b,0x8349,0x4099,0xbe,0x2b,0x86,0x5f,0xca,0x04,0xe0,0x92>
{
public:
    /// Handle a trace collection.
    virtual void handle(ITrace_collection* collection) = 0;
};

/// Main class that handles trace event collection and recording.
/// \ingroup nv_index_performance_measurement
class IEvent_tracing
    : public mi::base::Interface_declare<0x5c604cf0,0xb4ac,0x4c19,0xb8,0xf8,0x2d,0x1c,0xeb,0xa6,0x9c,0xff>
{
public:

    /// Add a notification handler for the availability of trace collection.
    ///
    /// \param[in]  handler     Pointer to the notification handler.
    virtual void add_collection_handler(ITrace_collection_handler* handler) = 0;

    /// Removes a notification handler for the availability of trace collection.
    ///
    /// \param[in]  handler     Pointer to the notification handler.
    virtual void remove_collection_handler(ITrace_collection_handler* handler) = 0;

    /// Get a specific collection id.
    ///
    /// \param[in] collection_id The id of the collection. It currently maps to the frame id.
    /// \param[in] raw           Provide raw trace events (triggers collection from all hosts).
    ///
    /// \return                  Returns a trace collection which could be empty.
    virtual ITrace_collection* get_collection(mi::Uint32 collection_id, bool raw = false) = 0;

    /// Record a trace tuple of format <namespace, namespace_id, trace_name, trace_value>
    /// for Uint64.
    ///
    /// \param[in] ns     Namespace
    /// \param[in] ns_id  Namespace id
    /// \param[in] name   Event name
    /// \param[in] value  Event value.
    ///
    /// \note For \c ns and \c name please use string literals.
    virtual void record(const char* ns, mi::Sint32 ns_id, const char* name, mi::Uint64 value) = 0;

    /// Record a trace tuple of format <namespace, namespace_id, trace_name, trace_value>
    /// for Sint64.
    ///
    /// \param[in] ns     Namespace
    /// \param[in] ns_id  Namespace id
    /// \param[in] name   Event name
    /// \param[in] value  Event value.
    ///
    /// \note For \c ns and \c name please use string literals.
    virtual void record(const char* ns, mi::Sint32 ns_id, const char* name, mi::Sint64 value) = 0;

    /// Record a trace tuple of format <namespace, namespace_id, trace_name, trace_value>
    /// for Float64.
    ///
    /// \param[in] ns     Namespace
    /// \param[in] ns_id  Namespace id
    /// \param[in] name   Event name
    /// \param[in] value  Event value.
    ///
    /// \note For \c ns and \c name please use string literals.
    virtual void record(const char* ns, mi::Sint32 ns_id, const char* name, mi::Float64 value) = 0;

    /// Record a time duration trace of format <namespace, namespace_id, trace_name, start_timestamp, duration>
    ///
    /// \param[in] ns               Namespace
    /// \param[in] ns_id            Namespace id
    /// \param[in] name             Event name
    /// \param[in] start_timestamp  Start timestamp (nanoseconds).
    /// \param[in] duration         Duration of event (nanoseconds).
    ///
    /// \note For \c ns and \c name please use string literals.
    virtual void record_time_duration(const char* ns,
                                      mi::Sint32 ns_id,
                                      const char* name,
                                      mi::Uint64 start_timestamp,
                                      mi::Uint64 duration) = 0;

    /// Record a time interval trace of format <namespace, namespace_id, trace_name, start_timestamp, end_timestamp>
    ///
    /// \param[in] ns               Namespace
    /// \param[in] ns_id            Namespace id
    /// \param[in] name             Event name
    /// \param[in] start_timestamp  Start timestamp (nanoseconds).
    /// \param[in] end_timestamp    End timestamp (nanoseconds.
    ///
    /// \note For \c ns and \c name please use string literals.
    virtual void record_time_interval(const char* ns,
                                      mi::Sint32 ns_id,
                                      const char* name,
                                      mi::Uint64 start_timestamp,
                                      mi::Uint64 end_timestamp) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ITRACE_EVENTS_H
