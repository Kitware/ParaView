// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInSituInitializationHelper.h"

#include "vtkArrayDispatch.h"
#include "vtkCPCxxHelper.h"
#include "vtkCallbackCommand.h"
#if VTK_MODULE_ENABLE_VTK_IOCatalystConduit
#include "vtkConduitSource.h"
#include <catalyst_conduit.hpp>
#endif
#include "vtkDataArrayAccessor.h"
#include "vtkFieldData.h"
#include "vtkInSituPipelinePython.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPSystemTools.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointSet.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSteeringDataGenerator.h"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>

#if VTK_MODULE_ENABLE_ParaView_PythonInitializer
extern "C"
{
  void vtkPVInitializePythonModules();
}

#include "vtkPythonInterpreter.h"
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
    bool ResultsFailed;
  };

  vtkSmartPointer<vtkCPCxxHelper> CPCxxHelper;
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkSmartPointer<vtkMPIController> MPIController;
#endif
  std::map<std::string, vtkSmartPointer<vtkSMSourceProxy>> Producers;
  std::vector<PipelineInfo> Pipelines;
  std::map<vtkSMProxy*, std::string> SteerableProxies;
  std::map<vtkSMProxy*, std::string> SteerableExtracts;

  bool InExecutePipelines = false;
  bool InResultsPipelines = false;
  int TimeStep = 0;
  double Time = 0.0;
#if VTK_MODULE_ENABLE_VTK_IOCatalystConduit
  // pointer arguments of the current catalyst call
  conduit_node* catalyst_params = nullptr;
#endif
};

template <typename PropertyType>
struct PropertyCopier
{
  PropertyType* SMProperty = nullptr;

  template <typename ArrayType>
  void operator()(ArrayType* array)
  {
    const vtkIdType numTuples = array->GetNumberOfTuples();
    const int numComponents = array->GetNumberOfComponents();
    this->SMProperty->SetNumberOfElements(static_cast<unsigned int>(numTuples * numComponents));
    vtkDataArrayAccessor<ArrayType> accessor(array);
    for (vtkIdType cc = 0; cc < numTuples; ++cc)
    {
      for (int comp = 0; comp < numComponents; ++comp)
      {
        this->SMProperty->SetElement(cc * numComponents + comp, accessor.Get(cc, comp));
      }
    }
  }
};

namespace
{
template <typename T>
void SetPropertyValue(T* prop, vtkDataArray* array)
{
  PropertyCopier<T> copier;
  copier.SMProperty = prop;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
  if (!Dispatcher::Execute(array, copier))
  {
    copier(array);
  }
}
}

int vtkInSituInitializationHelper::WasInitializedOnce;
int vtkInSituInitializationHelper::WasFinalizedOnce;
vtkInSituInitializationHelper::vtkInternals* vtkInSituInitializationHelper::Internals;
//----------------------------------------------------------------------------
vtkInSituInitializationHelper::vtkInSituInitializationHelper() = default;

