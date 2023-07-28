// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtknvindex_performance_values_h
#define vtknvindex_performance_values_h

#include <string>

#include "vtksys/SystemInformation.hxx"

#include <nv/index/iindex.h>

#include "vtkIndeXRepresentationsModule.h"

// The class vtknvindex_performance_values represents a helper class to collect
// performance values of NVIDIA IndeX renderings and output them to a text file
// along with system/hardware information for offline analysis.

class vtknvindex_performance_values
{
public:
  vtknvindex_performance_values();
  ~vtknvindex_performance_values();

  // Print performance values to a file.
  void print_perf_values(
    mi::base::Handle<nv::index::IFrame_results> frame_results, mi::Uint32 time_step = 0);

private:
  vtknvindex_performance_values(const vtknvindex_performance_values&) = delete;
  void operator=(const vtknvindex_performance_values&) = delete;

  // Represent some memory block as string including units.
  std::string to_string(mi::Uint64 memory) const;

  bool m_print_header;
  std::string m_last_log_file;
};

// General use system information
class vtknvindex_sysinfo
{
public:
  vtknvindex_sysinfo();
  ~vtknvindex_sysinfo();

  static vtknvindex_sysinfo* create_sysinfo();
  static vtknvindex_sysinfo* get_sysinfo();

  mi::Uint32 get_number_logical_cpu() const;

private:
  vtknvindex_sysinfo(const vtknvindex_sysinfo&) = delete;
  void operator=(const vtknvindex_sysinfo&) = delete;

  static vtknvindex_sysinfo* s_vtknvindex_sysinfo;
  mi::Uint32 m_nb_logical_cpu;
};
#endif
