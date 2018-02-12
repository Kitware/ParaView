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

#ifdef WIN32
#include <Windows.h>
#else // WIN32
#include <dlfcn.h>
#endif // WIN32

#include "vtksys/SystemTools.hxx"

#include "vtknvindex_application.h"
#include "vtknvindex_clock_pulse_generator.h"
#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_config_settings.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_irregular_volume_importer.h"
#include "vtknvindex_receiving_logger.h"
#include "vtknvindex_regular_volume_properties.h"
#include "vtknvindex_utilities.h"
#include "vtknvindex_volume_importer.h"

//-------------------------------------------------------------------------------------------------
vtknvindex_application::vtknvindex_application()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_application::~vtknvindex_application()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
mi::base::Handle<nv::index::IIndex>& vtknvindex_application::get_interface()
{
  return m_nvindex_interface;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_application::load_nvindex_library()
{
  // Load shared libraries.
  std::string lib_name = "libnvindex";

#ifdef WIN32
#ifdef DEBUG
  lib_name += "d.dll";
#else  //  DEBUG
  lib_name += ".dll";
#endif // DEBUG
#else  // WIN32
  lib_name += ".so";
#endif // WIN32

#ifdef WIN32
  m_p_handle = LoadLibrary(TEXT(lib_name.c_str()));
  if (!m_p_handle)
  {
    ERROR_LOG << "Unable to retrieve handle to the NVIDIA IndeX library.";
    return false;
  }
  void* symbol = GetProcAddress((HMODULE)m_p_handle, "nv_index_factory");
  if (!symbol)
  {
    ERROR_LOG << "Unable to retrieve the entry point into the " << lib_name.c_str() << " library.";
    return false;
  }
#else // WIN32
  m_p_handle = dlopen(lib_name.c_str(), RTLD_LAZY);
  if (!m_p_handle)
  {
    ERROR_LOG << "Unable to retrieve handle to the NVIDIA IndeX library: " << dlerror() << ".";
    return false;
  }

  void* symbol = dlsym(m_p_handle, "nv_index_factory");
  if (!symbol)
  {
    ERROR_LOG << "Unable to retrieve the entry point into the " << lib_name.c_str() << " library.";
    return false;
  }

#endif // WIN32

  m_nvindexlib_fname = lib_name;

  typedef nv::index::IIndex*(IIndex_factory());
  IIndex_factory* factory = (IIndex_factory*)symbol;

  m_nvindex_interface = factory();
  if (!m_nvindex_interface.is_valid_interface())
  {
    ERROR_LOG << "Failed to Initialize NVIDIA IndeX library.";

#ifdef WIN32
    ERROR_LOG << "Please verify that the PATH variable has been set appropriately and points to "
                 "the location of the NVIDIA IndeX libraries.";
#else  // WIN32
    ERROR_LOG << "Please verify that the environment variable LD_LIBRARY_PATH has been set "
                 "appropriately and points to the location of the NVIDIA IndeX libraries.";
#endif // WIN32
    return false;
  }

  // Check for license and authenticate.
  if (!authenticate_nvindex_library())
  {
    ERROR_LOG
      << "Failed to authenticate NVIDIA IndeX library, please provide a valid license file.\n";
    return false;
  }

  // Initialize logging through NVIDIA IndeX.
  vtknvindex::logger::vtknvindex_forwarding_logger_factory::instance()->initialize(
    m_nvindex_interface);

  {
    mi::base::Handle<mi::neuraylib::ILogging_configuration> logging_configuration(
      m_nvindex_interface->get_api_component<mi::neuraylib::ILogging_configuration>());
    assert(logging_configuration.is_valid_interface());

    logging_configuration->set_log_locally(true); // local logging

    // Install the receiving logger.
    mi::base::Handle<mi::base::ILogger> receiving_logger(
      new vtknvindex::logger::vtknvindex_receiving_logger());
    assert(receiving_logger.is_valid_interface());
    logging_configuration->set_receiving_logger(receiving_logger.get());
  }

  // Access and log NVIDIA IndeX version.
  INFO_LOG << "NVIDIA IndeX " << m_nvindex_interface->get_version() << " (build "
           << m_nvindex_interface->get_revision() << ").";

  return true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_application::authenticate_nvindex_library()
{

  std::string index_vendor_key, index_secret_key;
  std::string flexnet_lic_path("/path/to/flexnet/");

  bool found_license = false;

  // Try reading license from environment.
  vtksys::SystemTools sys_tools;
  const char* env_vendor_key = sys_tools.GetEnv("NVINDEX_VENDOR_KEY");
  const char* env_secret_key = sys_tools.GetEnv("NVINDEX_SECRET_KEY");

  if (env_vendor_key != NULL && env_secret_key != NULL)
  {
    index_vendor_key = env_vendor_key;
    index_secret_key = env_secret_key;
    found_license = true;
  }
  else // Try reading license from config file.
  {
    vtknvindex_xml_config_parser xml_parser;
    if (xml_parser.open_config_file("nvindex_config.xml"))
      found_license = xml_parser.get_license_strings(index_vendor_key, index_secret_key);
  }

  // No explicit license found, use default free license.
  if (!found_license)
  {
    index_vendor_key = "NVIDIA IndeX License for ParaView - IndeX:PV:Free:v1 - 20180307 "
                       "(oem:retail_cloud.20190331)";
    index_secret_key = "776532b335999674f8492574fbccd8defeac1b30e9804d5d42b383c57dcfab8f";
  }

  return (m_nvindex_interface->authenticate(index_vendor_key.c_str(), index_vendor_key.length(),
            index_secret_key.c_str(), index_secret_key.length(), flexnet_lic_path.c_str(),
            flexnet_lic_path.length()) == 0);
}

//-------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_application::setup_nvindex_library(const std::vector<std::string>& host_names)
{
  vtknvindex_xml_config_parser xml_parser;
  bool use_config_file = xml_parser.open_config_file("nvindex_config.xml");

  // Configure networking before starting the IndeX library.
  mi::base::Handle<mi::neuraylib::INetwork_configuration> inetwork_configuration(
    m_nvindex_interface->get_api_component<mi::neuraylib::INetwork_configuration>());
  assert(inetwork_configuration.is_valid_interface());

  // Networking is off by default.
  inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_OFF);

  if (use_config_file)
  {
    std::map<std::string, std::string> network_params;
    if (xml_parser.get_section_settings(network_params, "network"))
    {
      std::map<std::string, std::string>::iterator it;

      // Cluster network mode (protocol).
      it = network_params.find("cluster_mode");
      if (it != network_params.end())
      {
        const std::string cluster_mode(it->second);

        if (cluster_mode == "OFF")
        {
          inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_OFF);
        }
        else if (cluster_mode == "TCP")
        {
          inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_TCP);
        }
        else if (cluster_mode == "UDP")
        {
          inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_UDP);

          // Multicast address.
          it = network_params.find("multicast_address");
          if (it != network_params.end())
          {
            const std::string multicast_address(it->second);
            inetwork_configuration->set_multicast_address(multicast_address.c_str());
          }
          else
          {
            // Use default multicast address.
            inetwork_configuration->set_multicast_address("224.1.3.2");
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
            inetwork_configuration->set_discovery_address(discovery_address.c_str());
          }
          else
          {
            // use default discovery address
            inetwork_configuration->set_discovery_address("224.1.3.3:5555");
          }
        }
      }
      else
      {
        // Use default cluster mode, which is "OFF".
        inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_OFF);
      }

      if (inetwork_configuration->get_mode() != mi::neuraylib::INetwork_configuration::MODE_OFF)
      {
        // Cluster interface address.
        it = network_params.find("cluster_interface_address");
        if (it != network_params.end())
        {
          const std::string cluster_interface_address(it->second);
          inetwork_configuration->set_cluster_interface(cluster_interface_address.c_str());
        }

        // use RDMA.
        it = network_params.find("use_rdma");
        if (it != network_params.end())
        {
          const std::string use_rdma(it->second);
          inetwork_configuration->set_use_rdma(
            use_rdma == std::string("1") || use_rdma == std::string("yes"));
        }
        else
        {
          // Use RDMA by default.
          inetwork_configuration->set_use_rdma(true);
        }
      }
    }
    else if (host_names.size() > 1) // Use automatic cluster configuration.
    {
      inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_TCP);
      for (mi::Uint32 i = 0; i < host_names.size(); ++i)
      {
        inetwork_configuration->add_configured_host(host_names[i].c_str());
      }
    }
  }
  else if (host_names.size() > 1) // Use automatic cluster configuration.
  {
    inetwork_configuration->set_mode(mi::neuraylib::INetwork_configuration::MODE_TCP);
    for (mi::Uint32 i = 0; i < host_names.size(); ++i)
    {
      inetwork_configuration->add_configured_host(host_names[i].c_str());
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
        std::string retransmission_interval = std::string("retransmission_interval=") + it->second;
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

  // Register serializable classes.
  {
    bool is_registered = false;

    is_registered = m_nvindex_interface->register_serializable_class<vtknvindex_volume_importer>();
    assert(is_registered);

    is_registered =
      m_nvindex_interface->register_serializable_class<vtknvindex_irregular_volume_importer>();
    assert(is_registered);

    is_registered = m_nvindex_interface->register_serializable_class<vtknvindex_affinity>();
    assert(is_registered);

    is_registered =
      m_nvindex_interface->register_serializable_class<vtknvindex_clock_pulse_generator>();
    assert(is_registered);

    is_registered =
      m_nvindex_interface->register_serializable_class<vtknvindex_cluster_properties>();
    assert(is_registered);

    is_registered = m_nvindex_interface->register_serializable_class<vtknvindex_host_properties>();
    assert(is_registered);

    is_registered =
      m_nvindex_interface->register_serializable_class<vtknvindex_regular_volume_properties>();
    assert(is_registered);
  }

  // Starting the NVIDIA IndeX library.
  mi::Uint32 start_result = 0;
  if (start_result = m_nvindex_interface->start(true) != 0)
  {
    ERROR_LOG << "Start of the NVIDIA IndeX library failed.";
  }

  return start_result;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_application::initialize_arc()
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

  // Verifying it the local host has joined.
  // This may fail if there is a license problem.
  assert(is_local_host_joined());

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
bool vtknvindex_application::is_valid() const
{
  bool ret = true;

  ret = ret && m_database.is_valid_interface();
  ret = ret && m_global_scope.is_valid_interface();
  ret = ret && m_iindex_session.is_valid_interface();
  ret = ret && m_iindex_rendering.is_valid_interface();
  ret = ret && m_icluster_configuration.is_valid_interface();

  return ret;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_application::is_local_host_joined() const
{
  assert(m_icluster_configuration.is_valid_interface());
  const int max_retry = 3;
  for (int retry = 0; retry < max_retry; ++retry)
  {
    if (m_icluster_configuration->get_number_of_hosts() == 0)
    {
      INFO_LOG << "No host has joined yet, retrying ...";
      vtknvindex::util::sleep(0.2f); // wait for 0.2 second
    }
  }
  return (m_icluster_configuration->get_number_of_hosts() != 0);
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_application::shutdown()
{
  if (m_nvindex_interface.is_valid_interface())
  {
    // Unregister receiving logger.
    mi::base::Handle<mi::neuraylib::ILogging_configuration> logging_configuration(
      m_nvindex_interface->get_api_component<mi::neuraylib::ILogging_configuration>());
    logging_configuration->set_receiving_logger(0);
  }

  m_database = 0;
  m_global_scope = 0;
  m_iindex_session = 0;
  m_iindex_rendering = 0;
  m_icluster_configuration = 0;
  m_iindex_debug_configuration = 0;
  m_session_tag = mi::neuraylib::NULL_TAG;

  const mi::Sint32 nvindex_shutdown = m_nvindex_interface->shutdown();

  if (nvindex_shutdown != 0)
    ERROR_LOG << "Failed to shutdown the NVIDIA IndeX library (code: " << nvindex_shutdown << ").";

  m_nvindex_interface = 0;

  return nvindex_shutdown == 0;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_application::unload_iindex()
{
  assert(m_p_handle != 0);

#ifdef WIN32
  if (TRUE != FreeLibrary((HMODULE)m_p_handle))
  {
    const std::string nvindex_fname = m_nvindexlib_fname;
    ERROR_LOG << "Failed to unload the NVIDIA IndeX library (" << nvindex_fname << ").";
    return false;
  }
#else  // WIN32
  int result = dlclose(m_p_handle);
  if (result != 0)
  {
    ERROR_LOG << "Failed to unload the NVIDIA IndeX library: " << dlerror() << ".";
    return false;
  }
#endif // WIN32

  return true;
}
