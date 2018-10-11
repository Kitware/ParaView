/* Copyright 2018 NVIDIA Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef vtknvindex_performance_values_h
#define vtknvindex_performance_values_h

#include <fstream>
#include <string>

#include "vtksys/SystemInformation.hxx"

#include <nv/index/iindex.h>

class vtknvindex_application;

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
    vtknvindex_application& application, mi::base::Handle<nv::index::IFrame_results> frame_results);

private:
  vtknvindex_performance_values(const vtknvindex_performance_values&) = delete;
  void operator=(const vtknvindex_performance_values&) = delete;

  // Represent some memory block as string including units.
  std::string to_string(mi::Uint64 memory) const;

  bool m_print_header;
  std::string m_performance_log_file;
};

// General use system information
class VTK_EXPORT vtknvindex_sysinfo
{
public:
  vtknvindex_sysinfo();
  ~vtknvindex_sysinfo();

  vtksys::SystemInformation& get_sysinfo();

  mi::Uint32 get_number_logical_cpu() const;

private:
  vtknvindex_sysinfo(const vtknvindex_sysinfo&) = delete;
  void operator=(const vtknvindex_sysinfo&) = delete;

  vtksys::SystemInformation m_sys_info;
  mi::Uint32 m_nb_logical_cpu;
};
static vtknvindex_sysinfo vtknvindex_sysinfo_instance;
#endif
