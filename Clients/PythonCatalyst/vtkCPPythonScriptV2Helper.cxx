/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonScriptV2Helper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be first

#include "vtkCPPythonScriptV2Helper.h"

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSMExtractTriggerProxy.h"
#include "vtkSMExtractsController.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkSmartPyObject.h"

#include <cassert>
#include <map>
#include <string>
#include <vector>

#if VTK_MODULE_ENABLE_ParaView_RemotingLive
#include "vtkLiveInsituLink.h"
#endif

namespace
{
template <typename T>
class vtkScopedSet
{
  T& Ref;
  T Value;

public:
  vtkScopedSet(T& ref, const T& val)
    : Ref(ref)
    , Value(ref)
  {
    this->Ref = val;
  }
  ~vtkScopedSet() { this->Ref = this->Value; }
};
}

class vtkCPPythonScriptV2Helper::vtkInternals
{
public:
  vtkSmartPointer<vtkSMExtractsController> ExtractsController;

  // Collection of extractor created in this pipeline.
  vtkNew<vtkCollection> Extractors;

  // Collection of views created in this pipeline.
  std::vector<vtkSmartPointer<vtkSMProxy> > Views;

  // Collection of trivial producers created for `vtkCPPythonScriptV2Pipeline`.
  std::map<std::string, vtkSmartPointer<vtkSMProxy> > TrivialProducers;

#if VTK_MODULE_ENABLE_ParaView_RemotingLive
  vtkSmartPointer<vtkLiveInsituLink> LiveLink;
#endif

  // flag that tells us the module has custom "execution" methods.
  bool HasCustomExecutionLogic = false;

  vtkSmartPyObject APIModule;
  vtkSmartPyObject PackageName;
  vtkSmartPyObject Package;

  /**
   * Prepares the user provided Python module/package i.e. makes it available
   * for importing on all ranks.
   *
   * On success, this->PackageName will be set to name that should be used to
   * import the provided package/module.
   */
  bool Prepare(const std::string& fname);

  /**
   * Imports the module.
   */
  bool Import();

  /**
   * Returns false is any error had occurred and was flushed, otherwise returns
   * true.
   */
  static bool FlushErrors();

private:
  /**
   * Loads the API module from `paraview` package. Returns false if failed.
   */
  bool LoadAPIModule();

