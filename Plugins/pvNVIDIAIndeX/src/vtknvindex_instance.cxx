/* Copyright 2025 NVIDIA Corporation. All rights reserved.
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
// SPDX-FileCopyrightText: Copyright 2025 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtknvindex_instance.h"

#include <cassert>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else // _WIN32
#include <dlfcn.h>
#include <ifaddrs.h>
#include <netdb.h>
#endif // _WIN32

#include "vtkMultiProcessController.h"
#include "vtksys/SystemInformation.hxx"
#include "vtksys/SystemTools.hxx"

#include <nv/index/icolormap.h>
#include <nv/index/iindex_debug_configuration.h>
#include <nv/index/ilight.h>
#include <nv/index/iscene.h>
#include <nv/index/isession.h>
#include <nv/index/version.h>

#include "vtknvindex_affinity.h"
#include "vtknvindex_colormap_utility.h"
#include "vtknvindex_config_settings.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_global_settings.h"
#include "vtknvindex_host_properties.h"
#include "vtknvindex_irregular_volume_importer.h"
#include "vtknvindex_receiving_logger.h"
#include "vtknvindex_sparse_volume_importer.h"
#include "vtknvindex_volume_compute.h"

#ifdef __APPLE__
#define VTKNVINDEX_REMOTE_RENDERING_ONLY
#endif

namespace
{

#ifdef _WIN32
//-------------------------------------------------------------------------------------------------
// Create a string with last error message
static std::string get_last_error_as_str()
{
  DWORD error = GetLastError();
  if (error)
  {
    LPVOID lpMsgBuf;
    DWORD bufLen = FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, nullptr);

    if (bufLen)
    {
      LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
      std::string result(lpMsgStr, lpMsgStr + bufLen);

      LocalFree(lpMsgBuf);

      return result;
    }
  }
  return std::string();
}

#else
std::string get_interface_address(const std::string interface_name, bool ipv6 = false)
{
  std::string result;

  ifaddrs* if_list;
  if (getifaddrs(&if_list) == 0)
  {
    for (ifaddrs* i = if_list; i; i = i->ifa_next)
    {
      if (i->ifa_addr != nullptr && i->ifa_name != nullptr &&
        ((i->ifa_addr->sa_family == AF_INET && !ipv6) ||
          (i->ifa_addr->sa_family == AF_INET6 && ipv6)) &&
        std::string(i->ifa_name) == interface_name)
      {
        char buf[1025];
        int s = getnameinfo(i->ifa_addr,
          (i->ifa_addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in)
                                              : sizeof(struct sockaddr_in6),
          buf, 1025, nullptr, 0, NI_NUMERICHOST);
        if (s == 0)
        {
          result = buf;
          break;
        }
      }
    }
    freeifaddrs(if_list);
  }

  return result;
}
#endif // _WIN32

bool is_compatible_revision(const std::string& build_revision_str,
  const std::string& library_revision_full_str, bool& revision_mismatch)
{
  // Extract just the revision, without build date and platform
  std::string library_revision_str = library_revision_full_str;
  const std::size_t pos = library_revision_str.find(',');
  if (pos != std::string::npos)
  {
    library_revision_str = library_revision_str.substr(0, pos);
  }

  revision_mismatch = (library_revision_str != build_revision_str);
  if (revision_mismatch)
  {
    // Split up revision into its components
    std::vector<std::string> library_revision;
    std::stringstream str(library_revision_str);
    while (str.good())
    {
      std::string component;
      std::getline(str, component, '.');
      library_revision.push_back(component);
    }

    std::vector<std::string> build_revision = { std::to_string(
      NVIDIA_INDEX_LIBRARY_REVISION_MAJOR) };
    if (NVIDIA_INDEX_LIBRARY_REVISION_MINOR > 0)
    {
      build_revision.push_back(std::to_string(NVIDIA_INDEX_LIBRARY_REVISION_MINOR));
      if (NVIDIA_INDEX_LIBRARY_REVISION_SUBMINOR > 0)
      {
        build_revision.push_back(std::to_string(NVIDIA_INDEX_LIBRARY_REVISION_SUBMINOR));
      }
    }

    if (build_revision.size() == 1 || library_revision.size() == 1)
    {
      // No branch, meaning development version. Let's assume users know what they're doing.
      return true;
    }
    else if (build_revision.size() != library_revision.size())
    {
      return false;
    }
    else if (build_revision.size() >= 2)
    {
      // It's a release branch, verify that the major revision is the same
      return (build_revision[0] == library_revision[0]);
    }
  }

  return true;
}

} // namespace

const std::string vtknvindex_instance::s_config_filename = "nvindex_config.xml";

//-------------------------------------------------------------------------------------------------
vtknvindex_instance::vtknvindex_instance()
  : m_is_index_rank(false)
  , m_is_index_viewer(false)
  , m_is_index_initialized(false)
  , m_nvindex_colormaps(nullptr)
{
  // Build cluster information
  build_cluster_info();

  // Use one IndeX instance per host (running on the local rank 0)
  if (is_index_rank())
  {
    // Load NVIDIA IndeX library
    if (!load_nvindex())
      return;
  }
}

//-------------------------------------------------------------------------------------------------
vtknvindex_instance::~vtknvindex_instance()
{
  if (is_index_rank() && m_nvindex_interface)
  {
    {
      // Reduce log level for shutdown
      mi::base::Handle<mi::neuraylib::ILogging_configuration> logging_configuration(
        m_nvindex_interface->get_api_component<mi::neuraylib::ILogging_configuration>());
      logging_configuration->set_log_level(mi::base::MESSAGE_SEVERITY_WARNING);

      // Reduce log level even more when running with old library version, to hide harmless warnings
      if (std::string(m_nvindex_interface->get_revision()).find("329100.8100.3009,") == 0)
      {
        logging_configuration->set_log_level(mi::base::MESSAGE_SEVERITY_FATAL);
      }
    }

    // Shut down forwarding logger.
    vtknvindex::logger::vtknvindex_forwarding_logger_factory::delete_instance();

    // Shut down the NVIDIA IndeX library.
    shutdown_nvindex();

    // Unload the libraries.
    unload_nvindex();
  }

  delete m_nvindex_colormaps;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_instance::is_index_viewer() const
{
  return m_is_index_viewer;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_instance::is_index_rank() const
{
  return m_is_index_rank;
}

//-------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_instance::get_cur_local_rank_id() const
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  mi::Sint32 cur_rank_id = controller->GetLocalProcessId();

  vtksys::SystemInformation sys_info;
  std::string cur_host = sys_info.GetHostname();

  std::map<std::string, std::vector<mi::Sint32>>::const_iterator it =
    m_hostname_to_rankids.find(cur_host);
  if (it == m_hostname_to_rankids.end())
    return -1;

  for (mi::Size j = 0; j < it->second.size(); ++j)
  {
    if (it->second[j] == cur_rank_id)
      return j;
  }

  return -1;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_instance* vtknvindex_instance::get()
{
  // As a workaround for undefined shutdown order of static variables in dependency libs, this
  // pointer is leaked on shutdown.
  static vtknvindex_instance* instance = new vtknvindex_instance;
  return instance;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_instance::init_index()
{
  // Start one IndeX instance per host (running on the local rank 0)
  if (m_nvindex_interface && !m_is_index_initialized && is_index_rank())
  {
    // Setup NVIDIA IndeX
    if (!setup_nvindex())
    {
      return;
    }

    // Initialize IndeX session
    initialize_session();

    // Initialize scene graph
    if (m_is_index_viewer)
      init_scene_graph();

    m_is_index_initialized = true;
  }
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_instance::ensure_index_initialized() const
{
  static bool first = true;
  if (!m_is_index_initialized && first)
  {
    first = false;
#ifdef VTKNVINDEX_REMOTE_RENDERING_ONLY

#ifdef __APPLE__
    const char* platform = "macOS";
#else
    const char* platform = "this platform";
#endif

    ERROR_LOG
      << "The NVIDIA IndeX plugin does not support local rendering on " << platform << ". "
      << "You can however connect to a remote pvserver running NVIDIA IndeX on another host.";

#else // VTKNVINDEX_REMOTE_RENDERING_ONLY

    ERROR_LOG << "The NVIDIA IndeX plugin was not initialized! See the log output for details.";

#endif // VTKNVINDEX_REMOTE_RENDERING_ONLY
  }

  return m_is_index_initialized;
}

//-------------------------------------------------------------------------------------------------
mi::neuraylib::Tag vtknvindex_instance::get_perspective_camera() const
{
  return m_perspective_camera_tag;
}

//-------------------------------------------------------------------------------------------------
mi::neuraylib::Tag vtknvindex_instance::get_parallel_camera() const
{
  return m_parallel_camera_tag;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_colormap* vtknvindex_instance::get_index_colormaps() const
{
  return m_nvindex_colormaps;
}

//-------------------------------------------------------------------------------------------------
mi::neuraylib::Tag vtknvindex_instance::get_slice_colormap() const
{
  return m_slice_colormap_tag;
}

//-------------------------------------------------------------------------------------------------
mi::neuraylib::Tag vtknvindex_instance::get_scene_geom_group() const
{
  return m_geom_group_tag;
}

//-------------------------------------------------------------------------------------------------
mi::base::Handle<nv::index::IIndex>& vtknvindex_instance::get_interface()
{
  return m_nvindex_interface;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_instance::build_cluster_info()
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  mi::Sint32 nb_ranks = controller->GetNumberOfProcesses();

  // Gather all the rank ids.
  std::vector<mi::Sint32> rank_ids;
  rank_ids.resize(nb_ranks);

  mi::Sint32 cur_rank_id = controller->GetLocalProcessId();
  controller->AllGather(&cur_rank_id, &rank_ids[0], 1);

  // Gather host names from all the ranks.
  std::vector<std::string> host_names;
  vtksys::SystemInformation sys_info;
  std::string cur_host_name = sys_info.GetHostname() + std::string(" ");

  char* all_hosts = (char*)calloc(nb_ranks, cur_host_name.length() + 1);
  controller->AllGather(cur_host_name.c_str(), all_hosts, cur_host_name.length());

  std::string str(all_hosts);
  std::string buf;
  std::stringstream ss(str);

  while (ss >> buf)
    host_names.push_back(buf);

  free(all_hosts);

  controller->Barrier();

  // Get host ranks distribution
  for (mi::Sint32 i = 0; i < nb_ranks; i++)
    m_hostname_to_rankids[host_names[i]].push_back(rank_ids[i]);

  // Find index ranks
  if (cur_rank_id == 0)
  {
    m_is_index_viewer = true;
    m_is_index_rank = true;
  }
  else
  {
    m_is_index_viewer = false;
    cur_host_name = sys_info.GetHostname();
    std::map<std::string, std::vector<mi::Sint32>>::const_iterator it =
      m_hostname_to_rankids.find(cur_host_name);
    m_is_index_rank = (it != m_hostname_to_rankids.cend() && it->second[0] == cur_rank_id);
  }

  // Get host list
  m_host_list.clear();
  for (mi::Uint32 i = 0; i < host_names.size(); ++i)
  {
    if (find(m_host_list.begin(), m_host_list.end(), host_names[i]) == m_host_list.end())
      m_host_list.push_back(host_names[i]);
  }

  controller->Barrier();
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_instance::load_nvindex()
{
#ifdef VTKNVINDEX_REMOTE_RENDERING_ONLY

  return false;

#else // VTKNVINDEX_REMOTE_RENDERING_ONLY

  // Load shared libraries.
  const char* lib_name = "libnvindex" MI_BASE_DLL_FILE_EXT;
#if (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR >= 372500)
  const char* entry_point_name = "nv_factory";
#else
  const char* entry_point_name = "nv_index_factory";
#endif
  void* index_lib_symbol = nullptr;

#ifdef _WIN32
  m_p_handle = LoadLibrary(TEXT(lib_name));

  if (m_p_handle)
    index_lib_symbol = GetProcAddress((HMODULE)m_p_handle, entry_point_name);
#else
  m_p_handle = dlopen(lib_name, RTLD_LAZY);

  if (m_p_handle)
    index_lib_symbol = dlsym(m_p_handle, entry_point_name);
#endif

  if (!index_lib_symbol)
  {
#ifdef _WIN32
    const std::string error_str = get_last_error_as_str();
    const std::string path_env = "PATH";
#else
    const std::string error_str = dlerror();
    const std::string path_env = "LD_LIBRARY_PATH";
#endif

#if (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR >= 372500)
    if (m_p_handle)
    {
      // Check if the entry point used by older versions of IndeX exists
      const char* entry_point_name_old = "nv_index_factory";
#ifdef _WIN32
      index_lib_symbol = GetProcAddress((HMODULE)m_p_handle, entry_point_name_old);
#else
      index_lib_symbol = dlsym(m_p_handle, entry_point_name_old);
#endif
      if (index_lib_symbol)
      {
        // The library is still using the old entry point, bail out early
        ERROR_LOG << "The NVIDIA IndeX library '" << lib_name
                  << "' is not compatible with this plugin version '" << get_version()
                  << "', which was built against revision '" << NVIDIA_INDEX_LIBRARY_REVISION_STRING
                  << "' "
                  << "Please check your ParaView installation or get the matching IndeX libraries "
                  << "from the ParaView dependencies repository: "
                  << "https://www.paraview.org/files/dependencies/";
        return false;
      }
    }
#endif

    ERROR_LOG << "Failed to " << (m_p_handle ? "retrieve the entry point into" : "load")
              << " the NVIDIA IndeX library '" << lib_name << "': '" << error_str << "'.";
    ERROR_LOG << "Please verify that the environment variable " << path_env
              << " contains the path to the correct NVIDIA IndeX libraries.";

    return false;
  }

  m_nvindexlib_fname = lib_name;

  std::string library_revision_full_str;
#if (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR >= 372500)
  // Only access IVersion here, IIndeX will be retrieved after successful version check
  {
    mi::base::Handle<mi::neuraylib::IVersion> version(
      nv::index::nv_factory<mi::neuraylib::IVersion>(index_lib_symbol));
    if (!version)
    {
      ERROR_LOG << "Failed to retrieve the IVersion interface from the NVIDIA IndeX library.";
      return false;
    }

    library_revision_full_str = version->get_build_number();
  }
#else
  typedef nv::index::IIndex*(IIndex_factory());
  IIndex_factory* factory = (IIndex_factory*)index_lib_symbol;
  m_nvindex_interface = factory();
  if (!m_nvindex_interface.is_valid_interface())
  {
    ERROR_LOG << "Failed to initialize the NVIDIA IndeX library interface.";
    return false;
  }

  library_revision_full_str = m_nvindex_interface->get_revision();
#endif

  // Check that this IndeX library is compatible with this plugin version
  const std::string build_revision_str = NVIDIA_INDEX_LIBRARY_REVISION_STRING;
  bool revision_mismatch = false;
  if (!is_compatible_revision(build_revision_str, library_revision_full_str, revision_mismatch))
  {
    // Note that this doesn't automatically open the "Output Messages" window if it happens
    // during ParaView startup. The message will however become visible when another error is
    // later triggered in Render().
    ERROR_LOG << "The loaded NVIDIA IndeX library build '" << library_revision_full_str
              << "' is not compatible with this plugin version '" << get_version()
              << "', which was built against revision '" << build_revision_str << "' "
              << "Please check your ParaView installation or get the matching IndeX libraries "
              << "from the ParaView dependencies repository: "
              << "https://www.paraview.org/files/dependencies/";

    INFO_LOG << "Shutting down NVIDIA IndeX...";
    if (m_nvindex_interface)
    {
      m_nvindex_interface->shutdown();
      m_nvindex_interface.reset();
    }
    unload_nvindex();

    return false;
  }

#if (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR >= 372500)
#ifndef _WIN32
  // Explicitly set the path used by NVIDIA IndeX to load libdice.so. This is necessary in case the
  // non-transitive RUNPATH is applied to the plugin instead of RPATH.
  mi::base::Handle<nv::index::IIndex_loader_configuration> loader_config(
    nv::index::nv_factory<nv::index::IIndex_loader_configuration>(index_lib_symbol));
  if (loader_config)
  {
    loader_config->clear();
    // First search in the directory containing libnvindex.so
    loader_config->add_library_search_origin();
    // Then fall back to the default search path (e.g. LD_LIBRARY_PATH)
    loader_config->add_library_search_default();
  }
#endif // _WIN32

  m_nvindex_interface = nv::index::nv_factory<nv::index::IIndex>(index_lib_symbol);
  if (!m_nvindex_interface.is_valid_interface())
  {
    ERROR_LOG << "Failed to initialize the NVIDIA IndeX library interface.";
    return false;
  }
#endif

  // Check for license and authenticate.
  const mi::Sint32 auth_result = authenticate_nvindex();
  if (auth_result != 0)
  {
    ERROR_LOG << "Failed to authenticate NVIDIA IndeX library (result " << auth_result
              << "), please provide a valid license.";
    m_nvindex_interface->shutdown();
    m_nvindex_interface.reset();
    unload_nvindex();
    return false;
  }

  // Initialize logging through NVIDIA IndeX.
  vtknvindex::logger::vtknvindex_forwarding_logger_factory::instance()->initialize(
    m_nvindex_interface);

  {
    mi::base::Handle<mi::neuraylib::ILogging_configuration> logging_configuration(
      m_nvindex_interface->get_api_component<mi::neuraylib::ILogging_configuration>());

    logging_configuration->set_log_locally(true); // local logging

    // Create verbose log output, even when it will not be shown with the default log level
    // configured of the receiving logger.
    const mi::base::Message_severity log_level = mi::base::MESSAGE_SEVERITY_VERBOSE;
    logging_configuration->set_log_level(log_level);
    logging_configuration->set_log_level_by_category("MAIN", log_level);

    vtknvindex_global_settings* settings = vtknvindex_global_settings::GetInstance();
    mi::Uint32 log_prefix = logging_configuration->get_log_prefix();
    if (settings->GetLogTimestamp())
    {
      log_prefix |= mi::neuraylib::LOG_PREFIX_TIME;
    }

    if (settings->GetLogHostname())
    {
      log_prefix |= mi::neuraylib::LOG_PREFIX_HOST_NAME;
    }

    logging_configuration->set_log_prefix(log_prefix);

    // Install the receiving logger.
    mi::base::Handle<mi::base::ILogger> receiving_logger(new vtknvindex_receiving_logger());
    logging_configuration->set_receiving_logger(receiving_logger.get());
  }

  // Access and log NVIDIA IndeX version.
  INFO_LOG << "NVIDIA IndeX ParaView plugin " << get_version() << " "
           << (revision_mismatch ? "(compiled against " + build_revision_str + ") " : "")
           << "using NVIDIA IndeX library " << m_nvindex_interface->get_version() << " (build "
           << library_revision_full_str << ").";

  return true;

#endif // VTKNVINDEX_REMOTE_RENDERING_ONLY
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_instance::unload_nvindex()
{
#ifdef VTKNVINDEX_REMOTE_RENDERING_ONLY

  return false;

#else // VTKNVINDEX_REMOTE_RENDERING_ONLY

  assert(m_p_handle != 0);

#ifdef _WIN32
  if (TRUE != FreeLibrary((HMODULE)m_p_handle))
  {
    const std::string nvindex_fname = m_nvindexlib_fname;
    ERROR_LOG << "Failed to unload the NVIDIA IndeX library (" << nvindex_fname << ").";
    return false;
  }
#else  // _WIN32
  int result = dlclose(m_p_handle);
  if (result != 0)
  {
    ERROR_LOG << "Failed to unload the NVIDIA IndeX library: " << dlerror() << ".";
    return false;
  }
#endif // _WIN32

  return true;

#endif // VTKNVINDEX_REMOTE_RENDERING_ONLY
}

//-------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_instance::authenticate_nvindex()
{
  std::string index_vendor_key;
  std::string index_secret_key;
  bool found_license = false;

  // Try reading license from environment.
  const char* env_vendor_key = vtksys::SystemTools::GetEnv("NVINDEX_VENDOR_KEY");
  const char* env_secret_key = vtksys::SystemTools::GetEnv("NVINDEX_SECRET_KEY");

  if (env_vendor_key != nullptr && env_secret_key != nullptr)
  {
    index_vendor_key = env_vendor_key;
    index_secret_key = env_secret_key;
    found_license = (!index_vendor_key.empty() && !index_secret_key.empty());
    if (found_license)
    {
      INFO_LOG << "Using NVIDIA IndeX license from environment variables NVINDEX_VENDOR_KEY and "
               << "NVINDEX_SECRET_KEY.";
    }
  }

  if (!found_license)
  {
    vtknvindex_global_settings* settings = vtknvindex_global_settings::GetInstance();
    if (settings->GetVendorKey() && settings->GetSecretKey())
    {
      index_vendor_key = settings->GetVendorKey();
      index_secret_key = settings->GetSecretKey();
      found_license = (!index_vendor_key.empty() && !index_secret_key.empty());
      if (found_license)
      {
        INFO_LOG << "Using NVIDIA IndeX license specified in the 'Settings' dialog.";
      }
    }
  }

  if (!found_license)
  {
    // Try reading license from config file.
    vtknvindex_xml_config_parser xml_parser;
    const std::string config_full_path = xml_parser.get_config_full_path(s_config_filename);
    if (xml_parser.open_config_file(s_config_filename))
    {
      if (xml_parser.get_license_strings(index_vendor_key, index_secret_key))
      {
        if (!index_vendor_key.empty() && !index_secret_key.empty())
        {
          found_license = true;
          INFO_LOG << "Using NVIDIA IndeX license from configuration file '" << config_full_path
                   << "'.";
        }
        else
        {
          ERROR_LOG << "Empty vendor or secret license key defined in '" << config_full_path
                    << "', falling back to default license.";
          found_license = false;
        }
      }
    }
  }

  // No explicit license was specified, fall back to default license.
  if (!found_license)
  {
    return 0;
  }

  return m_nvindex_interface->authenticate(
    index_vendor_key.c_str(), static_cast<mi::Sint32>(index_vendor_key.length()),
    index_secret_key.c_str(), static_cast<mi::Sint32>(index_secret_key.length())
#if (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR < 372500)
                                ,
    nullptr, 0
#endif
  );
}

namespace
{

void setup_nvindex_network_from_settings(
  mi::base::Handle<mi::neuraylib::INetwork_configuration>& inetwork_configuration,
  bool& use_default_cluster_configuration, const std::string& default_discovery_host)
{
  vtknvindex_global_settings* settings = vtknvindex_global_settings::GetInstance();

  const int cluster_mode = settings->GetClusterMode();

  use_default_cluster_configuration = false;
  if (cluster_mode == vtknvindex_global_settings::CLUSTER_OFF)
  {
    inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_OFF);
  }
  else if (cluster_mode == vtknvindex_global_settings::CLUSTER_TCP)
  {
    // This is the default configuration
    use_default_cluster_configuration = true;
  }
  else if (cluster_mode == vtknvindex_global_settings::CLUSTER_UDP)
  {
    inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_UDP);

    // Multicast address.
    if (settings->GetMulticastAddress())
    {
      const std::string multicast_address(settings->GetMulticastAddress());
      if (inetwork_configuration->set_multicast_address(multicast_address.c_str()) != 0)
      {
        ERROR_LOG << "Could not set the multicast address to value '" << multicast_address
                  << "' specified in the 'Settings' dialog.";
      }
    }
    else
    {
      // Use default multicast address.
      const std::string multicast_address = "224.1.3.2";
      WARN_LOG << "Using default multicast address " << multicast_address << ".";
      inetwork_configuration->set_multicast_address(multicast_address.c_str());
    }
  }
  else if (cluster_mode == vtknvindex_global_settings::CLUSTER_TCP_WITH_DISCOVERY)
  {
    inetwork_configuration->set_mode(
      mi::neuraylib::INetwork_configuration::MODE_TCP_WITH_DISCOVERY);

    // Discovery address (required when cluster_mode is "TCP_WITH_DISCOVERY").
    if (settings->GetDiscoveryAddress())
    {
      const std::string discovery_address(settings->GetDiscoveryAddress());
      if (inetwork_configuration->set_discovery_address(discovery_address.c_str()) != 0)
      {
        ERROR_LOG << "Could not set the discovery address to value '" << discovery_address
                  << "' specified in the 'Settings' dialog.";
      }
    }
    else
    {
      // Use default discovery address: first host in the host list
      const std::string discovery_address = default_discovery_host + ":5555";
      inetwork_configuration->set_discovery_address(discovery_address.c_str());
    }
  }
  else
  {
    ERROR_LOG << "Unsupported value " << cluster_mode << " for ClusterMode in 'Settings' dialog.";
  }

  // Cluster interface address.
  if (settings->GetClusterInterfaceAddress())
  {
    const std::string cluster_interface_address(settings->GetClusterInterfaceAddress());
    if (inetwork_configuration->set_cluster_interface(cluster_interface_address.c_str()) != 0)
    {
      ERROR_LOG << "Could not set the cluster interface address to value '"
                << cluster_interface_address << "' specified in the 'Settings' dialog. "
                << "Please ensure to specify a network interface that is valid on all hosts.";
    }
  }

  // RDMA.
  inetwork_configuration->set_use_rdma(settings->GetUseRDMA());

  if (inetwork_configuration->get_use_rdma())
  {
    // Set RDMA interface
    if (settings->GetRDMAInterface())
    {
      const std::string rdma_interface(settings->GetRDMAInterface());
      if (inetwork_configuration->set_rdma_interface(rdma_interface.c_str()) != 0)
      {
        ERROR_LOG << "Could not set the RDMA interface to value '" << rdma_interface
                  << "' specified in the 'Settings' dialog.";
      }
    }

#ifndef _WIN32
    // Set alternative RDMA interface by name
    if (settings->GetRDMAInterfaceByName())
    {
      const std::string rdma_interface_name(settings->GetRDMAInterfaceByName());
      const std::string rdma_interface_address = get_interface_address(rdma_interface_name);
      if (inetwork_configuration->set_rdma_interface(rdma_interface_address.c_str()) != 0)
      {
        ERROR_LOG << "Could not set the RDMA interface name to value '" << rdma_interface_name
                  << "' specified in the 'Settings' dialog.";
      }
    }
#endif // _WIN32
  }
}

void setup_nvindex_network_from_config_file(vtknvindex_xml_config_parser& xml_parser,
  mi::base::Handle<mi::neuraylib::INetwork_configuration>& inetwork_configuration,
  bool& use_default_cluster_configuration, const std::string& default_discovery_host,
  const std::string& config_filename)
{
  std::map<std::string, std::string> network_params;
  if (xml_parser.get_section_settings(network_params, "network"))
  {
    std::map<std::string, std::string>::iterator it;

    // Cluster network mode (protocol).
    it = network_params.find("cluster_mode");
    if (it != network_params.end())
    {
      use_default_cluster_configuration = false;

      const std::string cluster_mode(it->second);
      if (cluster_mode == "OFF")
      {
        inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_OFF);
      }
      else if (cluster_mode == "TCP")
      {
        // This is the default configuration
        use_default_cluster_configuration = true;
      }
      else if (cluster_mode == "UDP")
      {
        inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_UDP);

        // Multicast address.
        it = network_params.find("multicast_address");
        if (it != network_params.end())
        {
          const std::string multicast_address(it->second);
          if (inetwork_configuration->set_multicast_address(multicast_address.c_str()) != 0)
          {
            ERROR_LOG << "Could not set the multicast address to value '" << multicast_address
                      << "' specified in configuration file '" << config_filename << "'.";
          }
        }
        else
        {
          // Use default multicast address.
          const std::string multicast_address = "224.1.3.2";
          WARN_LOG << "Using default multicast address " << multicast_address << ".";
          inetwork_configuration->set_multicast_address(multicast_address.c_str());
        }
      }
      else if (cluster_mode == "TCP_WITH_DISCOVERY")
      {
        inetwork_configuration->set_mode(
          mi::neuraylib::INetwork_configuration::MODE_TCP_WITH_DISCOVERY);

        // Discovery address (required when cluster_mode is "TCP_WITH_DISCOVERY").
        it = network_params.find("discovery_address");
        if (it != network_params.end())
        {
          const std::string discovery_address(it->second);
          if (inetwork_configuration->set_discovery_address(discovery_address.c_str()) != 0)
          {
            ERROR_LOG << "Could not set the discovery address to value '" << discovery_address
                      << "' specified in configuration file '" << config_filename << "'.";
          }
        }
        else
        {
          // Use default discovery address: first host in the host list
          const std::string discovery_address = default_discovery_host + ":5555";
          inetwork_configuration->set_discovery_address(discovery_address.c_str());
        }
      }
      else
      {
        ERROR_LOG << "Unsupported value '" << cluster_mode
                  << "' for 'cluster_mode' specified in configuration file '" << config_filename
                  << "'.";
      }
    }

    // Cluster interface address.
    it = network_params.find("cluster_interface_address");
    if (it != network_params.end())
    {
      const std::string cluster_interface_address(it->second);
      if (inetwork_configuration->set_cluster_interface(cluster_interface_address.c_str()) != 0)
      {
        ERROR_LOG << "Could not set the cluster interface address to value '"
                  << cluster_interface_address << "' specified in configuration file '"
                  << config_filename << "'. "
                  << "Please ensure to specify a network interface that is valid on all hosts.";
      }
    }

    // RDMA.
    it = network_params.find("use_rdma");
    if (it != network_params.end())
    {
      const std::string use_rdma(it->second);
      inetwork_configuration->set_use_rdma(use_rdma == "1" || use_rdma == "yes");
    }

    if (inetwork_configuration->get_use_rdma())
    {
      // Set RDMA interface
      it = network_params.find("rdma_interface");
      if (it != network_params.end())
      {
        const std::string rdma_interface(it->second);
        if (inetwork_configuration->set_rdma_interface(rdma_interface.c_str()) != 0)
        {
          ERROR_LOG << "Could not set the RDMA interface to value '" << rdma_interface
                    << "' specified in configuration file '" << config_filename << "'.";
        }
      }

#ifndef _WIN32
      // Set alternative RDMA interface by name
      it = network_params.find("rdma_interface_by_name");
      if (it != network_params.end())
      {
        const std::string rdma_interface_name(it->second);
        const std::string rdma_interface_address = get_interface_address(rdma_interface_name);
        if (inetwork_configuration->set_rdma_interface(rdma_interface_address.c_str()) != 0)
        {
          ERROR_LOG << "Could not set the RDMA interface name to value '" << rdma_interface_name
                    << "' specified in configuration file '" << config_filename << "'.";
        }
      }
#endif // _WIN32
    }
  }
}

} // namespace

//-------------------------------------------------------------------------------------------------
bool vtknvindex_instance::setup_nvindex()
{
  vtknvindex_xml_config_parser xml_parser;
  bool use_config_file = xml_parser.open_config_file(s_config_filename);

  // Configure networking before starting the IndeX library.
  mi::base::Handle<mi::neuraylib::INetwork_configuration> inetwork_configuration(
    m_nvindex_interface->get_api_component<mi::neuraylib::INetwork_configuration>());
  assert(inetwork_configuration.is_valid_interface());

  // Networking is off by default.
  inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_OFF);

  if (m_host_list.size() > 1)
  {
    bool use_default_cluster_configuration = true;

    setup_nvindex_network_from_settings(
      inetwork_configuration, use_default_cluster_configuration, m_host_list[0]);

    if (use_config_file)
    {
      setup_nvindex_network_from_config_file(xml_parser, inetwork_configuration,
        use_default_cluster_configuration, m_host_list[0], s_config_filename);
    }

    if (use_default_cluster_configuration)
    {
      inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_TCP);
      for (mi::Uint32 i = 0; i < m_host_list.size(); ++i)
      {
        inetwork_configuration->add_configured_host(m_host_list[i].c_str());
      }
    }

    if (inetwork_configuration->get_mode() != mi::neuraylib::INetwork_configuration::MODE_OFF)
    {
      // Define service mode.
      mi::base::Handle<nv::index::ICluster_configuration> icluster_configuration(
        m_nvindex_interface->get_api_component<nv::index::ICluster_configuration>());
      icluster_configuration->set_service_mode("rendering_and_compositing");

      // Debug configuration.
      mi::base::Handle<mi::neuraylib::IDebug_configuration> idebug_configuration(
        m_nvindex_interface->get_api_component<mi::neuraylib::IDebug_configuration>());
      assert(idebug_configuration.is_valid_interface());
      {
        std::map<std::string, std::string> network_params;
        std::map<std::string, std::string>::iterator it;

        if (use_config_file)
          xml_parser.get_section_settings(network_params, "network");

        // Setting max bandwidth.
        it = network_params.find("max_bandwidth");
        if (it != network_params.end())
        {
          std::string max_bandwidth = std::string("max_bandwidth=") + it->second;
          idebug_configuration->set_option(max_bandwidth.c_str());
        }

        // Setting unicast nak interval.
        it = network_params.find("unicast_nak_interval");
        if (it != network_params.end())
        {
          std::string unicast_nak_interval = std::string("unicast_nak_interval=") + it->second;
          idebug_configuration->set_option(unicast_nak_interval.c_str());
        }

        // Bandwidth increment.
        it = network_params.find("bandwidth_increment");
        if (it != network_params.end())
        {
          std::string bandwidth_increment = std::string("bandwidth_increment=") + it->second;
          idebug_configuration->set_option(bandwidth_increment.c_str());
        }

        // Bandwidth decrement.
        it = network_params.find("bandwidth_decrement");
        if (it != network_params.end())
        {
          std::string bandwidth_decrement = std::string("bandwidth_decrement=") + it->second;
          idebug_configuration->set_option(bandwidth_decrement.c_str());
        }

        // Retransmission interval.
        it = network_params.find("retransmission_interval");
        if (it != network_params.end())
        {
          std::string retransmission_interval =
            std::string("retransmission_interval=") + it->second;
          idebug_configuration->set_option(retransmission_interval.c_str());
        }

        // Additional unicast sockets.
        it = network_params.find("additional_unicast_sockets");
        if (it != network_params.end())
        {
          std::string additional_unicast_sockets =
            std::string("additional_unicast_sockets=") + it->second;
          idebug_configuration->set_option(additional_unicast_sockets.c_str());
        }

        // Retention time.
        it = network_params.find("retention");
        if (it != network_params.end())
        {
          std::string retention = std::string("retention=") + it->second;
          idebug_configuration->set_option(retention.c_str());
        }

        // Alive factor.
        it = network_params.find("alive_factor");
        if (it != network_params.end())
        {
          std::string alive_factor = std::string("alive_factor=") + it->second;
          idebug_configuration->set_option(alive_factor.c_str());
        }
      }
    }
  }

  {
    // Debug configuration.
    mi::base::Handle<nv::index::IIndex_debug_configuration> idebug_configuration(
      m_nvindex_interface->get_api_component<nv::index::IIndex_debug_configuration>());
    assert(idebug_configuration.is_valid_interface());

    // Reduce log output
    idebug_configuration->set_option("debug_configuration_quiet=yes");
    // Set optimized flags
    idebug_configuration->set_option("integration_flags=8");

    // Disable features not used by the plugin
    idebug_configuration->set_option("disable_picking=1");

    // Don't pre-allocate buffers for rasterizer
    idebug_configuration->set_option("rasterizer_memory_allocation=-1");

    // No need to reset CUDA devices on shutdown
    idebug_configuration->set_option("cuda_device_reset_disable=1");

    // Disable timeseries data prefetch.
    idebug_configuration->set_option("timeseries_data_prefetch_disable=1");

    // Disable IndeX parallel importing, given importers are already parallelized.
    idebug_configuration->set_option("async_subset_load=0");

    // Use strict domain subdivision only with multiple ranks.
    if (vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() > 1)
      idebug_configuration->set_option("use_strict_domain_subdivision=1");

#ifdef VTKNVINDEX_USE_KDTREE
#if (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR <= 348900)

    // Enable kd-tree affinity
    idebug_configuration->set_option("use_kdtree_subdivision=1");

    // TODO: Should this be set based on the number of GPUs when no MPI.
    idebug_configuration->set_option("subdivision_parts=4");

#if 0
    idebug_configuration->set_option("dump_kdtree_subdivision=1");
#endif

#endif // (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR <= 348900)

#if 0
    // Debug kd-tree
    idebug_configuration->set_option("debug_kdtree_subdivision=1");
#endif
#endif // VTKNVINDEX_USE_KDTREE

    // Use pinned memory for staging buffer (enabled by default).
    if (use_config_file)
    {
      std::map<std::string, std::string> index_params;
      if (xml_parser.get_section_settings(index_params, "index"))
      {
        std::map<std::string, std::string>::iterator it;

        it = index_params.find("use_pinned_staging_buffer");
        if (it != index_params.end())
        {
          std::string use_pinned_staging_buffer("svol_disable_pinned_staging_buffer=");

          if (it->second == std::string("1") || it->second == std::string("yes"))
            use_pinned_staging_buffer += std::string("1");
          else
            use_pinned_staging_buffer += std::string("0");

          idebug_configuration->set_option(use_pinned_staging_buffer.c_str());
        }
      }
    }

    // Extra configuration from "Settings" dialog.
    vtknvindex_global_settings* settings = vtknvindex_global_settings::GetInstance();
    mi::base::Handle<mi::neuraylib::IDebug_configuration> dice_debug_configuration(
      m_nvindex_interface->get_api_component<mi::neuraylib::IDebug_configuration>());
    for (int i = 0; i < settings->GetNumberOfExtraConfigurations(); ++i)
    {
      const std::string cfg = settings->GetExtraConfiguration(i);
      if (cfg.empty())
      {
        continue;
      }

      bool success = true;
      if (cfg.rfind("dice.", 0) == 0)
      {
        // Handle options starting with "dice."
        success = (dice_debug_configuration->set_option(cfg.substr(5).c_str()) == 0);
      }
      else
      {
        success = (idebug_configuration->set_option(cfg.c_str()) == 0);
      }

      if (!success)
      {
        ERROR_LOG << "Failed to set NVIDIA IndeX extra configuration option '" << cfg
                  << "', specified in the 'Settings' dialog.";
      }
    }
  }

  // Register serializable classes.
  {
    bool is_registered = false;

    is_registered =
      m_nvindex_interface->register_serializable_class<vtknvindex_irregular_volume_importer>();
    assert(is_registered);

    is_registered =
      m_nvindex_interface->register_serializable_class<vtknvindex_sparse_volume_importer>();
    assert(is_registered);

    is_registered = m_nvindex_interface->register_serializable_class<vtknvindex_affinity>();
    assert(is_registered);

    is_registered = m_nvindex_interface->register_serializable_class<vtknvindex_KDTree_affinity>();
    assert(is_registered);

    is_registered = m_nvindex_interface->register_serializable_class<vtknvindex_volume_compute>();
    assert(is_registered);
  }

  // Start the NVIDIA IndeX library.
  const mi::Uint32 start_result = m_nvindex_interface->start(true);
  if (start_result != 0)
  {
    ERROR_LOG << "Fatal: Could not start NVIDIA IndeX library (error code " << start_result << "), "
              << "see log messages above for details.";
    return false;
  }

  // Synchronize IndeX viewer with remote instances.
  if (inetwork_configuration->get_mode() != mi::neuraylib::INetwork_configuration::MODE_OFF)
  {
    // IndeX viewer must wait until the remote nodes are connected
    if (vtkMultiProcessController::GetGlobalController()->GetLocalProcessId() == 0)
    {
      mi::base::Handle<nv::index::ICluster_configuration> icluster_configuration(
        m_nvindex_interface->get_api_component<nv::index::ICluster_configuration>());

      const mi::Uint32 cluster_size = static_cast<mi::Uint32>(m_host_list.size());
      if (cluster_size > 0)
      {
        mi::Uint32 old_nb_hosts = 0;
        mi::Uint32 nb_hosts = 0;
        INFO_LOG << "Waiting until cluster size reaches " << cluster_size << " hosts";
        while (nb_hosts < cluster_size)
        {
          nb_hosts = icluster_configuration->get_number_of_hosts();
          if (nb_hosts > old_nb_hosts)
          {
            INFO_LOG << "Cluster now has " << nb_hosts << " host" << (nb_hosts == 1 ? "" : "s")
                     << ", need " << (cluster_size - std::min(nb_hosts, cluster_size))
                     << " more to continue";
            old_nb_hosts = nb_hosts;
          }
          else if (nb_hosts < old_nb_hosts)
          {
            INFO_LOG << "Aborting because at least one host has left - had " << old_nb_hosts
                     << " hosts, only " << nb_hosts << " left";
            return false;
          }
          vtknvindex::util::sleep(0.5f);
        }
        INFO_LOG << "Cluster size " << cluster_size << " reached, continuing...";
      }
    }
  }

  return true;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_instance::shutdown_nvindex()
{
  if (m_nvindex_interface.is_valid_interface())
  {
    // Unregister receiving logger.
    mi::base::Handle<mi::neuraylib::ILogging_configuration> logging_configuration(
      m_nvindex_interface->get_api_component<mi::neuraylib::ILogging_configuration>());
    logging_configuration->set_receiving_logger(nullptr);
  }

  m_database = nullptr;
  m_global_scope = nullptr;
  m_iindex_session = nullptr;
  m_iindex_rendering = nullptr;
  m_icluster_configuration = nullptr;
  m_iindex_debug_configuration = nullptr;
  m_session_tag = mi::neuraylib::NULL_TAG;

  m_nvindex_interface->shutdown();
  m_nvindex_interface = nullptr;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_instance::initialize_session()
{
  {
    m_database = m_nvindex_interface->get_api_component<mi::neuraylib::IDatabase>();
    assert(m_database.is_valid_interface());

    m_global_scope = m_database->get_global_scope();
    assert(m_global_scope.is_valid_interface());

    m_iindex_session = m_nvindex_interface->get_api_component<nv::index::IIndex_session>();
    assert(m_iindex_session.is_valid_interface());

    m_iindex_rendering = m_nvindex_interface->create_rendering_interface();
    assert(m_iindex_rendering.is_valid_interface());

    m_icluster_configuration =
      m_nvindex_interface->get_api_component<nv::index::ICluster_configuration>();
    assert(m_icluster_configuration.is_valid_interface());

    m_iindex_debug_configuration =
      m_nvindex_interface->get_api_component<nv::index::IIndex_debug_configuration>();
    assert(m_iindex_debug_configuration.is_valid_interface());
  }

  {
    mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
      m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
    assert(dice_transaction.is_valid_interface());

    m_session_tag = m_iindex_session->create_session(dice_transaction.get());
    assert(m_session_tag.is_valid());

    dice_transaction->commit();
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_instance::init_scene_graph()
{
  // DiCE database access.
  mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
    m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
  assert(dice_transaction.is_valid_interface());

  // Create scene graph
  {
    // Access the session instance from the database.
    mi::base::Handle<const nv::index::ISession> session(
      dice_transaction->access<const nv::index::ISession>(m_session_tag));
    assert(session.is_valid_interface());

    // Access (edit mode) the scene instance from the database.
    mi::base::Handle<nv::index::IScene> scene(
      dice_transaction->edit<nv::index::IScene>(session->get_scene()));
    assert(scene.is_valid_interface());

    // Create volumes colormap
    mi::base::Handle<nv::index::IColormap> volume_colormap(
      scene->create_attribute<nv::index::IColormap>());
    assert(volume_colormap.is_valid_interface());

    m_volume_colormap_tag = dice_transaction->store_for_reference_counting(
      volume_colormap.get(), mi::neuraylib::NULL_TAG, "volume_colormap");
    assert(m_volume_colormap_tag.is_valid());

    // Create slices colormap
    mi::base::Handle<nv::index::IColormap> slice_colormap(
      scene->create_attribute<nv::index::IColormap>());
    assert(volume_colormap.is_valid_interface());

    m_slice_colormap_tag = dice_transaction->store_for_reference_counting(
      slice_colormap.get(), mi::neuraylib::NULL_TAG, "slice_colormap");
    assert(m_slice_colormap_tag.is_valid());

    // Create geom group, parent for all volumes and slices in the scene
    mi::base::Handle<nv::index::IStatic_scene_group> geom_group(
      scene->create_scene_group<nv::index::IStatic_scene_group>());
    assert(geom_group.is_valid_interface());

    m_geom_group_tag = dice_transaction->store_for_reference_counting(
      geom_group.get(), mi::neuraylib::NULL_TAG, "geom_group");
    assert(m_geom_group_tag.is_valid());

    // Create scene light (head light).
    mi::base::Handle<nv::index::IDirectional_headlight> light(
      scene->create_attribute<nv::index::IDirectional_headlight>());

    mi::math::Vector_struct<mi::Float32, 3> light_direction = { 0, 0, 1 };
    light->set_direction(light_direction);
    mi::math::Color_struct light_intensity = { 1, 1, 1 };
    light->set_intensity(light_intensity);

    const mi::neuraylib::Tag light_tag = dice_transaction->store_for_reference_counting(
      light.get(), mi::neuraylib::NULL_TAG, "scene_light");
    assert(light_tag.is_valid());

    // Create colormaps groups
    mi::base::Handle<nv::index::IStatic_scene_group> colormaps_group(
      scene->create_scene_group<nv::index::IStatic_scene_group>());
    assert(colormaps_group.is_valid_interface());

    colormaps_group->append(m_slice_colormap_tag, dice_transaction.get());

    const mi::neuraylib::Tag colormaps_group_tag = dice_transaction->store_for_reference_counting(
      colormaps_group.get(), mi::neuraylib::NULL_TAG, "colormaps_group");
    assert(colormaps_group_tag.is_valid());

    // Add all new created elements to scene graph
    scene->append(m_volume_colormap_tag, dice_transaction.get());
    scene->append(light_tag, dice_transaction.get());
    scene->append(m_geom_group_tag, dice_transaction.get());
    scene->append(colormaps_group_tag, dice_transaction.get());

    // Create perspective and parallel cameras to be exchangables.
    m_perspective_camera_tag =
      session->create_camera(dice_transaction.get(), nv::index::IPerspective_camera::IID());
    assert(m_perspective_camera_tag.is_valid());

    m_parallel_camera_tag =
      session->create_camera(dice_transaction.get(), nv::index::IOrthographic_camera::IID());
    assert(m_parallel_camera_tag.is_valid());
  }

  if (m_is_index_viewer)
    m_nvindex_colormaps = new vtknvindex_colormap(m_volume_colormap_tag, m_slice_colormap_tag);

  dice_transaction->commit();
}

//-------------------------------------------------------------------------------------------------
const char* vtknvindex_instance::get_version() const
{
  return "6.0.0";
}
