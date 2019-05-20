/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief  API for reading tracing events.
///
#ifndef NVIDIA_INDEX_ITRACE_EVENTS_H
#define NVIDIA_INDEX_ITRACE_EVENTS_H

#include <mi/base/interface_declare.h>
#include <mi/base/types.h>
#include <mi/dice.h>

namespace nv
{
namespace index
{

enum ITrace_event_type
{
  NONE = 0,
  VALUE_U64,
  VALUE_S64,
  VALUE_F64,
  VALUE_DURATION,
  NEW_NS,
  CLEAR_NS
};

class ITrace_event_iterator : public mi::base::Interface_declare<0x55d30b9a, 0x4927, 0x4150, 0x90,
                                0x63, 0xd6, 0xd6, 0x8a, 0x40, 0xbf, 0xf0>
{
public:
  virtual void next() = 0;
  virtual bool begin() = 0;
  virtual bool end() = 0;

  virtual const char* get_namespace() const = 0;
  virtual mi::Sint32 get_namespace_id() const = 0;
  virtual const char* get_name() const = 0;
  virtual mi::Uint64 get_timestamp() const = 0;
  virtual mi::Uint32 get_host_id() const = 0;

  virtual ITrace_event_type get_type() const = 0;

  virtual void get_value(mi::Uint64& value) const = 0;
  virtual void get_value(mi::Sint64& value) const = 0;
  virtual void get_value(mi::Float64& value) const = 0;
};

class ITrace_collection : public mi::base::Interface_declare<0xe05ed2f7, 0x71da, 0x4211, 0x87, 0x1b,
                            0x46, 0xfd, 0x3f, 0xb8, 0x8a, 0xbd>
{
public:
  virtual mi::Size get_collection_id() const = 0;

  virtual mi::Uint64 get_collection_timestamp() const = 0;

  virtual mi::Uint32 get_local_host_id() const = 0;

  virtual mi::Size get_nb_traces() const = 0;

  virtual ITrace_event_iterator* get_trace_iterator() const = 0;
};

class ITrace_collection_handler : public mi::base::Interface_declare<0xb9b64b1b, 0x8349, 0x4099,
                                    0xbe, 0x2b, 0x86, 0x5f, 0xca, 0x04, 0xe0, 0x92>
{
public:
  virtual void handle(ITrace_collection* collection) = 0;
};

class IEvent_tracing : public mi::base::Interface_declare<0x5c604cf0, 0xb4ac, 0x4c19, 0xb8, 0xf8,
                         0x2d, 0x1c, 0xeb, 0xa6, 0x9c, 0xff>
{
public:
  /// Add a notification handler for the availability of trace collection.
  ///
  /// \param[in]  handler     Pointer to the notification handler.
  virtual void add_collection_handler(ITrace_collection_handler* handler) = 0;

  /// Get a specific collection id.
  ///
  /// \param[in] collection_id The if of the collection. It currently maps to the frame id.
  ///
  /// \return
  virtual ITrace_collection* get_collection(mi::Uint32 collection_id, bool raw = false) = 0;

  virtual void record(const char* ns, mi::Sint32 ns_id, const char* name, mi::Uint64 value) = 0;
  virtual void record(const char* ns, mi::Sint32 ns_id, const char* name, mi::Sint64 value) = 0;
  virtual void record(const char* ns, mi::Sint32 ns_id, const char* name, mi::Float64 value) = 0;
  virtual void record_time_duration(const char* ns, mi::Sint32 ns_id, const char* name,
    mi::Uint64 start_timestamp, mi::Uint64 duration) = 0;
  virtual void record_time_interval(const char* ns, mi::Sint32 ns_id, const char* name,
    mi::Uint64 start_timestamp, mi::Uint64 end_timestamp) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ITRACE_EVENTS_H
