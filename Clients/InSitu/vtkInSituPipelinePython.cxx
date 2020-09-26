/*=========================================================================

  Program:   ParaView
  Module:    vtkInSituPipelinePython.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "vtkPython.h" // must be first
#endif

#include "vtkCollection.h"
#include "vtkInSituInitializationHelper.h"
#include "vtkInSituPipelinePython.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPSystemTools.h"
#include "vtkPVLogger.h"
#include "vtkSMExtractTriggerProxy.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

#if VTK_MODULE_ENABLE_ParaView_RemotingLive
#include "vtkLiveInsituLink.h"
#endif

#include <vtksys/SystemTools.hxx>

class vtkInSituPipelinePython::vtkInternals
{
public:
  vtkSmartPointer<vtkSMExtractsController> ExtractsController;

  // Collection of extractor created in this pipeline.
  vtkNew<vtkCollection> Extractors;

#if VTK_MODULE_ENABLE_ParaView_RemotingLive
  vtkSmartPointer<vtkLiveInsituLink> LiveLink;
#endif

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  vtkSmartPyObject APIModule;
  vtkSmartPyObject Package;

  bool LoadAPIModule()
  {
    if (this->APIModule)
    {
      return true;
    }

    this->APIModule.TakeReference(PyImport_ImportModule("paraview.catalyst.insitu_pipeline"));
    if (!this->APIModule)
    {
      vtkLogF(ERROR, "Failed to import required Python module 'paraview.catalyst.insitu_pipeline'");
      vtkInternals::FlushErrors();
      return false;
    }
    return true;
  }

  static bool FlushErrors()
  {
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
      return false;
    }
    return true;
  }
#endif
};

namespace
{
// Used to keep track of the active pipeline.
class ScopedActivate
{
  static vtkInSituPipelinePython* ActivePipeline;

public:
  ScopedActivate(vtkInSituPipelinePython* self) { ActivePipeline = self; }
  ~ScopedActivate() { ActivePipeline = nullptr; }
  static vtkInSituPipelinePython* GetActivePipeline() { return ActivePipeline; }
};
vtkInSituPipelinePython* ScopedActivate::ActivePipeline = nullptr;
}

vtkStandardNewMacro(vtkInSituPipelinePython);
vtkCxxSetObjectMacro(vtkInSituPipelinePython, Options, vtkSMProxy);
//----------------------------------------------------------------------------
vtkInSituPipelinePython::vtkInSituPipelinePython()
  : Internals(new vtkInSituPipelinePython::vtkInternals())
  , PipelinePath(nullptr)
  , Options(nullptr)
{
}

//----------------------------------------------------------------------------
vtkInSituPipelinePython::~vtkInSituPipelinePython()
{
  this->SetPipelinePath(nullptr);
  this->SetOptions(nullptr);
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Initialize()
{
  ScopedActivate activate(this);
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  if (!this->PipelinePath)
  {
    vtkLogF(ERROR, "No pipeline path specified. Use 'SetPipelinePath'.");
    return false;
  }

  auto& internals = (*this->Internals);
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (!internals.LoadAPIModule())
  {
    return false;
  }

  vtkSmartPyObject method(PyString_FromString("load_module"));
  vtkSmartPyObject archive(PyString_FromString(this->PipelinePath));
  internals.Package.TakeReference(
    PyObject_CallMethodObjArgs(internals.APIModule, method, archive.GetPointer(), nullptr));
  if (!internals.Package)
  {
    vtkInternals::FlushErrors();
    return false;
  }

  vtkSmartPyObject method2(PyString_FromString("do_initialize"));
  vtkSmartPyObject self(vtkPythonUtil::GetObjectFromPointer(this));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method2, self.GetPointer(), internals.Package.GetPointer(), nullptr));
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
#else
  vtkLogF(WARNING, "Python support not enabled!");
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Execute(int timestep, double time)
{
  ScopedActivate activate(this);
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  auto& internals = (*this->Internals);
  if (!internals.Package || !internals.APIModule)
  {
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("do_execute"));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, internals.Package.GetPointer(), nullptr));
  if (!result)
  {
    vtkInternals::FlushErrors();
    return false;
  }

  // Generate extracts from extractor added by this pipeline.
  internals.ExtractsController->SetTimeStep(timestep);
  internals.ExtractsController->SetTime(time);
  internals.ExtractsController->Extract(internals.Extractors);

  // Handle Live.
  if (this->IsLiveActivated())
  {
    this->DoLive(timestep, time);
  }
  return true;
#else
  (void)timestep;
  (void)time;
  vtkLogF(WARNING, "Python support not enabled!");
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Finalize()
{
  ScopedActivate activate(this);
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  auto& internals = (*this->Internals);
  if (!internals.Package || !internals.APIModule)
  {
    return false;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("do_finalize"));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, internals.Package.GetPointer(), nullptr));
  if (!result)
  {
    vtkInternals::FlushErrors();
    internals.Package = nullptr;
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
  return true;
#else
  vtkLogF(WARNING, "Python support not enabled!");
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkInSituPipelinePython::RegisterExtractor(vtkSMProxy* extractor)
{
  auto pipeline = ScopedActivate::GetActivePipeline();
  if (!pipeline)
  {
    vtkLogF(ERROR, "No active pipeline. This is not expected and must point to some "
                   "internal logic error! Aborting for debugging purposes!");
    abort();
  }

  auto& internals = (*pipeline->Internals);
  vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Registering extractor (%s) for pipeline (%s)",
    vtkLogIdentifier(extractor), vtkLogIdentifier(pipeline));
  internals.Extractors->AddItem(extractor);
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::IsLiveActivated()
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
void vtkInSituPipelinePython::DoLive(int timestep, double time)
{
#if VTK_MODULE_ENABLE_ParaView_RemotingLive
  vtkVLogScopeFunction(PARAVIEW_LOG_CATALYST_VERBOSITY());
  auto& internals = (*this->Internals);
  if (!internals.LiveLink)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "live: creating vtkLiveInsituLink");
    internals.LiveLink.TakeReference(vtkLiveInsituLink::New());
    const std::string url = vtkSMPropertyHelper(this->Options, "CatalystLiveURL").GetAsString();
    auto split_idx = url.find(":");
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

    // update all producers since the in situ code does not update any
    // pipelines.
    vtkInSituInitializationHelper::UpdateAllProducers(time);

    internals.LiveLink->InsituPostProcess(time, timestep);
  } while (
    internals.LiveLink->GetSimulationPaused() && internals.LiveLink->WaitForLiveChange() == 0);
#endif
}

//----------------------------------------------------------------------------
void vtkInSituPipelinePython::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