//----------------------------------------------------------------------------
vtkInSituInitializationHelper::~vtkInSituInitializationHelper() = default;

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::Initialize(
  vtkTypeUInt64 comm, const std::vector<std::string>& python_paths)
{
  vtkInSituInitializationHelper::Internals = new vtkInternals();
  auto& internals = (*vtkInSituInitializationHelper::Internals);

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  int isMPIInitialized = 0;
  if (MPI_Initialized(&isMPIInitialized) == MPI_SUCCESS && isMPIInitialized)
  {
    vtkVLogScopeF(
      PARAVIEW_LOG_CATALYST_VERBOSITY(), "Initializing MPI communicator using 'comm' (%llu)", comm);
    // convert comm to MPI handle.
    MPI_Comm mpicomm = MPI_Comm_f2c(comm);
    vtkMPICommunicatorOpaqueComm opaqueComm(&mpicomm);
    vtkNew<vtkMPICommunicator> mpiCommunicator;
    mpiCommunicator->InitializeExternal(&opaqueComm);
    internals.MPIController = vtkSmartPointer<vtkMPIController>::New();
    internals.MPIController->SetCommunicator(mpiCommunicator);
    vtkMultiProcessController::SetGlobalController(internals.MPIController);
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

  // for now, I am using vtkCPCxxHelper; that class should be removed when we
  // deprecate Legacy Catalyst API.
  internals.CPCxxHelper.TakeReference(vtkCPCxxHelper::New());

#if VTK_MODULE_ENABLE_ParaView_PythonInitializer
  // register static Python modules built, if any.
  vtkPVInitializePythonModules();

  // Since vtkPythonInterpreter offers onlu prepend and not append, we add the
  // paths in reverse order to keep the same order of precedence.
  for (auto iter = python_paths.crbegin(); iter != python_paths.crend(); iter++)
  {
    // remove any trailing slashes, not required but makes debuggin easier
    const std::string& path = *iter;
    std::string resolved_path =
      vtksys::SystemTools::ConvertToOutputPath(vtksys::SystemTools::CollapseFullPath(path));
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Adding PYTHONPATH %s", resolved_path.c_str());
    vtkPythonInterpreter::PrependPythonPath(resolved_path.c_str());
  }

#else
  (void)python_paths;
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
vtkInSituPipeline* vtkInSituInitializationHelper::AddPipeline(
  const std::string& name, const std::string& path)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(
      ERROR, "'vtkInSituInitializationHelper::AddPipeline' cannot be called before 'Initialize'.");
    return nullptr;
  }

  if (path.empty())
  {
    vtkLogF(WARNING, "Empty filename provided for script.");
    return nullptr;
  }
  std::string tmp = path;
  if (!vtkPSystemTools::FileExists(path.c_str()))
  {
    tmp.clear();
    if (vtksys::SystemTools::HasEnv("PYTHONPATH"))
    {
      std::string pythonPath;
      vtksys::SystemTools::GetEnv("PYTHONPATH", pythonPath);
      vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
        "Looking in PYTHONPATH '%s' for Catalyst script '%s'.", pythonPath.c_str(), path.c_str());
#if defined(_WIN32) && !defined(__MINGW32__)
      char splitChar = ';';
#else
      char splitChar = ':';
#endif
      std::string fileName = vtksys::SystemTools::GetFilenameName(path);
      std::vector<std::string> paths = vtksys::SystemTools::SplitString(pythonPath, splitChar);
      for (auto& p : paths)
      {
#if defined(_WIN32) && !defined(__MINGW32__)
        std::string testPath = p + "\\" + fileName;
#else
        std::string testPath = p + "/" + fileName;
#endif
        if (vtkPSystemTools::FileExists(testPath.c_str()))
        {
          tmp = testPath;
          break;
        }
      }
    }
    if (tmp.empty())
    {
      vtkLogF(
        WARNING, "File/path does not exist and not in PYTHONPATH: '%s'. Skipping.", path.c_str());
      return nullptr;
    }
  }

  vtkNew<vtkInSituPipelinePython> pipeline;
  pipeline->SetFileName(tmp.c_str());
  pipeline->SetName(name.c_str());
  vtkInSituInitializationHelper::AddPipeline(pipeline);
  return pipeline;
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::AddPipeline(vtkInSituPipeline* pipeline)
{
  if (pipeline)
  {
    auto& internals = (*vtkInSituInitializationHelper::Internals);
    internals.Pipelines.push_back(
      vtkInternals::PipelineInfo{ pipeline, false, false, false, false });
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
    vtkLogF(ERROR,
      "'vtkInSituInitializationHelper::MarkProducerModified' cannot be called before "
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
    vtkLogF(ERROR,
      "'vtkInSituInitializationHelper::MarkProducerModified' cannot be called before "
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
bool vtkInSituInitializationHelper::ExecutePipelines(const conduit_node* params)
{
#if VTK_MODULE_ENABLE_VTK_IOCatalystConduit

  auto& internals = (*vtkInSituInitializationHelper::Internals);
  // store a pointer to the catalyst_execute parameters. This will be accessible from python.
  internals.catalyst_params = const_cast<conduit_node*>(params);

  const conduit_cpp::Node& cpp_params = conduit_cpp::cpp_node(const_cast<conduit_node*>(params));
  const auto& root = cpp_params["catalyst"];

  const int timestep = root.has_path("state/timestep")
    ? root["state/timestep"].to_int64()
    : (root.has_path("state/cycle") ? root["state/cycle"].to_int64() : 0);
  const double time = root.has_path("state/time") ? root["state/time"].to_float64() : 0;

  std::vector<std::string> pipelines;
  if (root.has_path("state/pipelines"))
  {
    const auto state_pipelines = root["state/pipelines"];
    const conduit_index_t nchildren = state_pipelines.number_of_children();
    for (conduit_index_t i = 0; i < nchildren; ++i)
    {
      pipelines.push_back(state_pipelines.child(i).as_string());
    }
  }
  else
  {
    for (auto& item : internals.Pipelines)
    {
      if (item.Pipeline->GetName())
      {
        pipelines.push_back(item.Pipeline->GetName());
      }
    }
  }

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

  return vtkInSituInitializationHelper::ExecutePipelines(timestep, time, pipelines, parameters);
#else
  vtkLogF(ERROR, "ParaView is compiled without Conduit support");
  (void)(params);
  return false;
#endif
}

bool vtkInSituInitializationHelper::ExecutePipelines(int timestep, double time,
  const std::vector<std::string>& pipelines, const std::vector<std::string>& parameters)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR,
      "'vtkInSituInitializationHelper::MarkProducerModified' cannot be called before "
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

  UpdateSteerableProxies();

  std::set<std::string> toExecute;
  for (auto& p : pipelines)
  {
    toExecute.insert(p);
  }

  for (auto& item : internals.Pipelines)
  {
    if (item.Pipeline->GetName() && toExecute.find(item.Pipeline->GetName()) == toExecute.end())
    {
      continue;
    }

    if (!item.Initialized)
    {
      item.InitializationFailed = !item.Pipeline->Initialize();
      item.Initialized = true;
    }

    if (!item.InitializationFailed && !item.ExecuteFailed)
    {
      // set the execute parameters for this pipeline
      auto pipeline = vtkInSituPipelinePython::SafeDownCast(item.Pipeline);
      if (pipeline)
      {
        pipeline->SetParameters(parameters);
      }
      // If `Initialize` failed, don't call `Execute` on the Pipeline.
      // If Execute fails even once, we no longer call Execute on this pipeline
      // in subsequent calls to `ExecutePipelines`.
      item.ExecuteFailed = !item.Pipeline->Execute(timestep, time);
    }
  }
  internals.InExecutePipelines = false;

  for (auto& item : internals.Pipelines)
  {
    if (item.InitializationFailed)
    {
      vtkLogF(ERROR, "%s pipeline failed to initialize", item.Pipeline->GetName());
      return false;
    }
    if (item.ExecuteFailed)
    {
      vtkLogF(ERROR, "%s pipeline failed to execute", item.Pipeline->GetName());
      return false;
    }
  }
  return true;
}

bool vtkInSituInitializationHelper::GetResultsFromPipelines(conduit_node* catalyst_params)
{
#if VTK_MODULE_ENABLE_VTK_IOCatalystConduit
  auto& internals = (*vtkInSituInitializationHelper::Internals);
  if (internals.InExecutePipelines)
  {
    vtkLogF(ERROR, "Calling ExecutePipelines during GetResultsFromPipelines is not supported!");
    return false;
  }

  if (internals.InResultsPipelines)
  {
    vtkLogF(ERROR, "Recursive call to 'GetResultsFromPipelines' not supported!");
    return false;
  }

  internals.InResultsPipelines = true;

  // store a pointer to the catalyst_results parameters. This will be accessible from python.
  internals.catalyst_params = catalyst_params;

  // By default we use the time & timestep information from the previous catalyst_execute run.
  // However, a user may override it using the "catalyst/state/time" and "catalyst/state/timestep"
  // paths
  const conduit_cpp::Node& cpp_params = conduit_cpp::cpp_node(catalyst_params);
  if (cpp_params.has_path("catalyst"))
  {
    const auto& root = cpp_params["catalyst"];

    if (root.has_path("state/timestep"))
    {
      internals.TimeStep = root["state/timestep"].to_int64();
    }
    else if (root.has_path("state/cycle"))
    {
      internals.TimeStep = root["state/cycle"].to_int64();
    }

    if (root.has_path("state/time"))
    {
      internals.Time = root["state/time"].to_float64();
    }
  }

  for (auto& item : internals.Pipelines)
  {
    if (!item.InitializationFailed && !item.ExecuteFailed && !item.ResultsFailed)
    {
      item.ResultsFailed = !item.Pipeline->Results();
    }
  }

  internals.InResultsPipelines = false;

  return true;
#else
  vtkLogF(ERROR, "ParaView is compiled without Conduit support");
  (void)(catalyst_params);
  return false;
#endif
}

//----------------------------------------------------------------------------
int vtkInSituInitializationHelper::GetAttributeTypeFromString(const std::string& associationString)
{
  std::string lower_case_string = associationString;
  std::transform(associationString.begin(), associationString.end(), lower_case_string.begin(),
    [](char character) { return std::tolower(character); });

  if (lower_case_string == "point")
  {
    return vtkDataObject::POINT;
  }
  else if (lower_case_string == "cell")
  {
    return vtkDataObject::CELL;
  }
  else if (lower_case_string == "field")
  {
    return vtkDataObject::FIELD;
  }

  vtkLog(ERROR, "Invalid association \"" << associationString << "\"");
  return -1;
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::UpdateSteerableProxies()
{
#if VTK_MODULE_ENABLE_VTK_IOCatalystConduit
  auto& internals = (*vtkInSituInitializationHelper::Internals);

  for (auto& steerable_proxies : internals.SteerableProxies)
  {
    vtkPVXMLElement* hints = steerable_proxies.first->GetHints();
    if (!hints)
    {
      continue;
    }
    vtkPVXMLElement* mesh_hint =
      hints->FindNestedElementByName("CatalystInitializePropertiesWithMesh");
    if (!mesh_hint)
    {
      continue;
    }

    const char* mesh_name = mesh_hint->GetAttribute("mesh");
    if (!mesh_name)
    {
      vtkLog(ERROR, "`CatalystInitializePropertiesWithMesh` missing 'mesh' attribute.");
      continue;
    }

    auto source_iterator = internals.Producers.find(mesh_name);
    if (source_iterator == internals.Producers.end())
    {
      vtkLog(ERROR, << "No mesh named '" << mesh_name << "' present.");
      continue;
    }

    auto conduit_source =
      vtkConduitSource::SafeDownCast(source_iterator->second->GetClientSideObject());
    if (!conduit_source)
    {
      vtkLog(ERROR, << "No vtkConduitSource proxy!");
      continue;
    }

    conduit_source->Update();

    auto partitioned_data_set = vtkPartitionedDataSet::SafeDownCast(conduit_source->GetOutput());
    if (!partitioned_data_set)
    {
      vtkLog(ERROR, << "No vtkMultiBlockDataSet mesh.");
      continue;
    }

    if (partitioned_data_set->GetNumberOfPartitions() == 0)
    {
      vtkLog(ERROR, "Empty partitioned data set '" << mesh_name << "'.");
      continue;
    }

    auto grid = partitioned_data_set->GetPartition(0);
    if (!grid)
    {
      vtkLog(ERROR, "Empty grid received for mesh named '" << mesh_name << "'.");
      continue;
    }

    for (unsigned int cc = 0, max = mesh_hint->GetNumberOfNestedElements(); cc < max; ++cc)
    {
      auto child = mesh_hint->GetNestedElement(cc);
      if (child == nullptr || child->GetName() == nullptr ||
        strcmp(child->GetName(), "Property") != 0)
      {
        continue;
      }
      if (!child->GetAttribute("name") || !child->GetAttribute("array"))
      {
        vtkLog(ERROR, "Missing required attribute on `Property` element. Skipping.");
        continue;
      }

      auto property = steerable_proxies.first->GetProperty(child->GetAttribute("name"));
      if (!property)
      {
        vtkLog(ERROR,
          "No property named '" << child->GetAttribute("name") << "' present on proxy. Skipping.");
        continue;
      }

      int assoc = GetAttributeTypeFromString(child->GetAttributeOrDefault("association", "point"));
      if (assoc < 0)
      {
        vtkLog(ERROR, "Invalid 'association' specified. Skipping.");
        continue;
      }
      auto arrayname = child->GetAttribute("array");
      auto* fd = grid->GetAttributesAsFieldData(assoc);
      vtkDataArray* array = fd ? fd->GetArray(arrayname) : nullptr;
      if (strcmp(arrayname, "coords") == 0 && vtkPointSet::SafeDownCast(grid) &&
        assoc == vtkDataObject::POINT)
      {
        array = vtkPointSet::SafeDownCast(grid)->GetPoints()->GetData();
      }

      if (!array)
      {
        vtkLog(ERROR, "No array named '" << arrayname << "' present. Skipping.");
        continue;
      }
      else
      {
        if (auto dp = vtkSMDoubleVectorProperty::SafeDownCast(property))
        {
          ::SetPropertyValue<vtkSMDoubleVectorProperty>(dp, array);
        }
        else if (auto ip = vtkSMIntVectorProperty::SafeDownCast(property))
        {
          ::SetPropertyValue<vtkSMIntVectorProperty>(ip, array);
        }
        else if (auto idp = vtkSMIdTypeVectorProperty::SafeDownCast(property))
        {
          ::SetPropertyValue<vtkSMIdTypeVectorProperty>(idp, array);
        }
        else
        {
          vtkLog(ERROR,
            "Properties of type '" << property->GetClassName() << "' are not supported. Skipping.");
          continue;
        }
      }
    }
  }
#else
  vtkErrorWithObjectMacro(nullptr, << "module IOCatalystConduit is disabled, cannot use steering.");
#endif
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::UpdateAllProducers(double time)
{
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR,
      "'vtkInSituInitializationHelper::UpdateAllProducers' cannot be called before "
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
  if (!internals.InExecutePipelines && !internals.InResultsPipelines)
  {
    vtkLogF(ERROR,
      "'GetTimeStep' cannot be called outside 'ExecutePipelines' or 'GetResultsFromPipelines'.");
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
  if (!internals.InExecutePipelines && !internals.InResultsPipelines)
  {
    vtkLogF(
      ERROR, "'GetTime' cannot be called outside 'ExecutePipelines' or 'GetResultsFromPipelines'.");
    return 0;
  }

  return internals.Time;
}

//----------------------------------------------------------------------------
conduit_node* vtkInSituInitializationHelper::GetCatalystParameters()
{
#if VTK_MODULE_ENABLE_VTK_IOCatalystConduit
  if (vtkInSituInitializationHelper::Internals == nullptr)
  {
    vtkLogF(ERROR, "'GetCatalystParameters' cannot be called before 'Initialize'.");
    return nullptr;
  }

  auto& internals = (*vtkInSituInitializationHelper::Internals);
  if (!internals.InExecutePipelines && !internals.InResultsPipelines)
  {
    vtkLogF(ERROR,
      "'GetCatalystParameters' cannot be called outside 'ExecutePipelines' or "
      "'GetCatalystParameters'.");
    return nullptr;
  }

  return internals.catalyst_params;
#else
  vtkLogF(ERROR, "ParaView is compiled without Conduit support");
  return nullptr;
#endif
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

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::GetSteerableProxies(
  std::vector<std::pair<std::string, vtkSMProxy*>>& proxies)
{
  if (vtkInSituInitializationHelper::Internals != nullptr)
  {
    auto internals = vtkInSituInitializationHelper::Internals;
    proxies.reserve(
      proxies.size() + internals->SteerableProxies.size() + internals->SteerableExtracts.size());
    for (auto& proxy : internals->SteerableProxies)
    {
      proxies.emplace_back(proxy.second, proxy.first);
    }

    for (auto& proxy : internals->SteerableExtracts)
    {
      // The dataset to be serialized is the input of the extract
      vtkSMProxy* inputProxy =
        vtkSMProxyProperty::SafeDownCast(proxy.first->GetProperty("Producer"))->GetProxy(0);

      if (inputProxy)
      {
        proxies.emplace_back(proxy.second, inputProxy);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkInSituInitializationHelper::UpdateSteerableParameters(
  vtkSMProxy* steerableProxy, const char* steerableSourceName)
{
  if (steerableProxy == nullptr || steerableSourceName == nullptr)
  {
    return;
  }

  if (vtkInSituInitializationHelper::Internals != nullptr)
  {
    auto internals = vtkInSituInitializationHelper::Internals;

    if (strcmp(steerableProxy->GetXMLName(), "SteeringExtractor") == 0)
    {
      auto it = internals->SteerableExtracts.lower_bound(steerableProxy);
      if (it == internals->SteerableExtracts.end() || it->first != steerableProxy)
      {
        internals->SteerableExtracts.emplace_hint(it, steerableProxy, steerableSourceName);
      }
    }
    else
    {
      auto it = internals->SteerableProxies.lower_bound(steerableProxy);
      if (it == internals->SteerableProxies.end() || it->first != steerableProxy)
      {
        internals->SteerableProxies.emplace_hint(it, steerableProxy, steerableSourceName);
      }
    }

    UpdateSteerableProxies();
  }
}
