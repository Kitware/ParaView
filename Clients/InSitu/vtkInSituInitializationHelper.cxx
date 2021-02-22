/*=========================================================================

  Program:   ParaView
  Module:    vtkInSituInitializationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInSituInitializationHelper.h"

#include "vtkCPCxxHelper.h"
#include "vtkInSituPipelinePython.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPSystemTools.h"
#include "vtkPVLogger.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <map>
#include <string>

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
extern "C" {
void vtkPVInitializePythonModules();
}
#endif

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif

// #include "ParaView_paraview_plugins.h"

class vtkInSituInitializationHelper::vtkInternals
{
public:
  struct PipelineInfo
  {
    vtkSmartPointer<vtkInSituPipeline> Pipeline;
    bool Initialized;
    bool InitializationFailed;
    bool ExecuteFailed;
  };

  vtkSmartPointer<vtkCPCxxHelper> CPCxxHelper;
  std::map<std::string, vtkSmartPointer<vtkSMSourceProxy> > Producers;
  std::vector<PipelineInfo> Pipelines;

  bool InExecutePipelines = false;
  int TimeStep = 0;
  double Time = 0.0;
};

int vtkInSituInitializationHelper::WasInitializedOnce;
int vtkInSituInitializationHelper::WasFinalizedOnce;
vtkInSituInitializationHelper::vtkInternals* vtkInSituInitializationHelper::Internals;
//----------------------------------------------------------------------------
vtkInSituInitializationHelper::vtkInSituInitializationHelper() = default;

//----------------------------------------------------------------------------
vtkInSituInitializationHelper::~vtkInSituInitializationHelper() = default;

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::Initialize(vtkTypeUInt64 comm)
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  {
    vtkVLogScopeF(
      PARAVIEW_LOG_CATALYST_VERBOSITY(), "Initializing MPI communicator using 'comm' (%llu)", comm);
    // convert comm to MPI handle.
    MPI_Comm mpicomm = MPI_Comm_f2c(comm);
    vtkMPICommunicatorOpaqueComm opaqueComm(&mpicomm);
    vtkNew<vtkMPICommunicator> mpiCommunicator;
    mpiCommunicator->InitializeExternal(&opaqueComm);
    vtkNew<vtkMPIController> controller;
    controller->SetCommunicator(mpiCommunicator);
    vtkMultiProcessController::SetGlobalController(controller);
  }
#else
  vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
    "ParaView not built with MPI enabled. 'comm' (%llu) is ignored.", comm);
#endif

  if (auto controller = vtkMultiProcessController::GetGlobalController())
  {
    // For in situ, when running in distributed mode, ensure that we don't
    // generate any output on the stderr on satellite ranks. One can of course
    // generate log files, if needed for satellite, but it's best to disable
    // any terminal output for errors/warnings etc.
    if (controller->GetLocalProcessId() > 0)
    {
      vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);
    }
  }

  vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Initializing Catalyst");
  if (vtkInSituInitializationHelper::WasInitializedOnce)
  {
    vtkLogF(
      WARNING, "'vtkInSituInitializationHelper::Initialize' should not be called more than once.");
    return;
  }

  vtkInSituInitializationHelper::WasInitializedOnce = 1;

  vtkInSituInitializationHelper::Internals = new vtkInternals();
  auto& internals = (*vtkInSituInitializationHelper::Internals);
  // for now, I am using vtkCPCxxHelper; that class should be removed when we
  // deprecate Legacy Catalyst API.
  internals.CPCxxHelper.TakeReference(vtkCPCxxHelper::New());

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  // register static Python modules built, if any.
  vtkPVInitializePythonModules();
#endif

  // skipping for now
  // // register static plugins
  // ParaView_paraview_plugins_initialize();
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::Finalize()
{
  if (vtkInSituInitializationHelper::WasFinalizedOnce)
  {
    vtkLogF(
      WARNING, "'vtkInSituInitializationHelper::Finalize' should not be called more than once.");
    return;
  }

  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(
      ERROR, "'vtkInSituInitializationHelper::Finalize' cannot be called before 'Initialize'.");
    return;
  }

  // finalize pipelines.
  const auto& internals = (*vtkInSituInitializationHelper::Internals);
  for (auto& item : internals.Pipelines)
  {
    if (item.Initialized && !item.InitializationFailed)
    {
      item.Pipeline->Finalize();
    }
  }

  vtkInSituInitializationHelper::WasFinalizedOnce = 1;
  delete vtkInSituInitializationHelper::Internals;
  vtkInSituInitializationHelper::Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::AddPipeline(const std::string& path)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(
      ERROR, "'vtkInSituInitializationHelper::AddPipeline' cannot be called before 'Initialize'.");
    return;
  }

  if (path.empty())
  {
    vtkLogF(WARNING, "Empty path specified.");
    return;
  }

  if (!vtkPSystemTools::FileExists(path.c_str()))
  {
    vtkLogF(WARNING, "File/path does not exist: '%s'. Skipping", path.c_str());
    return;
  }

  vtkNew<vtkInSituPipelinePython> pipeline;
  pipeline->SetFileName(path.c_str());
  vtkInSituInitializationHelper::AddPipeline(pipeline);
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::AddPipeline(vtkInSituPipeline* pipeline)
{
  if (pipeline)
  {
    auto& internals = (*vtkInSituInitializationHelper::Internals);
    internals.Pipelines.push_back(vtkInternals::PipelineInfo{ pipeline, false, false, false });
  }
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::SetProducer(
  const std::string& channelName, vtkSMSourceProxy* producer)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(
      ERROR, "'vtkInSituInitializationHelper::SetProducer' cannot be called before 'Initialize'.");
    return;
  }

  if (channelName.empty() || producer == nullptr)
  {
    vtkLogF(ERROR, "Invalid arguments to 'SetProducer'");
    return;
  }

  auto& internals = (*vtkInSituInitializationHelper::Internals);
  if (internals.Producers.find(channelName) != internals.Producers.end())
  {
    vtkLogF(
      ERROR, "Producer for channel '%s' already exists. Cannot be replaced!", channelName.c_str());
    return;
  }

  vtkNew<vtkSMParaViewPipelineController> contoller;
  contoller->RegisterPipelineProxy(producer, channelName.c_str());
  internals.Producers[channelName] = producer;
}

//----------------------------------------------------------------------------
vtkSMSourceProxy* vtkInSituInitializationHelper::GetProducer(const std::string& channelName)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(
      ERROR, "'vtkInSituInitializationHelper::GetProducer' cannot be called before 'Initialize'.");
    return nullptr;
  }

  if (channelName.empty())
  {
    vtkLogF(ERROR, "Invalid arguments to 'GetProducer'");
    return nullptr;
  }

  auto& internals = (*vtkInSituInitializationHelper::Internals);
  auto iter = internals.Producers.find(channelName);
  return iter != internals.Producers.end() ? iter->second.GetPointer() : nullptr;
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::MarkProducerModified(const std::string& channelName)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR, "'vtkInSituInitializationHelper::MarkProducerModified' cannot be called before "
                   "'Initialize'.");
    return;
  }

  if (auto producer = vtkInSituInitializationHelper::GetProducer(channelName))
  {
    vtkInSituInitializationHelper::MarkProducerModified(producer);
  }
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::MarkProducerModified(vtkSMSourceProxy* producer)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR, "'vtkInSituInitializationHelper::MarkProducerModified' cannot be called before "
                   "'Initialize'.");
    return;
  }

  producer->UpdateVTKObjects();
  if (auto obj = vtkObject::SafeDownCast(producer->GetClientSideObject()))
  {
    obj->Modified();
  }
  producer->MarkModified(producer);
}

//----------------------------------------------------------------------------
bool vtkInSituInitializationHelper::ExecutePipelines(int timestep, double time)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR, "'vtkInSituInitializationHelper::MarkProducerModified' cannot be called before "
                   "'Initialize'.");
    return false;
  }

  auto& internals = (*vtkInSituInitializationHelper::Internals);
  if (internals.InExecutePipelines)
  {
    vtkLogF(ERROR, "Recursive call to 'ExecutePipelines' not supported!");
    return false;
  }

  internals.InExecutePipelines = true;
  internals.TimeStep = timestep;
  internals.Time = time;

  for (auto& item : internals.Pipelines)
  {
    if (!item.Initialized)
    {
      item.InitializationFailed = !item.Pipeline->Initialize();
      item.Initialized = true;
    }

    if (!item.InitializationFailed && !item.ExecuteFailed)
    {
      // If `Initialize` failed, don't call `Execute` on the Pipeline.
      // If Execute fails even once, we no longer call Execute on this pipeline
      // in subsequent calls to `ExecutePipelines`.
      item.ExecuteFailed = !item.Pipeline->Execute(timestep, time);
    }
  }

  internals.InExecutePipelines = false;
  return true;
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::UpdateAllProducers(double time)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR, "'vtkInSituInitializationHelper::UpdateAllProducers' cannot be called before "
                   "'Initialize'.");
    return;
  }

  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Updating all producer (time=%f)", time);

  auto& internals = (*vtkInSituInitializationHelper::Internals);
  for (const auto& pair : internals.Producers)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "updating producer '%s'", pair.first.c_str());
    pair.second->UpdatePipeline(time);
  }
}

//----------------------------------------------------------------------------
int vtkInSituInitializationHelper::GetTimeStep()
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR, "'GetTimeStep' cannot be called before 'Initialize'.");
    return 0;
  }

  auto& internals = (*vtkInSituInitializationHelper::Internals);
  if (!internals.InExecutePipelines)
  {
    vtkLogF(ERROR, "'GetTimeStep' cannot be called outside 'ExecutePipelines'.");
    return 0;
  }

  return internals.TimeStep;
}

//----------------------------------------------------------------------------
double vtkInSituInitializationHelper::GetTime()
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR, "'GetTime' cannot be called before 'Initialize'.");
    return 0;
  }

  auto& internals = (*vtkInSituInitializationHelper::Internals);
  if (!internals.InExecutePipelines)
  {
    vtkLogF(ERROR, "'GetTime' cannot be called outside 'ExecutePipelines'.");
    return 0;
  }

  return internals.Time;
}

//----------------------------------------------------------------------------
bool vtkInSituInitializationHelper::IsPythonSupported()
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  return true;
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
