/*=========================================================================

  Program:   ParaView
  Module:    ParaViewCatalyst.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <catalyst.h>
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
#include "vtkSMPluginManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSteeringDataGenerator.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#endif

#include "catalyst_impl_paraview.h"

static bool update_producer_mesh_blueprint(const std::string& channel_name,
  const conduit_cpp::Node* node, const conduit_cpp::Node* global_fields, bool multimesh)
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
  algo->SetNode(conduit_cpp::c_node(node));
  algo->SetGlobalFieldsNode(conduit_cpp::c_node(global_fields));
  algo->SetUseMultiMeshProtocol(multimesh);
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

static bool convert_to_blueprint_mesh(
  vtkSMProxy* proxy, const std::string& name, conduit_cpp::Node& node)
{
  if (proxy != nullptr)
  {
    if (auto steeringDataGenerator =
          vtkSteeringDataGenerator::SafeDownCast(proxy->GetClientSideObject()))
    {
      steeringDataGenerator->Update();
      if (vtkDataObject* outputDataObject = steeringDataGenerator->GetOutput())
      {
        auto channel = node[name];

        if (auto multi_block = vtkMultiBlockDataSet::SafeDownCast(outputDataObject))
        {
          if (auto data_object = multi_block->GetBlock(0))
          {
            return vtkDataObjectToConduit::FillConduitNode(data_object, channel);
          }
        }
      }
    }
  }
  return true;
}

enum paraview_catalyst_error
{
  paraview_catalyst_error_invalid_node = 100,
  paraview_catalyst_error_results = 101,
};
#define pvcatalyst_err(name) static_cast<enum catalyst_error>(paraview_catalyst_error_##name)

//-----------------------------------------------------------------------------
enum catalyst_error catalyst_initialize_paraview(const conduit_node* params)
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
  vtkTypeUInt64 comm = static_cast<vtkTypeUInt64>(MPI_Comm_c2f(MPI_COMM_WORLD));
  if (cpp_params.has_path("catalyst/mpi_comm"))
  {
    comm = cpp_params["catalyst/mpi_comm"].to_int64();
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

        auto pipeline = vtkInSituInitializationHelper::AddPipeline(fname);

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
        vtkLog(ERROR, "Failed to load plugin xml " << proxyFilename.c_str());
      }
      else
      {
        vtkLog(INFO, "Catalyst plugin loaded: " << proxyFilename.c_str());
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
        vtkInSituInitializationHelper::AddPipeline(p);
      }
    }
  }

  if (!cpp_params.has_path("catalyst/scripts") && !cpp_params.has_path("catalyst/pipelines"))
  {
    // no catalyst initialization specified.
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "No Catalyst Python scripts or pre-compiled "
                                                "pipelines specified. No analysis pipelines will "
                                                "be executed.");
  }

  return catalyst_error_ok;
}

//-----------------------------------------------------------------------------
enum catalyst_error catalyst_execute_paraview(const conduit_node* params)
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
  const double time = root.has_path("state/time") ? root["state/time"].to_float64() : 0;

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
      if (type == "mesh" || type == "multimesh")
      {
        update_producer_mesh_blueprint(channel_name, &data_node, &fields, type == "multimesh");
      }
    }
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "No 'catalyst/channels' found. "
                                                "No meshes will be processed.");
  }

  // check for optional 'parameters'
  std::vector<std::string> parameters;
  if (root.has_path("state/parameters"))
  {
    const auto state_parameters = root["state/parameters"];
    const conduit_index_t nchildren = state_parameters.number_of_children();
    for (conduit_index_t i = 0; i < nchildren; ++i)
    {
      parameters.push_back(state_parameters.child(i).as_string());
    }
  }
  vtkInSituInitializationHelper::ExecutePipelines(timestep, time, parameters);

  return catalyst_error_ok;
}

//-----------------------------------------------------------------------------
enum catalyst_error catalyst_finalize_paraview(const conduit_node* params)
{
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());

  const conduit_cpp::Node cpp_params = conduit_cpp::cpp_node(const_cast<conduit_node*>(params));
  if (cpp_params.has_path("catalyst") &&
    !vtkCatalystBlueprint::Verify("finalize", cpp_params["catalyst"]))
  {
    vtkLogF(ERROR, "invalid 'catalyst' node passed to 'catalyst_finalize'. Finalization may fail.");
  }

  vtkInSituInitializationHelper::Finalize();

  return catalyst_error_ok;
}

//-----------------------------------------------------------------------------
enum catalyst_error catalyst_about_paraview(conduit_node* params)
{
  catalyst_stub_about(params);
  conduit_cpp::Node cpp_params = conduit_cpp::cpp_node(params);
  cpp_params["catalyst"]["capabilities"].append().set("paraview");
  if (vtkInSituInitializationHelper::IsPythonSupported())
  {
    cpp_params["catalyst"]["capabilities"].append().set("python");
  }
  cpp_params["catalyst"]["implementation"] = "paraview";

  return catalyst_error_ok;
}

//-----------------------------------------------------------------------------
enum catalyst_error catalyst_results_paraview(conduit_node* params)
{
  auto stub_error_status = catalyst_stub_results(params);

  if (stub_error_status != catalyst_error_ok)
  {
    return stub_error_status;
  }

  conduit_cpp::Node cpp_params = conduit_cpp::cpp_node(params);
  auto catalyst_node = cpp_params["catalyst"];

  bool is_success = true;
  std::vector<std::pair<std::string, vtkSMProxy*> > steerableProxies;
  vtkInSituInitializationHelper::GetSteerableProxies(steerableProxies);
  for (auto& proxy : steerableProxies)
  {
    is_success &= convert_to_blueprint_mesh(proxy.second, proxy.first, catalyst_node);
  }

  return is_success ? catalyst_error_ok : pvcatalyst_err(results);
}
