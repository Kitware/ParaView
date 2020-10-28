/* Copyright 2020 NVIDIA Corporation. All rights reserved.
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

#include <iomanip>
#include <iostream>

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_instance.h"
#include "vtknvindex_performance_values.h"

#include <nv/index/version.h>

//-------------------------------------------------------------------------------------------------
vtknvindex_performance_values::vtknvindex_performance_values()
  : m_print_header(true)
  , m_performance_log_file("nvindex_perf_log.txt")
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_performance_values::~vtknvindex_performance_values()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_performance_values::print_perf_values(
  mi::base::Handle<nv::index::IFrame_results> frame_results, mi::Uint32 time_step)
{
  // Prints only a subset of the full performance values that NVIDIA IndeX provides.
  // Additional fields can be added as and when required.

  vtksys::ofstream file_handler;
  file_handler.open(m_performance_log_file.c_str(), std::ios::out | std::ios::app);
  std::ostringstream s;
  const std::streamsize ssize_time(10);

  if (m_print_header)
  {
    std::vector<std::string> path_components;
    path_components.push_back(vtksys::SystemTools::GetCurrentWorkingDirectory() + "/");
    path_components.push_back(m_performance_log_file);
    INFO_LOG << "Writing performance values to '" << vtksys::SystemTools::JoinPath(path_components)
             << "'";

    std::ostringstream header_str;

    // Prepare header report with current time, OS, SW/HW info.
    vtksys::SystemInformation sys_info;
    sys_info.RunOSCheck();
    sys_info.RunCPUCheck();

    vtknvindex_instance* index_instance = vtknvindex_instance::get();

    header_str << "--------------------------------------------------------------------------------"
                  "--------------------------------------------------------------------------------"
               << "\n";
    header_str << "IndeX Performance Values: " << vtksys::SystemTools::GetCurrentDateTime("%c")
               << "\n";
    header_str << "--------------------------------------------------------------------------------"
                  "--------------------------------------------------------------------------------"
               << "\n";

    header_str << "Host name        : " << sys_info.GetHostname() << "\n";
    header_str << "OS               : " << sys_info.GetOSDescription() << "\n";
    header_str << "CPU Description  : " << sys_info.GetCPUDescription() << "\n";
    header_str << "Number of hosts  : "
               << index_instance->m_icluster_configuration->get_number_of_hosts() << "\n";
    header_str << "Number of GPUs   : "
               << index_instance->m_icluster_configuration->get_number_of_GPUs() << "\n";

    header_str << "--------------------------------------------------------------------------------"
                  "--------------------------------------------------------------------------------"
               << "\n";

    header_str << "NVIDIA IndeX ParaView plugin version : " << index_instance->get_version()
               << "\n";

    const mi::base::Handle<nv::index::IIndex>& index = index_instance->get_interface();

    header_str << "NVIDIA IndeX header file version:     : "
               << NVIDIA_INDEX_LIBRARY_VERSION_QUALIFIED_STRING << ", "
               << NVIDIA_INDEX_LIBRARY_REVISION_STRING << "\n";
    header_str << "NVIDIA IndeX library build version    : " << index->get_version() << ", "
               << index->get_revision() << "\n";
    header_str << "DiCE library API interface version    : " << index->get_dice_interface_version()
               << "\n";
    header_str << "DiCE header  API version              : "
               << MI_NEURAYLIB_VERSION_QUALIFIED_STRING << "\n";
    header_str << "DiCE library build version            : " << index->get_dice_version() << "\n";
    header_str << "NVIDIA driver version                 : " << index->get_nvidia_driver_version()
               << "\n";

    header_str << "--------------------------------------------------------------------------------"
                  "--------------------------------------------------------------------------------"
               << "\n";
    header_str << "TSTEP    : Time step number"
               << "\n";
    header_str << "T_REND   : Time total rendering [ms]"
               << "\n";
    header_str << "T_COMP   : Time total compositing [ms]"
               << "\n";
    header_str << "T_FCOMP  : Time total final compositing [ms]"
               << "\n";
    header_str << "T_FRAME  : Time complete frame [ms]"
               << "\n";
    header_str << "FPS      : Frames per second"
               << "\n";
    header_str << "N_SCUBES : Number sub cubes rendered"
               << "\n";
    header_str << "S_VOLDAT : Size volume data rendered"
               << "\n";
    header_str << "T_GPU_UP : Time GPU upload [ms]"
               << "\n";
    header_str << "T_GPU_DN : Time GPU download [ms]"
               << "\n";
    header_str << "S_VOL_UP : Size volume data upload"
               << "\n";
    header_str << "S_PIN_M  : Size pinned host memory"
               << "\n";
    header_str << "S_UPIN_M : Size unpinned host memory"
               << "\n";
    header_str << "S_SYS_M  : Size system memory usage"
               << "\n";
    header_str << "S_GPU_M  : Size GPU memory used"
               << "\n";
    header_str << "N_SPANS  : Number horizontal spans"
               << "\n";
    header_str << "--------------------------------------------------------------------------------"
                  "--------------------------------------------------------------------------------"
               << "\n";

    header_str << std::setw(5) << "TSTEP" << std::setw(ssize_time) << "T_REND"
               << std::setw(ssize_time) << "T_COMP" << std::setw(ssize_time) << "T_FCOMP"
               << std::setw(ssize_time) << "T_FRAME" << std::setw(ssize_time) << "FPS"
               << std::setw(ssize_time) << "NSCUBES" << std::setw(ssize_time) << "S_VOLDAT"
               << std::setw(ssize_time) << "T_GPU_UP" << std::setw(ssize_time) << "T_GPU_DN"
               << std::setw(ssize_time) << "S_VOL_UP" << std::setw(ssize_time) << "S_PIN_M"
               << std::setw(ssize_time) << "S_UPIN_M" << std::setw(ssize_time) << "S_SYS_M"
               << std::setw(ssize_time) << "S_GPU_M" << std::setw(ssize_time) << "N_SPANS"
               << "\n";

    file_handler << header_str.str();
    m_print_header = false;
  }
  const mi::base::Handle<nv::index::IPerformance_values> perf_values(
    frame_results->get_performance_values());
  s << std::setw(5) << time_step << std::setw(ssize_time)
    << perf_values->get_time("time_total_rendering") << std::setw(ssize_time)
    << perf_values->get_time("time_total_compositing") << std::setw(ssize_time)
    << perf_values->get_time("time_total_final_compositing") << std::setw(ssize_time)
    << perf_values->get_time("time_complete_frame") << std::setw(ssize_time)
    << perf_values->get_time("frames_per_second") << std::setw(ssize_time)
    << perf_values->get("nb_subcubes_rendered") << std::setw(ssize_time)
    << to_string(perf_values->get("size_volume_data_rendered")) << std::setw(ssize_time)
    << perf_values->get_time("time_gpu_upload") << std::setw(ssize_time)
    << perf_values->get_time("time_gpu_download") << std::setw(ssize_time)
    << to_string(perf_values->get("size_volume_data_upload")) << std::setw(ssize_time)
    << to_string(perf_values->get("size_pinned_host_memory")) << std::setw(ssize_time)
    << to_string(perf_values->get("size_unpinned_host_memory")) << std::setw(ssize_time)
    << to_string(perf_values->get("size_system_memory_usage")) << std::setw(ssize_time)
    << to_string(perf_values->get("size_gpu_memory_used")) << std::setw(ssize_time)
    << perf_values->get("nb_horizontal_spans") << "\n";

  file_handler << s.str();
  file_handler.close();
}

//-------------------------------------------------------------------------------------------------
std::string vtknvindex_performance_values::to_string(mi::Uint64 memory) const
{
  std::ostringstream number_str;
  const mi::Uint64 K = 1024u;

  if (memory < K)
    number_str << memory << " b";
  else if (memory < K * K)
    number_str << std::setprecision(4) << memory / (K * 1.0f) << " Kb";
  else if (memory < K * K * K)
    number_str << std::setprecision(4) << memory / ((K * K) * 1.0f) << " Mb";
  else if (memory < K * K * K * K)
    number_str << std::setprecision(4) << memory / ((K * K * K) * 1.0f) << " Gb";
  else
    number_str << std::setprecision(4) << memory / ((K * K * K * K) * 1.0f) << " Tb";

  return number_str.str();
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sysinfo::vtknvindex_sysinfo()
{
  vtksys::SystemInformation sys_info;
  sys_info.RunOSCheck();
  sys_info.RunCPUCheck();
  m_nb_logical_cpu = sys_info.GetNumberOfLogicalCPU();
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sysinfo::~vtknvindex_sysinfo()
{
  delete s_vtknvindex_sysinfo;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sysinfo* vtknvindex_sysinfo::create_sysinfo()
{
  return new vtknvindex_sysinfo();
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sysinfo* vtknvindex_sysinfo::get_sysinfo()
{
  return s_vtknvindex_sysinfo;
}

//-------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_sysinfo::get_number_logical_cpu() const
{
  return m_nb_logical_cpu;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sysinfo* vtknvindex_sysinfo::s_vtknvindex_sysinfo = vtknvindex_sysinfo::create_sysinfo();
