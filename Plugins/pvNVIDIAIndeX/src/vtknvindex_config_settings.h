/* Copyright 2019 NVIDIA Corporation. All rights reserved.
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

#ifndef vtknvindex_config_settings_h
#define vtknvindex_config_settings_h

#include <map>
#include <string>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkXMLDataParser.h>

#include <nv/index/iconfig_settings.h>
#include <nv/index/islice_scene_element.h>

#include "vtknvindex_rtc_kernel_params.h"

// Defines max slices and slice maps in the scene graph
#define MAX_SLICES 3

// Represents NVIDIA IndeX's slice parameter.
struct vtknvindex_slice_params
{
  enum slice_mode
  {
    X_NORMAL = 0,
    Y_NORMAL,
    Z_NORMAL,
    CUSTOM
  };

  bool enable;
  slice_mode mode;
  mi::math::Vector<mi::Float32, 3> position;
  mi::math::Vector<mi::Float32, 3> normal;
  mi::math::Vector<mi::Float32, 3> up;
  mi::math::Vector<mi::Float32, 2> extent;
  mi::Float32 displace;

  vtknvindex_slice_params();
};

// The class vtknvindex_xml_config_parser is used to parse xml files with IndeX settings
// for licensing and networking.

class vtknvindex_xml_config_parser
{
public:
  vtknvindex_xml_config_parser();
  ~vtknvindex_xml_config_parser();

  // Open an xml config file.
  bool open_config_file(const std::string& config_filename, bool create = false);

  // Retrieve license string from the xml file.
  bool get_license_strings(std::string& vendor_key, std::string& secret_key);

  // Retrieve Flex license path.
  bool get_flex_license_path(std::string& path);

  // Retrieve all parameters from a given section.
  bool get_section_settings(
    std::map<std::string, std::string>& params, const std::string& section) const;

private:
  // utilities
  static bool file_exists(const std::string& filename);
  static bool get_home_path(std::string& home_path);
  static bool get_temp_path(std::string& temp_path);
  bool create_config_file(const std::string& filename) const;

  vtkSmartPointer<vtkXMLDataParser> m_parser;
  vtkXMLDataElement* m_root_elm;
};

// Class vtknvindex_config_settings represents general volume and rendering settings
// from ParaView's GUI for setting up NVIDIA IndeX.

class vtknvindex_config_settings
{
public:
  vtknvindex_config_settings();
  ~vtknvindex_config_settings();

  // Set region of interest.
  void set_region_of_interest(const mi::math::Bbox_struct<mi::Float32, 3>& region_of_interest);
  void get_region_of_interest(mi::math::Bbox_struct<mi::Float32, 3>& region_of_interest) const;

  // Set/get horizontal slice position.
  void set_hslice_position(mi::Float32 slice_pos);
  mi::Float32 get_hslice_position() const;

  // Set/get subcube size.
  void set_subcube_size(const mi::math::Vector_struct<mi::Uint32, 3>& subcube_size);
  void get_subcube_size(mi::math::Vector_struct<mi::Uint32, 3>& subcube_size) const;

  // Set/get subcube border size.
  void set_subcube_border(mi::Sint32 border);
  mi::Uint32 get_subcube_border() const;

  // Set/get filtering mode
  void set_filter_mode(mi::Sint32 filter_mode);
  nv::index::IConfig_settings::Volume_filtering_modes get_filter_mode() const;

  // Set/get pre-integration mode.
  void set_preintegration(bool enable_preint);
  bool is_preintegration() const;

  // Set/get volume step size.
  void set_step_size(mi::Float32 step_size);
  mi::Float32 get_step_size() const;

  // Set/get irregular volume step size.
  void set_ivol_step_size(mi::Float32 step_size);
  mi::Float32 get_ivol_step_size() const;

  // Set/get flag for performance logging.
  void set_log_performance(bool is_dump);
  bool is_log_performance() const;

  // Set/get flag to dump internal state.
  void set_dump_internal_state(bool is_dump);
  bool is_dump_internal_state() const;

  // Set/get play forward/backward time series animation.
  void animation_play_forward(bool play_forward);
  bool animation_play_forward() const;

  // Set/get time series animation interval max.
  void set_animation_interval_max(mi::Sint32 interval_max);
  mi::Uint32 get_animation_interval_max() const;

  // Set/get opacity mode and reference.
  void set_opacity_mode(mi::Uint32 opacity_mode);
  mi::Uint32 get_opacity_mode() const;

  void set_opacity_reference(mi::Float32 opacity_reference);
  mi::Float32 get_opacity_reference() const;

  // Set/get slice parameter.
  void enable_volume(bool enable);
  bool get_enable_volume() const;

  void enable_slice(mi::Uint32 id, bool enable);
  void set_slice_mode(mi::Uint32 id, mi::Uint32 slice_mode);
  void set_slice_displace(mi::Uint32 id, mi::Float32 displace);

  // Set/get CUDA-based shader mode.
  void set_rtc_kernel(vtknvindex_rtc_kernels kernel);
  vtknvindex_rtc_kernels get_rtc_kernel() const;

  const vtknvindex_slice_params& get_slice_params(mi::Uint32 id) const;

private:
  vtknvindex_config_settings(const vtknvindex_config_settings&) = delete;
  void operator=(const vtknvindex_config_settings&) = delete;

  bool m_enable_preintegration;                               // Use pre-integration tables.
  bool m_dump_internal_state;                                 // Dump scene to file.
  bool m_log_performance;                                     // Log performance values to file.
  bool m_animation_play_forward;                              // Animation is running.
  mi::Uint32 m_animation_interval_max;                        // The max animation interval.
  mi::Uint32 m_filter_mode;                                   // Volume filtering mode.
  mi::Uint32 m_subcube_border;                                // NVIDIA IndeX subcube border size.
  mi::Float32 m_step_size;                                    // Volume raycast step size.
  mi::Float32 m_ivol_step_size;                               // IRR-Volume raycast step size.
  mi::Uint32 m_opacity_mode;                                  // Volume opacity mode.
  mi::Float32 m_opacity_reference;                            // Volume reference opacity.
  vtknvindex_rtc_kernels m_rtc_kernel;                        // Current rtc kernel.
  mi::math::Vector_struct<mi::Uint32, 3> m_subcube_size;      // NVIDIA IndeX subcube size.
  mi::math::Bbox_struct<mi::Float32, 3> m_region_of_interest; // Region of interest.

  bool m_enable_volume;                               // Render volume?
  vtknvindex_slice_params m_slice_params[MAX_SLICES]; // List of slice params.
};

#endif