  bool PackageImportFailed = false;
};

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::vtkInternals::LoadAPIModule()
{
  if (this->APIModule)
  {
    return true;
  }

  this->APIModule.TakeReference(PyImport_ImportModule("paraview.catalyst.v2_internals"));
  if (!this->APIModule)
  {
    vtkLogF(ERROR, "Failed to import required Python module 'paraview.catalyst.v2_internals'");
    vtkInternals::FlushErrors();
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::vtkInternals::FlushErrors()
{
  if (PyErr_Occurred())
  {
    PyErr_Print();
    PyErr_Clear();
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::vtkInternals::Prepare(const std::string& fname)
{
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (!this->LoadAPIModule())
  {
    return false;
  }

  vtkSmartPyObject method(PyString_FromString("register_module"));
  vtkSmartPyObject archive(PyString_FromString(fname.c_str()));
  this->PackageName.TakeReference(
    PyObject_CallMethodObjArgs(this->APIModule, method, archive.GetPointer(), nullptr));
  if (!this->PackageName)
  {
    vtkInternals::FlushErrors();
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::vtkInternals::Import()
{
  if (!this->PackageName)
  {
    // not "prepared".
    vtkLogF(ERROR, "Cannot determine package name. "
                   "Did you forget to call 'PrepareFromScript'?");
    return false;
  }

  if (this->Package)
  {
    // already imported.
    return true;
  }

  if (this->PackageImportFailed)
  {
    // avoid re-importing if it already failed.
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("import_and_validate"));
  this->Package.TakeReference(
    PyObject_CallMethodObjArgs(this->APIModule, method, this->PackageName.GetPointer(), nullptr));
  if (!this->Package)
  {
    vtkInternals::FlushErrors();
    this->PackageImportFailed = true;
    return false;
  }

  vtkSmartPyObject method2(PyString_FromString("has_customized_execution"));
  vtkSmartPyObject result(
    PyObject_CallMethodObjArgs(this->APIModule, method2, this->Package.GetPointer(), nullptr));
  if (!result || !PyBool_Check(result))
  {
    vtkInternals::FlushErrors();
    this->PackageImportFailed = true;
    return false;
  }

  this->HasCustomExecutionLogic = (result == Py_True);
  return true;
}

//----------------------------------------------------------------------------
vtkCPPythonScriptV2Helper* vtkCPPythonScriptV2Helper::ActiveInstance = nullptr;
//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkCPPythonScriptV2Helper);
vtkCxxSetObjectMacro(vtkCPPythonScriptV2Helper, Options, vtkSMProxy);
//----------------------------------------------------------------------------
vtkCPPythonScriptV2Helper::vtkCPPythonScriptV2Helper()
  : Internals(new vtkCPPythonScriptV2Helper::vtkInternals())
  , Options(nullptr)
  , Filename{}
  , DataDescription(nullptr)
{
}

//----------------------------------------------------------------------------
vtkCPPythonScriptV2Helper::~vtkCPPythonScriptV2Helper()
{
  this->SetOptions(nullptr);
  delete this->Internals;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::PrepareFromScript(const std::string& fname)
{
  vtkScopedSet<vtkCPPythonScriptV2Helper*> scoped(vtkCPPythonScriptV2Helper::ActiveInstance, this);

  auto& internals = (*this->Internals);
  if (internals.Prepare(fname))
  {
    this->Filename = fname;
    return true;
  }
  else
  {
    this->Filename = std::string();
    return false;
  }
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::IsImported() const
{
  auto& internals = (*this->Internals);
  return (internals.Package != nullptr);
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::Import()
{
  vtkScopedSet<vtkCPPythonScriptV2Helper*> scoped(vtkCPPythonScriptV2Helper::ActiveInstance, this);

  auto& internals = (*this->Internals);
  return internals.Import();
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::Import(vtkCPDataDescription* dataDesc)
{
  assert(dataDesc != nullptr);
  vtkScopedSet<vtkCPDataDescription*> scopedDD(this->DataDescription, dataDesc);
  return this->Import();
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::CatalystInitialize(vtkCPDataDescription* dataDesc)
{
  assert(dataDesc != nullptr);
  vtkScopedSet<vtkCPDataDescription*> scopedDD(this->DataDescription, dataDesc);
  return this->CatalystInitialize();
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::CatalystInitialize()
{
  vtkScopedSet<vtkCPPythonScriptV2Helper*> scoped(vtkCPPythonScriptV2Helper::ActiveInstance, this);

  auto& internals = (*this->Internals);
  if (!internals.Import())
  {
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("do_catalyst_initialize"));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, internals.Package.GetPointer(), nullptr));
  if (!result)
  {
    vtkInternals::FlushErrors();
    return false;
  }

  // Set up Extracts controller based on the Catalyst options.
  vtkLogIfF(WARNING, this->Options == nullptr,
    "Catalyst options proxy was not setup in Initialize call. Extracts may not be generated "
    "correctly.");
  internals.ExtractsController.TakeReference(vtkSMExtractsController::New());
  if (this->Options)
  {
    internals.ExtractsController->SetExtractsOutputDirectory(
      vtkSMPropertyHelper(this->Options, "ExtractsOutputDirectory").GetAsString());
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::CatalystFinalize()
{
  if (!this->IsImported())
  {
    return false;
  }

  vtkScopedSet<vtkCPPythonScriptV2Helper*> scoped(vtkCPPythonScriptV2Helper::ActiveInstance, this);

  auto& internals = (*this->Internals);

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("do_catalyst_finalize"));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, internals.Package.GetPointer(), nullptr));
  if (!result)
  {
    vtkInternals::FlushErrors();
    return false;
  }

  if (this->Options &&
    vtkSMPropertyHelper(this->Options, "GenerateCinemaSpecification").GetAsInt() == 1)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "saving Cinema specification");
    internals.ExtractsController->SaveSummaryTable(
      "data.csv", this->Options->GetSessionProxyManager());
  }
  internals.Package = nullptr;
  internals.ExtractsController = nullptr;
  return true;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::CatalystExecute(int timestep, double time)
{
  vtkScopedSet<vtkCPPythonScriptV2Helper*> scoped(vtkCPPythonScriptV2Helper::ActiveInstance, this);

  auto& internals = (*this->Internals);
  if (!internals.Import())
  {
    return false;
  }

  if (!this->IsActivated(timestep, time))
  {
    // skip calling RequestDataDescription.
    return true;
  }

  // Update ViewTime on each of the views.
  for (auto& view : internals.Views)
  {
    vtkSMPropertyHelper(view, "ViewTime").Set(time);
    view->UpdateVTKObjects();
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("do_catalyst_execute"));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, internals.Package.GetPointer(), nullptr));
  if (!result)
  {
    vtkInternals::FlushErrors();
    return false;
  }

  // Generate extracts from extractor added by this pipeline.
  internals.ExtractsController->SetTime(time);
  internals.ExtractsController->SetTimeStep(timestep);
  internals.ExtractsController->Extract(internals.Extractors);

  // Handle Live.
  if (this->IsLiveActivated())
  {
    this->DoLive(timestep, time);
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::CatalystExecute(vtkCPDataDescription* dataDesc)
{
  assert(dataDesc != nullptr);
  auto& internals = (*this->Internals);
  for (const auto& pair : internals.TrivialProducers)
  {
    const auto name = pair.first;
    const auto producer = pair.second;
    auto ipdesc = dataDesc->GetInputDescriptionByName(name.c_str());
    assert(ipdesc);

    auto tp = vtkPVTrivialProducer::SafeDownCast(producer->GetClientSideObject());
    tp->SetOutput(ipdesc->GetGrid(), dataDesc->GetTime());
    tp->SetWholeExtent(ipdesc->GetWholeExtent());

    producer->MarkModified(producer);
  }

  // We raise a warning if no TrivialProducers are know by this time since it
  // may indicate that the script does not depend on any simulation data at all
  // which it more often than not a bug.
  if (internals.TrivialProducers.size() == 0)
  {
    vtkLogF(WARNING, "script may not depend on simulation data; is that expected?");
  }

  vtkScopedSet<vtkCPDataDescription*> scopedDD(this->DataDescription, dataDesc);
  return this->CatalystExecute(dataDesc->GetTimeStep(), dataDesc->GetTime());
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::RequestDataDescription(vtkCPDataDescription* dataDesc)
{
  assert(dataDesc != nullptr);

  vtkScopedSet<vtkCPPythonScriptV2Helper*> scoped(vtkCPPythonScriptV2Helper::ActiveInstance, this);
  vtkScopedSet<vtkCPDataDescription*> scopedDD(this->DataDescription, dataDesc);

  auto& internals = (*this->Internals);
  if (!internals.Import())
  {
    return false;
  }

  if (!this->IsActivated(dataDesc->GetTimeStep(), dataDesc->GetTime()))
  {
    // skip calling RequestDataDescription.
    return true;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("do_request_data_description"));
  vtkSmartPyObject pyarg(vtkPythonUtil::GetObjectFromPointer(dataDesc));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, internals.Package.GetPointer(), pyarg.GetPointer(), nullptr));
  if (!result || !PyBool_Check(result))
  {
    vtkInternals::FlushErrors();
    return false;
  }

  if (result.GetPointer() == Py_True)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "'RequestDataDescription' provided in script; not enabling meshes/fields automatically.");
    // if RequestDataDescription was provided, then we defer to that custom
    // Python code to ensure fields/meshes we turned on as appropriate.
    return true;
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "'RequestDataDescription' was not provided in script; enabling all meshes/fields.");
    // RequestDataDescription and extractors really doesn't work well.
    // for now, just enable all fields and meshes. we'll need to see how we can
    // support that better in the future.
    for (unsigned int cc = 0, max = dataDesc->GetNumberOfInputDescriptions(); cc < max; ++cc)
    {
      auto ipdesc = dataDesc->GetInputDescription(cc);
      ipdesc->AllFieldsOn();
      ipdesc->GenerateMeshOn();
    }
  }

  return true;
}

//----------------------------------------------------------------------------
vtkCPPythonScriptV2Helper* vtkCPPythonScriptV2Helper::GetActiveInstance()
{
  return vtkCPPythonScriptV2Helper::ActiveInstance;
}

//----------------------------------------------------------------------------
void vtkCPPythonScriptV2Helper::RegisterExtractor(vtkSMProxy* extractor)
{
  auto& internals = (*this->Internals);
  vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Registering extractor (%s) for pipeline (%s)",
    vtkLogIdentifier(extractor), vtkLogIdentifier(this));
  internals.Extractors->AddItem(extractor);
}

//----------------------------------------------------------------------------
void vtkCPPythonScriptV2Helper::RegisterView(vtkSMProxy* view)
{
  auto& internals = (*this->Internals);
  vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Registering view (%s) for pipeline (%s)",
    vtkLogIdentifier(view), vtkLogIdentifier(this));
  internals.Views.push_back(view);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkCPPythonScriptV2Helper::GetTrivialProducer(const char* inputname)
{
  auto& internals = (*this->Internals);
  auto iter = internals.TrivialProducers.find(inputname);
  if (iter != internals.TrivialProducers.end())
  {
    return iter->second;
  }

  assert(this->DataDescription != nullptr);
  auto ipdesc = this->DataDescription->GetInputDescriptionByName(inputname);
  auto pxm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  auto producer = pxm->NewProxy("sources", "PVTrivialProducer2");

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(producer);
  if (auto tp = vtkPVTrivialProducer::SafeDownCast(producer->GetClientSideObject()))
  {
    tp->SetOutput(ipdesc->GetGrid(), this->DataDescription->GetTime());
    tp->SetWholeExtent(ipdesc->GetWholeExtent());
  }
  producer->UpdateVTKObjects();

  controller->RegisterPipelineProxy(producer, inputname);
  // add it to map.
  internals.TrivialProducers[inputname].TakeReference(producer);

  return producer;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::IsActivated(int timestep, double time)
{
  auto& internals = (*this->Internals);
  internals.ExtractsController->SetTime(time);
  internals.ExtractsController->SetTimeStep(timestep);

  if (auto globalTrigger = this->Options
      ? vtkSMExtractTriggerProxy::SafeDownCast(
          vtkSMPropertyHelper(this->Options, "GlobalTrigger").GetAsProxy())
      : nullptr)
  {
    if (!globalTrigger->IsActivated(internals.ExtractsController))
    {
      vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "global trigger not activated for ts=%d, time=%f",
        timestep, time);
      return false;
    }
  }

  if (internals.HasCustomExecutionLogic)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "treating as activated due to presence of custom callbacks");
    return true;
  }

  if (this->IsLiveActivated())
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "live-trigger activated.");
    return true;
  }

  if (internals.Extractors->GetNumberOfItems() == 0)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "module has no extractors and no custom execution callbacks; is that expected?");
  }

  if (internals.ExtractsController->IsAnyTriggerActivated(internals.Extractors))
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "some extractor activated.");
    return true;
  }

  vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "nothing activated (ts=%d, time=%f)", timestep, time);
  return false;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Helper::IsLiveActivated()
{
  if (!this->Options || vtkSMPropertyHelper(this->Options, "EnableCatalystLive").GetAsInt() == 0)
  {
    return false;
  }

  auto& internals = (*this->Internals);
  auto trigger = vtkSMExtractTriggerProxy::SafeDownCast(
    vtkSMPropertyHelper(this->Options, "CatalystLiveTrigger").GetAsProxy());
  if (trigger == nullptr || !trigger->IsActivated(internals.ExtractsController))
  {
    return false;
  }

#if VTK_MODULE_ENABLE_ParaView_RemotingLive
  return true;
#else
  static bool warn_once = true;
  if (warn_once)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(),
      "ParaView not built with 'RemotingLive' module enabled. Catalyst Live "
      "is not available.");
    warn_once = false;
  }
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkCPPythonScriptV2Helper::DoLive(int timestep, double time)
{
#if VTK_MODULE_ENABLE_ParaView_RemotingLive
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());
  auto& internals = (*this->Internals);

  if (this->DataDescription && internals.TrivialProducers.size() == 0)
  {
    vtkVLogF(
      PARAVIEW_LOG_CATALYST_VERBOSITY(), "live: create producers since none created in analysis");
    // this implies that we are in LegacyCatalystAdaptor mode and no trivial
    // producers were created for input descriptions. In that case, we create
    // trivial produces for all inputs
    for (unsigned int cc = 0, max = this->DataDescription->GetNumberOfInputDescriptions(); cc < max;
         ++cc)
    {
      this->GetTrivialProducer(this->DataDescription->GetInputDescriptionName(cc));
    }
  }

  if (!internals.LiveLink)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "live: creating vtkLiveInsituLink");
    internals.LiveLink.TakeReference(vtkLiveInsituLink::New());
    const std::string url = vtkSMPropertyHelper(this->Options, "CatalystLiveURL").GetAsString();
    auto split_idx = url.find(':');
    std::string hostname;
    int port = -1;
    if (split_idx == std::string::npos)
    {
      hostname = url;
    }
    else
    {
      hostname = url.substr(0, split_idx);
      port = std::atoi(url.substr(split_idx + 1).c_str());
    }
    internals.LiveLink->SetHostname(hostname.empty() ? "localhost" : hostname.c_str());
    internals.LiveLink->SetInsituPort(port <= 0 ? 22222 : port);
  }

  auto pxm = this->Options->GetSessionProxyManager();
  if (!internals.LiveLink->Initialize(pxm))
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "live: link init failed; skipping.");
    return;
  }

  do
  {
    internals.LiveLink->InsituUpdate(time, timestep);

    // FIXME:
    // update all producers since the in situ code does not update any
    // pipelines.
    // vtkInSituInitializationHelper::UpdateAllProducers(time);

    internals.LiveLink->InsituPostProcess(time, timestep);
  } while (
    internals.LiveLink->GetSimulationPaused() && internals.LiveLink->WaitForLiveChange() == 0);
#endif
}

//----------------------------------------------------------------------------
void vtkCPPythonScriptV2Helper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
