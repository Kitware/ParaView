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
#include <catalyst_stub.h>
#include <conduit.hpp>
#include <conduit_blueprint.hpp>
#include <conduit_cpp_to_c.hpp>

#include "vtkCatalystBlueprint.h"
#include "vtkConduitSource.h"
#include "vtkInSituInitializationHelper.h"
#include "vtkInSituPipelineIO.h"
#include "vtkPVLogger.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#endif

static bool update_producer_mesh_blueprint(
  const std::string& channel_name, const conduit::Node* node, const conduit::Node* global_fields)
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
  vtkInSituInitializationHelper::MarkProducerModified(channel_name);
  return true;
}

static vtkSmartPointer<vtkInSituPipeline> create_precompiled_pipeline(const conduit::Node& node)
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

//-----------------------------------------------------------------------------
void catalyst_initialize(const conduit_node* params)
{
  vtkLogger::Init();
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());

  const auto& cpp_params = (*conduit::cpp_node(params));
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
    return;
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
      auto iter = scripts.children();
      while (iter.has_next())
      {
        iter.next();
        const std::string script = iter.node().as_string();
        vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Analysis script: '%s'", script.c_str());
        vtkInSituInitializationHelper::AddPipeline(script);
      }
    }
    else
    {
      vtkLogF(WARNING, "Python support not enabled, 'catalyst/scripts' are ignored.");
    }
  }

  if (cpp_params.has_path("catalyst/pipelines"))
  {
    auto& pipelines = cpp_params["catalyst/pipelines"];
    auto iter = pipelines.children();
    while (iter.has_next())
    {
      iter.next();
      if (auto p = create_precompiled_pipeline(iter.node()))
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
}

//-----------------------------------------------------------------------------
void catalyst_execute(const conduit_node* params)
{
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());

  const auto& cpp_params = (*conduit::cpp_node(params));
  if (!cpp_params.has_path("catalyst"))
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Path 'catalyst' is not provided. Skipping.");
    return;
  }

  const auto& root = cpp_params["catalyst"];
  if (!vtkCatalystBlueprint::Verify("execute", root))
  {
    vtkLogF(ERROR, "invalid 'catalyst' node passed to 'catalyst_execute'. Execution failed.");
    return;
  }

  // catalyst/timestep or catalyst/cycle is used to indicate the timestep
  // catalyst/time is used to provide the time
  const int timestep = root.has_path("state/timestep")
    ? root["state/timestep"].to_int64()
    : (root.has_path("state/cycle") ? root["state/cycle"].to_int64() : 0);
  const double time = root.has_path("state/time") ? root["state/time"].to_float64() : 0;

  vtkVLogScopeF(
    PARAVIEW_LOG_CATALYST_VERBOSITY(), "co-processing for timestep=%d, time=%f", timestep, time);

  conduit::Node globalFields;

  // catalyst/channels are used to communicate meshes.
  if (root.has_child("channels"))
  {
    auto iter = root["channels"].children();
    while (iter.has_next())
    {
      iter.next();
      const std::string channel_name = iter.name();
      const auto& channel_node = iter.node();
      const std::string type = channel_node["type"].as_string();
      if (type == "mesh")
      {
        auto& mesh_node = channel_node["data"];
        conduit::Node info;
        if (conduit::blueprint::verify("mesh", mesh_node, info))
        {
          vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
            "Conduit Mesh blueprint validation succeeded for channel (%s)", channel_name.c_str());

          auto& fields = globalFields[channel_name];
          fields["time"].set(time);
          fields["timestep"].set(timestep);
          fields["cycle"].set(timestep);
          fields["channel"].set(channel_name);

          update_producer_mesh_blueprint(channel_name, &mesh_node, &fields);
        }
        else
        {
          vtkLogF(
            ERROR, "'data' on channel '%s' is not a valid 'mesh'. skipping.", channel_name.c_str());
        }
      }
      else
      {
        vtkLogF(ERROR, "channel (%s) has unsupported type (%s). skipping.", channel_name.c_str(),
          type.c_str());
      }
    }
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "No 'catalyst/channels' found. "
                                                "No meshes will be processed.");
  }

  vtkInSituInitializationHelper::ExecutePipelines(timestep, time);
}

//-----------------------------------------------------------------------------
void catalyst_finalize(const conduit_node* params)
{
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());

  const auto& cpp_params = (*conduit::cpp_node(params));
  if (cpp_params.has_path("catalyst") &&
    !vtkCatalystBlueprint::Verify("finalize", cpp_params["catalyst"]))
  {
    vtkLogF(ERROR, "invalid 'catalyst' node passed to 'catalyst_finalize'. Finalization mayfail.");
  }

  vtkInSituInitializationHelper::Finalize();
}

//-----------------------------------------------------------------------------
void catalyst_about(conduit_node* params)
{
  catalyst_stub_about(params);
  auto& cpp_params = (*conduit::cpp_node(params));
  cpp_params["catalyst"]["capabilities"].append().set("paraview");
  if (vtkInSituInitializationHelper::IsPythonSupported())
  {
    cpp_params["catalyst"]["capabilities"].append().set("python");
  }
  cpp_params["catalyst"]["implementation"] = "ParaView Catalyst";
}
