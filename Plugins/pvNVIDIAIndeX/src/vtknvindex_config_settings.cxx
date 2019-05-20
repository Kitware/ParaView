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

#include <sys/stat.h>
#ifdef _WIN32
#include "shlobj.h"
#endif // _WIN32

#include "vtkSetGet.h"
#include "vtkXMLDataSetWriter.h"

#include "vtknvindex_config_settings.h"
#include "vtknvindex_forwarding_logger.h"

//-------------------------------------------------------------------------------------------------
vtknvindex_slice_params::vtknvindex_slice_params()
{
  enable = false;
  position = normal = up = mi::math::Vector<mi::Float32, 3>(0.f);
  extent = mi::math::Vector<mi::Float32, 2>(100.f);
  displace = 0;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_xml_config_parser::vtknvindex_xml_config_parser()
{
  m_root_elm = NULL;
  m_parser = vtkSmartPointer<vtkXMLDataParser>::New();
  m_parser->SetIgnoreCharacterData(0);
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_xml_config_parser::open_config_file(const std::string& config_filename, bool create)
{
  m_root_elm = NULL;

  std::string config_full_path;

  if (!get_home_path(config_full_path))
    return false;

  config_full_path += config_filename;

  // If config file doesn't exists, then we just ignore it quietly.
  if (!file_exists(config_full_path))
  {
    if (!create)
      return false;

    if (!create_config_file(config_full_path))
      return false;
  }

  m_parser->SetFileName(config_full_path.c_str());

  if (m_parser->Parse() != 1)
  {
    ERROR_LOG << "XML Config Parser | Unable to parse the config file: " << config_filename << ".";
    return false;
  }

  m_root_elm = m_parser->GetRootElement();
  if (std::string(m_root_elm->GetName()) != std::string("index_config"))
  {
    ERROR_LOG << "XML Config Parser | The config file: '" << config_filename
              << "' doesn't contain NVIDIA IndeX configuration parameter.";
    return false;
  }

  return true;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_xml_config_parser::~vtknvindex_xml_config_parser()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_xml_config_parser::get_license_strings(
  std::string& vendor_key, std::string& secret_key)
{
  vtkXMLDataElement* license_elm = m_root_elm->LookupElementWithName("license");
  if (!license_elm)
    return false;

  vtkXMLDataElement* vendor_key_elm = license_elm->FindNestedElementWithName("vendor_key");
  if (!vendor_key_elm)
  {
    ERROR_LOG << "XML Config Parser | The vendor key is not available.";
    return false;
  }

  vtkXMLDataElement* secret_key_elm = license_elm->FindNestedElementWithName("secret_key");
  if (!secret_key_elm)
  {
    ERROR_LOG << "XML Config Parser | The secret key is not available.";
    return false;
  }

  vendor_key = std::string(vendor_key_elm->GetCharacterData());
  secret_key = std::string(secret_key_elm->GetCharacterData());

  return true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_xml_config_parser::get_flex_license_path(std::string& path)
{
  vtkXMLDataElement* license_elm = m_root_elm->LookupElementWithName("license");
  if (!license_elm)
  {
    return false;
  }

  vtkXMLDataElement* flex_license_path_elm =
    license_elm->FindNestedElementWithName("flex_license_path");
  if (!flex_license_path_elm)
  {
    return false;
  }

  path = std::string(flex_license_path_elm->GetCharacterData());

  return true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_xml_config_parser::get_section_settings(
  std::map<std::string, std::string>& params, const std::string& section) const
{
  vtkXMLDataElement* section_elm = m_root_elm->LookupElementWithName(section.c_str());
  if (!section_elm)
    return false;

  mi::Sint32 num_settings = section_elm->GetNumberOfNestedElements();
  for (mi::Sint32 i = 0; i < num_settings; i++)
  {
    vtkXMLDataElement* param_elm = section_elm->GetNestedElement(i);
    std::string param_name(param_elm->GetName());
    std::string param_value(param_elm->GetCharacterData());
    params[param_name] = param_value;
  }

  return true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_xml_config_parser::file_exists(const std::string& filename)
{
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_xml_config_parser::get_home_path(std::string& home_path)
{
  // get home path from environment
  const char* env_home_path = getenv("NVINDEX_PVPLUGIN_HOME");

  if (env_home_path != NULL)
  {
    home_path = std::string(env_home_path);

#ifdef _WIN32
    home_path += "\\";
#else
    home_path += "/";
#endif

    return true;
  }

// get home path from ParaView configuration folder
#ifdef _WIN32
  TCHAR app_data_path[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, app_data_path)))
  {
    home_path = std::string(app_data_path) + "\\ParaView\\";
    return true;
  }
#else
  const char* home = getenv("HOME");

  if (home != NULL && strlen(home) > 0)
  {
    home_path = std::string(home) + std::string("/.config/ParaView/");
    return true;
  }
#endif // _WIN32

  home_path = "";
  return false;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_xml_config_parser::get_temp_path(std::string& temp_path)
{
#ifdef _WIN32

  char path[MAX_PATH];
  if (GetTempPath(MAX_PATH, path) != 0)
  {
    temp_path = path;
    return true;
  }
  else
  {
    temp_path = "";
    return false;
  }

#else
  temp_path = "/tmp";

  char* path = getenv("TMPDIR");
  if (path == NULL || strlen(path) == 0)
  {
    path = getenv("TEMP");
    if (path == NULL || strlen(path) == 0)
    {
      path = getenv("TEMPDIR");
    }
  }

  if (path != NULL || strlen(path) > 0)
  {
    temp_path = path;
  }

  temp_path += "/";

  return true;

#endif // _WIN32
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_xml_config_parser::create_config_file(const std::string& filename) const
{
  std::ofstream xml_file;
  xml_file.open(filename.c_str());
  if (xml_file.is_open())
  {
    xml_file << "<index_config>" << std::endl << "</index_config>";
    xml_file.close();
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_config_settings::vtknvindex_config_settings()
  : m_enable_preintegration(false)
  , m_dump_internal_state(false)
  , m_log_performance(false)
  , m_animation_play_forward(true)
  , m_animation_interval_max(1)
  , m_filter_mode(0)
  , m_subcube_border(2)
  , m_step_size(1.0f)
  , m_ivol_step_size(1.0f)
  , m_opacity_mode(0)
  , m_opacity_reference(1.0f)
  , m_rtc_kernel(RTC_KERNELS_NONE)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_config_settings::~vtknvindex_config_settings()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_region_of_interest(
  const mi::math::Bbox_struct<mi::Float32, 3>& region_of_interest)
{
  m_region_of_interest = region_of_interest;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::get_region_of_interest(
  mi::math::Bbox_struct<mi::Float32, 3>& region_of_interest) const
{
  region_of_interest = m_region_of_interest;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_subcube_size(
  const mi::math::Vector_struct<mi::Uint32, 3>& subcube_size)
{
  m_subcube_size = subcube_size;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::get_subcube_size(
  mi::math::Vector_struct<mi::Uint32, 3>& subcube_size) const
{
  subcube_size = m_subcube_size;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_subcube_border(mi::Sint32 border)
{
  m_subcube_border = border;
}

//-------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_config_settings::get_subcube_border() const
{
  return m_subcube_border;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_filter_mode(mi::Sint32 filter_mode)
{
  m_filter_mode = filter_mode;
}

//-------------------------------------------------------------------------------------------------
nv::index::IConfig_settings::Volume_filtering_modes vtknvindex_config_settings::get_filter_mode()
  const
{
  mi::Uint32 filter_mode = m_filter_mode;
  if (filter_mode == 5)
    filter_mode = 3;
  else if (filter_mode == 7)
    filter_mode = 5;

  return static_cast<nv::index::IConfig_settings::Volume_filtering_modes>(filter_mode);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_preintegration(bool enable_preint)
{
  m_enable_preintegration = enable_preint;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_config_settings::is_preintegration() const
{
  return m_enable_preintegration;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_step_size(mi::Float32 step_size)
{
  m_step_size = step_size;
}

//-------------------------------------------------------------------------------------------------
mi::Float32 vtknvindex_config_settings::get_step_size() const
{
  return m_step_size;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_ivol_step_size(mi::Float32 step_size)
{
  m_ivol_step_size = step_size;
}
//-------------------------------------------------------------------------------------------------
mi::Float32 vtknvindex_config_settings::get_ivol_step_size() const
{
  return m_ivol_step_size;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_dump_internal_state(bool is_dump)
{
  m_dump_internal_state = is_dump;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_config_settings::is_dump_internal_state() const
{
  return m_dump_internal_state;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_log_performance(bool is_log)
{
  m_log_performance = is_log;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_config_settings::is_log_performance() const
{
  return m_log_performance;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::animation_play_forward(bool play_forward)
{
  m_animation_play_forward = play_forward;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_config_settings::animation_play_forward() const
{
  return m_animation_play_forward;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_animation_interval_max(mi::Sint32 interval_max)
{
  m_animation_interval_max = interval_max;
}

//-------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_config_settings::get_animation_interval_max() const
{
  return m_animation_interval_max;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_opacity_mode(mi::Uint32 opacity_mode)
{
  m_opacity_mode = opacity_mode;
}

//-------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_config_settings::get_opacity_mode() const
{
  return m_opacity_mode;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_opacity_reference(mi::Float32 opacity_reference)
{
  m_opacity_reference = opacity_reference;
}

//-------------------------------------------------------------------------------------------------
mi::Float32 vtknvindex_config_settings::get_opacity_reference() const
{
  return m_opacity_reference;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::enable_volume(bool enable)
{
  m_enable_volume = enable;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_config_settings::get_enable_volume() const
{
  return m_enable_volume;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::enable_slice(mi::Uint32 id, bool enable)
{
  m_slice_params[id].enable = enable;
}
//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_slice_displace(mi::Uint32 id, mi::Float32 displace)
{
  m_slice_params[id].displace = displace;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_slice_mode(mi::Uint32 id, mi::Uint32 slice_mode)
{
  m_slice_params[id].mode = static_cast<vtknvindex_slice_params::slice_mode>(slice_mode);
}

//-------------------------------------------------------------------------------------------------
const vtknvindex_slice_params& vtknvindex_config_settings::get_slice_params(mi::Uint32 id) const
{
  return m_slice_params[id];
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_config_settings::set_rtc_kernel(vtknvindex_rtc_kernels kernel)
{
  m_rtc_kernel = kernel;
}
//-------------------------------------------------------------------------------------------------
vtknvindex_rtc_kernels vtknvindex_config_settings::get_rtc_kernel() const
{
  return m_rtc_kernel;
}
