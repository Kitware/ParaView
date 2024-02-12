// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include <catalyst.h>
#include <catalyst_api.h>
#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>
#include <catalyst_stub.h>

#include "vtkCallbackCommand.h"
#include "vtkCatalystBlueprint.h"
#include "vtkCommand.h"
#include "vtkConduitSource.h"
#include "vtkDataObjectToConduit.h"
#include "vtkInSituInitializationHelper.h"
#include "vtkInSituPipelineIO.h"
#include "vtkInSituPipelinePython.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPVLogger.h"
#include "vtkPartitionedDataSet.h"
#include "vtkSMPluginManager.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#endif

#if VTK_MODULE_ENABLE_VTK_IOIOSS
#include "vtkIOSSReader.h"
#endif

#if VTK_MODULE_ENABLE_VTK_IOFides
#include "vtkFidesReader.h"
#endif

#include "catalyst_impl_paraview.h"

static bool update_producer_mesh_blueprint(const std::string& channel_name,
  const conduit_node* node, const conduit_node* global_fields, bool multimesh,
  const conduit_node* assemblyNode, bool multiblock, bool amr)
{
  auto producer = vtkInSituInitializationHelper::GetProducer(channel_name);
  if (producer == nullptr)
  {
    auto pxm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    producer = vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "Conduit"));
    if (!producer)
    {
      vtkLogF(ERROR, "Failed to create 'Conduit' proxy!");
      return false;
    }
    vtkInSituInitializationHelper::SetProducer(channel_name, producer);
    producer->Delete();
  }

  auto algo = vtkConduitSource::SafeDownCast(producer->GetClientSideObject());
  algo->SetNode(node);
  algo->SetGlobalFieldsNode(global_fields);
  algo->SetUseMultiMeshProtocol(multimesh);
  algo->SetOutputMultiBlock(multiblock);
  algo->SetAssemblyNode(assemblyNode);
  algo->SetUseAMRMeshProtocol(amr);
  vtkInSituInitializationHelper::MarkProducerModified(channel_name);
  return true;
}

static vtkSmartPointer<vtkInSituPipeline> create_precompiled_pipeline(const conduit_cpp::Node& node)
{
  if (node["type"].as_string() == "io")
  {
    vtkNew<vtkInSituPipelineIO> pipeline;
    pipeline->SetFileName(node["filename"].as_string().c_str());
    pipeline->SetChannelName(node["channel"].as_string().c_str());
    return pipeline;
  }
  else
  {
    return nullptr;
  }
}

static bool update_producer_ioss(const std::string& channel_name, const conduit_cpp::Node* node,
  const conduit_cpp::Node* vtkNotUsed(global_fields))
{
#if VTK_MODULE_ENABLE_VTK_IOIOSS
  auto producer = vtkInSituInitializationHelper::GetProducer(channel_name);
  if (producer == nullptr)
  {
    auto pxm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    producer = vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "IOSSReader"));
    if (!producer)
    {
      vtkLogF(ERROR, "Failed to create 'Conduit' proxy!");
      return false;
    }
    vtkSMPropertyHelper(producer, "FileName").Set("catalyst.bin");
    producer->UpdateVTKObjects();
    vtkInSituInitializationHelper::SetProducer(channel_name, producer);
    producer->Delete();
  }

  auto algo = vtkIOSSReader::SafeDownCast(producer->GetClientSideObject());
  algo->SetDatabaseTypeOverride("catalyst");
  algo->AddProperty(
    "CATALYST_CONDUIT_NODE", conduit_cpp::c_node(const_cast<conduit_cpp::Node*>(node)));
  /*
  algo->SetGlobalFieldsNode(global_fields);
  */
  vtkInSituInitializationHelper::MarkProducerModified(channel_name);
  return true;
#else
  (void)channel_name;
  (void)node;
  return false;
#endif
}

static bool update_producer_fides(
  const std::string& channel_name, const conduit_cpp::Node& node, double& time)
{
#if VTK_MODULE_ENABLE_VTK_IOFides
  auto producer = vtkInSituInitializationHelper::GetProducer(channel_name);
  if (producer == nullptr)
  {
    auto pxm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    producer = vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "FidesJSONReader"));
    if (!producer)
    {
      vtkLogF(ERROR, "Failed to create 'Fides' proxy!");
      return false;
    }

    vtkSMPropertyHelper(producer, "FileName").Set(node["json_file"].as_string().c_str());
    vtkSMPropertyHelper(producer, "DataSourceIO")
      .Set(0, node["data_source_io/source"].as_string().c_str());
    vtkSMPropertyHelper(producer, "DataSourceIO")
      .Set(1, node["data_source_io/address"].as_string().c_str());
    vtkSMPropertyHelper(producer, "DataSourcePath")
      .Set(0, node["data_source_path/source"].as_string().c_str());
    vtkSMPropertyHelper(producer, "DataSourcePath")
      .Set(1, node["data_source_path/path"].as_string().c_str());
    vtkInSituInitializationHelper::SetProducer(channel_name, producer);
    producer->UpdateVTKObjects();
    // Required so that vtkFidesReader will setup the inline reader
    producer->UpdatePipelineInformation();
    producer->Delete();
  }
  auto algo = vtkFidesReader::SafeDownCast(producer->GetClientSideObject());
  algo->PrepareNextStep();
  time = algo->GetTimeOfCurrentStep();
  vtkInSituInitializationHelper::MarkProducerModified(channel_name);
  return true;
#else
  (void)channel_name;
  (void)time;
  return false;
#endif
}

static bool process_script_args(vtkInSituPipelinePython* pipeline, const conduit_cpp::Node& node)
{
  std::vector<std::string> args;
  conduit_index_t nchildren = node.number_of_children();
  for (conduit_index_t i = 0; i < nchildren; ++i)
  {
    args.push_back(node.child(i).as_string());
  }
  pipeline->SetArguments(args);
  return true;
}

/**
 * Fill a conduit node's channel with a specified name with the dataset given by the proxy's
 * ClientSideObject. Return true if the mesh conversion to conduit was successful if it happened,
 * even if the channel is empty.
 */
static bool convert_to_blueprint_mesh(
  vtkSMProxy* proxy, const std::string& name, conduit_cpp::Node& node)
{
  if (proxy == nullptr)
  {
    return true;
  }

  auto sourceDataset = vtkAlgorithm::SafeDownCast(proxy->GetClientSideObject());
  if (sourceDataset == nullptr)
  {
    return true;
  }

  sourceDataset->Update();
  vtkDataObject* outputDataObject = sourceDataset->GetOutputDataObject(0);
  if (outputDataObject == nullptr)
  {
    return true;
  }

  conduit_cpp::Node channel = node[name];
  if (auto multi_block = vtkMultiBlockDataSet::SafeDownCast(outputDataObject))
  {
    if (auto data_object = multi_block->GetBlock(0))
    {
      return vtkDataObjectToConduit::FillConduitNode(data_object, channel);
    }
  }
  else if (auto partitioned = vtkPartitionedDataSet::SafeDownCast(outputDataObject))
  {
    return vtkDataObjectToConduit::FillConduitNode(
      partitioned->GetPartitionAsDataObject(0), channel);
  }

  return vtkDataObjectToConduit::FillConduitNode(outputDataObject, channel);
}

enum paraview_catalyst_status
{
  paraview_catalyst_status_invalid_node = 100,
  paraview_catalyst_status_results = 101,
};
#define pvcatalyst_err(name) static_cast<enum catalyst_status>(paraview_catalyst_status_##name)

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_initialize_paraview(const conduit_node* params)
{
  vtkLogger::Init();
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());

  const conduit_cpp::Node cpp_params = conduit_cpp::cpp_node(const_cast<conduit_node*>(params));
  if (!cpp_params.has_path("catalyst"))
  {
    // no catalyst params specified, right now, am not sure if this is a error.
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "'catalyst' node node present. using default initialization params.");
  }
  else if (!vtkCatalystBlueprint::Verify("initialize", cpp_params["catalyst"]))
  {
    vtkLogF(
      ERROR, "invalid 'catalyst' node passed to 'catalyst_initialize'. Initialization failed.");
    return pvcatalyst_err(invalid_node);
  }

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  static_assert(sizeof(MPI_Fint) <= sizeof(vtkTypeUInt64),
    "MPI_Fint size is greater than 64bit! That is not supported.");
  vtkTypeUInt64 comm = 0;
  int isMPIInitialized = 0;
  if (MPI_Initialized(&isMPIInitialized) == MPI_SUCCESS && isMPIInitialized)
  {
    comm = static_cast<vtkTypeUInt64>(MPI_Comm_c2f(MPI_COMM_WORLD));
    if (cpp_params.has_path("catalyst/mpi_comm"))
    {
      comm = cpp_params["catalyst/mpi_comm"].to_int64();
    }
  }
#else
  const vtkTypeUInt64 comm = 0;
#endif
  vtkInSituInitializationHelper::Initialize(comm);

  if (cpp_params.has_path("catalyst/scripts"))
  {
    if (vtkInSituInitializationHelper::IsPythonSupported())
    {
      auto& scripts = cpp_params["catalyst/scripts"];
      conduit_index_t nchildren = scripts.number_of_children();
      for (conduit_index_t i = 0; i < nchildren; ++i)
      {
        auto script = scripts.child(i);
        const auto fname =
          script.dtype().is_string() ? script.as_string() : script["filename"].as_string();

        vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Analysis script: '%s'", fname.c_str());

        auto pipeline = vtkInSituInitializationHelper::AddPipeline(script.name(), fname);

        // check for optional 'args'
        if (script.has_path("args"))
        {
          ::process_script_args(vtkInSituPipelinePython::SafeDownCast(pipeline), script["args"]);
        }
      }
    }
    else
    {
      vtkLogF(WARNING, "Python support not enabled, 'catalyst/scripts' are ignored.");
    }
  }

  if (cpp_params.has_path("catalyst/proxies"))
  {
    auto plmgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();

    const auto& proxies = cpp_params["catalyst/proxies"];
    conduit_index_t nchildren = proxies.number_of_children();
    for (conduit_index_t i = 0; i < nchildren; ++i)
    {
      auto proxy = proxies.child(i);
      const auto proxyFilename =
        proxy.dtype().is_string() ? proxy.as_string() : proxy["filename"].as_string();

      if (!plmgr->LoadLocalPlugin(proxyFilename.c_str()))
      {
        vtkLog(ERROR, "Failed to load plugin xml " << proxyFilename);
      }
      else
      {
        vtkLog(INFO, "Catalyst plugin loaded: " << proxyFilename);
      }
    }
  }

  if (cpp_params.has_path("catalyst/pipelines"))
  {
    auto& pipelines = cpp_params["catalyst/pipelines"];
    conduit_index_t nchildren = pipelines.number_of_children();
    for (conduit_index_t i = 0; i < nchildren; ++i)
    {
      if (auto p = create_precompiled_pipeline(pipelines.child(i)))
      {
        p->SetName(pipelines.child(i).name().c_str());
        vtkInSituInitializationHelper::AddPipeline(p);
      }
    }
  }

  if (!cpp_params.has_path("catalyst/scripts") && !cpp_params.has_path("catalyst/pipelines"))
  {
    // no catalyst initialization specified.
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "No Catalyst Python scripts or pre-compiled pipelines specified. No "
      "analysis pipelines will be executed.");
  }

  return catalyst_status_ok;
}

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_execute_paraview(const conduit_node* params)
{
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());

  const conduit_cpp::Node cpp_params = conduit_cpp::cpp_node(const_cast<conduit_node*>(params));
  if (!cpp_params.has_path("catalyst"))
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Path 'catalyst' is not provided. Skipping.");
    return pvcatalyst_err(invalid_node);
  }

  const auto& root = cpp_params["catalyst"];
  if (!vtkCatalystBlueprint::Verify("execute", root))
  {
    vtkLogF(ERROR, "invalid 'catalyst' node passed to 'catalyst_execute'. Execution failed.");
    return pvcatalyst_err(invalid_node);
  }

  // catalyst/timestep or catalyst/cycle is used to indicate the timestep
  // catalyst/time is used to provide the time
  const int timestep = root.has_path("state/timestep")
    ? root["state/timestep"].to_int64()
    : (root.has_path("state/cycle") ? root["state/cycle"].to_int64() : 0);
  double time = root.has_path("state/time") ? root["state/time"].to_float64() : 0;

  const int output_multiblock =
    root.has_path("state/multiblock") ? root["state/multiblock"].to_int() : 0;

  vtkVLogScopeF(
    PARAVIEW_LOG_CATALYST_VERBOSITY(), "co-processing for timestep=%d, time=%f", timestep, time);

  conduit_cpp::Node globalFields;

  // catalyst/channels are used to communicate meshes.
  if (root.has_child("channels"))
  {
    const auto channels = root["channels"];
    const conduit_index_t nchildren = channels.number_of_children();
    for (conduit_index_t i = 0; i < nchildren; ++i)
    {
      const auto channel_node = channels.child(i);
      const std::string channel_name = channel_node.name();
      const std::string type = channel_node["type"].as_string();

      const auto data_node = channel_node["data"];
      bool is_valid = true;

      // check for optional channel state data
      const int channel_timestep = channel_node.has_path("state/timestep")
        ? channel_node["state/timestep"].to_int64()
        : (channel_node.has_path("state/cycle") ? channel_node["state/cycle"].to_int64()
                                                : timestep);

      const double channel_time =
        channel_node.has_path("state/time") ? channel_node["state/time"].to_float64() : time;

      const int channel_output_multiblock = channel_node.has_path("state/multiblock")
        ? channel_node["state/multiblock"].to_int()
        : output_multiblock;

      if (type == "mesh")
      {
        conduit_cpp::Node info;
        is_valid = conduit_cpp::Blueprint::verify("mesh", data_node, info);
        if (!is_valid)
        {
          vtkLogF(ERROR, "'data' on channel '%s' is not a valid 'mesh'; skipping channel.",
            channel_name.c_str());
        }
      }
      else if (type == "multimesh")
      {
        for (conduit_index_t didx = 0, dmax = data_node.number_of_children();
             didx < dmax && is_valid; ++didx)
        {
          const auto mesh_node = data_node.child(didx);
          conduit_cpp::Node info;
          is_valid = conduit_cpp::Blueprint::verify("mesh", mesh_node, info);
          if (!is_valid)
          {
            vtkLogF(ERROR, "'data/%s' on channel '%s' is not a valid 'mesh'; skipping channel.",
              mesh_node.name().c_str(), channel_name.c_str());
          }
        }

        if (channel_node.has_path("assembly"))
        {
          is_valid = vtkCatalystBlueprint::Verify("assembly", channel_node["assembly"]);
          if (!is_valid)
          {
            vtkLogF(ERROR, "'assembly' on channel '%s' is not valid; skipping channel.",
              channel_name.c_str());
          }
        }
      }
      else if (type == "ioss")
      {
#if VTK_MODULE_ENABLE_VTK_IOIOSS
        is_valid = true;
        vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
          "IOSS mesh detected for channel (%s); validation will be skipped for now",
          channel_name.c_str());
#else
        vtkLogF(ERROR, "IOSS mesh is not supported by this build. Rebuild with IOSS enabled.");
#endif
      }
      else if (type == "fides")
      {
#if VTK_MODULE_ENABLE_VTK_IOFides
        is_valid = true;
        vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
          "Fides mesh detected for channel (%s); validation will be skipped for now",
          channel_name.c_str());
#else
        vtkLogF(ERROR, "Fides mesh is not supported by this build. Rebuild with Fides enabled.");
#endif
      }
      else if (type == "amrmesh")
      {
        is_valid = true;
        vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
          "amrmesh mesh detected for channel (%s); validation will be skipped for now",
          channel_name.c_str());
      }
      else
      {
        is_valid = false;
        vtkLogF(ERROR, "channel '%s' has unsupported type '%s'; skipping.", channel_name.c_str(),
          type.c_str());
      }

      if (!is_valid)
      {
        continue; // skip this channel.
      }

      // populate field data node for this channel.
      auto fields = globalFields[channel_name];
      fields["time"].set(channel_time);
      fields["timestep"].set(channel_timestep);
      fields["cycle"].set(channel_timestep);
      fields["channel"].set(channel_name);
      if (type == "mesh" || type == "multimesh" || type == "amrmesh")
      {
        conduit_node* assembly = nullptr;
        if (channel_node.has_path("assembly"))
        {
          auto anode = channel_node["assembly"];
          assembly = conduit_cpp::c_node(&anode);
        }
        update_producer_mesh_blueprint(channel_name, conduit_cpp::c_node(&data_node),
          conduit_cpp::c_node(&fields), type == "multimesh", assembly,
          channel_output_multiblock != 0, type == "amrmesh");
      }
      else if (type == "ioss")
      {
        update_producer_ioss(channel_name, &data_node, &fields);
      }
      else if (type == "fides")
      {
        update_producer_fides(channel_name, cpp_params["catalyst/fides"], time);
      }

      // Set in situ mode. Temporal filters are notified that they don't have the whole time
      // series up front
      auto producer = vtkInSituInitializationHelper::GetProducer(channel_name);
      if (auto algo = vtkAlgorithm::SafeDownCast(producer->GetClientSideObject()))
      {
        algo->SetNoPriorTemporalAccessInformationKey();
      }
    }
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "No 'catalyst/channels' found. No meshes will be processed.");
  }

  vtkInSituInitializationHelper::ExecutePipelines(params);

  return catalyst_status_ok;
}

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_finalize_paraview(const conduit_node* params)
{
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());

  const conduit_cpp::Node cpp_params = conduit_cpp::cpp_node(const_cast<conduit_node*>(params));
  if (cpp_params.has_path("catalyst") &&
    !vtkCatalystBlueprint::Verify("finalize", cpp_params["catalyst"]))
  {
    vtkLogF(ERROR, "invalid 'catalyst' node passed to 'catalyst_finalize'. Finalization may fail.");
  }

  vtkInSituInitializationHelper::Finalize();

  return catalyst_status_ok;
}

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_about_paraview(conduit_node* params)
{
  catalyst_stub_about(params);
  conduit_cpp::Node cpp_params = conduit_cpp::cpp_node(params);
  cpp_params["catalyst"]["capabilities"].append().set("paraview");
  if (vtkInSituInitializationHelper::IsPythonSupported())
  {
    cpp_params["catalyst"]["capabilities"].append().set("python");
  }
  cpp_params["catalyst"]["implementation"] = "paraview";

  return catalyst_status_ok;
}

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_results_paraview(conduit_node* params)
{
  auto stub_error_status = catalyst_stub_results(params);

  if (stub_error_status != catalyst_status_ok)
  {
    return stub_error_status;
  }

  conduit_cpp::Node cpp_params = conduit_cpp::cpp_node(params);
  auto catalyst_node = cpp_params["catalyst"];

  bool is_success = true;
  std::vector<std::pair<std::string, vtkSMProxy*>> steerableProxies;
  vtkInSituInitializationHelper::GetSteerableProxies(steerableProxies);
  for (auto& proxy : steerableProxies)
  {
    is_success &= convert_to_blueprint_mesh(proxy.second, proxy.first, catalyst_node);
  }

  vtkInSituInitializationHelper::GetResultsFromPipelines(params);

  return is_success ? catalyst_status_ok : pvcatalyst_err(results);
}
