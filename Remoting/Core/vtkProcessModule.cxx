
/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessModule.h"
#include "vtkProcessModuleInternals.h"

#include "vtkCommand.h"
#include "vtkCompositeDataSet.h"
#include "vtkDummyController.h"
#include "vtkFloatingPointExceptions.h"
#include "vtkInformation.h"
#include "vtkMultiThreader.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPSystemTools.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#include "vtkPolyData.h"
#include "vtkSessionIterator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTCPNetworkAccessManager.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPIController.h"
#endif

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonInterpreter.h"
#endif

#ifdef _WIN32
#include "vtkDynamicLoader.h"
#else
#include <signal.h>
#endif

// this include is needed to ensure that vtkPVPluginLoader singleton doesn't get
// destroyed before the process module singleton is cleaned up.
#include "vtkPVPluginLoader.h"

#include <assert.h>
#include <clocale> // needed for setlocale()
#include <sstream>
#include <stdexcept> // for runtime_error

namespace
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
// Returns true if the arguments has the specified boolean_arg.
bool vtkFindArgument(const char* boolean_arg, int argc, char**& argv)
{
  for (int cc = 0; cc < argc; cc++)
  {
    if (argv[cc] != nullptr && strcmp(argv[cc], boolean_arg) == 0)
    {
      return true;
    }
  }
  return false;
}
#endif

// This is used to avoid creating vtkWin32OutputWindow on ParaView executables.
// vtkWin32OutputWindow is not a useful window for any of the ParaView commandline
// executables.
class vtkPVGenericOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkPVGenericOutputWindow, vtkOutputWindow);
  static vtkPVGenericOutputWindow* New();

private:
  vtkPVGenericOutputWindow() = default;
  ~vtkPVGenericOutputWindow() override = default;
};
vtkStandardNewMacro(vtkPVGenericOutputWindow);
}

//----------------------------------------------------------------------------
// * STATICS
vtkProcessModule::ProcessTypes vtkProcessModule::ProcessType = vtkProcessModule::PROCESS_INVALID;

bool vtkProcessModule::FinalizeMPI = false;
bool vtkProcessModule::FinalizePython = false;

vtkSmartPointer<vtkProcessModule> vtkProcessModule::Singleton;
vtkSmartPointer<vtkMultiProcessController> vtkProcessModule::GlobalController;

int vtkProcessModule::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines = 1;
int vtkProcessModule::DefaultMinimumGhostLevelsToRequestForStructuredPipelines = 0;

//----------------------------------------------------------------------------
bool vtkProcessModule::Initialize(ProcessTypes type, int& argc, char**& argv)
{
  setlocale(LC_NUMERIC, "C");

  vtkProcessModule::ProcessType = type;

  vtkProcessModule::GlobalController = vtkSmartPointer<vtkDummyController>::New();

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  // scan the arguments to determine if we need to initialize MPI on client.
  bool use_mpi;
  if (type == PROCESS_CLIENT)
  {
#if defined(PARAVIEW_INITIALIZE_MPI_ON_CLIENT)
    use_mpi = true;
#else
    use_mpi = false;
#endif
  }
  else
  {
    use_mpi = true;
  }

  // Refer to vtkPVOptions.cxx for details.
  if (vtkFindArgument("--mpi", argc, argv))
  {
    use_mpi = true;
  }
  else if (vtkFindArgument("--no-mpi", argc, argv))
  {
    use_mpi = false;
  }

  // initialize MPI only on all processes if paraview is compiled w/MPI.
  int mpi_already_initialized = 0;
  MPI_Initialized(&mpi_already_initialized);
  if (mpi_already_initialized == 0 && use_mpi)
  {
    // MPICH changes the current working directory after MPI_Init. We fix that
    // by changing the CWD back to the original one after MPI_Init.
    std::string cwd = vtksys::SystemTools::GetCurrentWorkingDirectory();

    // This is here to avoid false leak messages from vtkDebugLeaks when
    // using mpich. It appears that the root process which spawns all the
    // main processes waits in MPI_Init() and calls exit() when
    // the others are done, causing apparent memory leaks for any objects
    // created before MPI_Init().
    MPI_Init(&argc, &argv);

    // restore CWD to what it was before the MPI initialization.
    vtksys::SystemTools::ChangeDirectory(cwd.c_str());

    vtkProcessModule::FinalizeMPI = true;
  } // END if MPI is already initialized

  if (use_mpi || mpi_already_initialized)
  {
    if (vtkMPIController* controller =
          vtkMPIController::SafeDownCast(vtkMultiProcessController::GetGlobalController()))
    {
      vtkProcessModule::GlobalController = controller;
    }
    else
    {
      vtkProcessModule::GlobalController = vtkSmartPointer<vtkMPIController>::New();
      vtkProcessModule::GlobalController->Initialize(&argc, &argv, /*initializedExternally*/ 1);
    }
    // Get number of ranks in this process group
    int numRanks = vtkProcessModule::GlobalController->GetNumberOfProcesses();
    // Ensure that the user cannot run a client with more than one rank.
    if (type == PROCESS_CLIENT && numRanks > 1)
    {
      throw std::runtime_error("Client process should be run with one process!");
    }
  }
#else
  static_cast<void>(argc); // unused warning when MPI is off
  static_cast<void>(argv); // unused warning when MPI is off
#endif
  vtkProcessModule::GlobalController->BroadcastTriggerRMIOn();
  vtkMultiProcessController::SetGlobalController(vtkProcessModule::GlobalController);

  // Hack to support -display parameter.  vtkPVOptions requires parameters to be
  // specified as -option=value, but it is generally expected that X window
  // programs allow you to set the display as -display host:port (i.e. without
  // the = between the option and value).  Unless someone wants to change
  // vtkPVOptions to work with or without the = (which may or may not be a good
  // idea), then this is the easiest way around the problem.
  for (int i = 1; i < argc - 1; i++)
  {
    if (strcmp(argv[i], "-display") == 0)
    {
      size_t size = strlen(argv[i + 1]) + 10;
      char* displayenv = new char[size];
      snprintf(displayenv, size, "DISPLAY=%s", argv[i + 1]);
      vtksys::SystemTools::PutEnv(displayenv);
      delete[] displayenv;
      // safe to delete since PutEnv keeps a copy of the string.
      argc -= 2;
      for (int j = i; j < argc; j++)
      {
        argv[j] = argv[j + 2];
      }
      argv[argc] = nullptr;
      break;
    }
  }

#ifdef _WIN32
  // Avoid Ghost windows on windows XP
  typedef void (*VOID_FUN)();
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary("user32.dll");
  if (lib)
  {
    VOID_FUN func =
      (VOID_FUN)vtkDynamicLoader::GetSymbolAddress(lib, "DisableProcessWindowsGhosting");
    if (func)
    {
      (*func)();
    }
  }
#endif // _WIN32

#ifdef PARAVIEW_ENABLE_FPE
  vtkFloatingPointExceptions::Enable();
#endif

  if (vtkProcessModule::ProcessType != PROCESS_CLIENT)
  {
    // On non-client processes, we don't want VTK default output window esp. on
    // Windows since that pops up too many windows. Hence we replace it.
    vtkNew<vtkPVGenericOutputWindow> window;
    vtkOutputWindow::SetInstance(window.GetPointer());
  }

  // In general turn off error prompts. This is where the process waits for
  // user-input on any error/warning. In past, we turned on prompts on Windows.
  // However, for client processes, we capture the error messages in the UI and
  // for the server-processes, prompts can cause unnecessary interruptions hence
  // we turn off prompts all together.
  vtkOutputWindow::GetInstance()->PromptUserOff();

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  if (vtksys::SystemTools::GetEnv("PARAVIEW_USE_MPI_SSEND"))
  {
    vtkMPIController::SetUseSsendForRMI(1);
  }
#endif

  vtkMultiThreader::SetGlobalMaximumNumberOfThreads(1);

  // The running dashboard tests avoid showing the abort/retry popup dialog.
  vtksys::SystemTools::EnableMSVCDebugHook();

#ifndef _WIN32
  // When trying to send data to a socket/pipe and that socket/pipe is
  // closed/broken, the system will trigger a SIGPIPE signal which has by
  // default a quit/crash handler that won't give you any chance to handle the
  // error in any way.
  // Instead, we ignore the default handler to properly capture the error and
  // eventually provide some type of recovery.
  signal(SIGPIPE, SIG_IGN);
#endif

  // Create the process module.
  vtkProcessModule::Singleton = vtkSmartPointer<vtkProcessModule>::New();
  vtkProcessModule::Singleton->DetermineExecutablePath(argc, argv);
  vtkProcessModule::Singleton->InitializePythonEnvironment();
  return true;
}

//----------------------------------------------------------------------------
bool vtkProcessModule::Finalize()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  // Finalize Python before anything else. This ensures that all proxy
  // references are removed before the process module disappears.
  if (vtkProcessModule::FinalizePython && vtkPythonInterpreter::IsInitialized())
  {
    vtkPythonInterpreter::Finalize();
  }
#endif

  if (vtkProcessModule::Singleton)
  {
    // Make sure no session are kept inside ProcessModule so SessionProxyManager
    // could cleanup their Proxies before the ProcessModule get deleted.
    vtkProcessModule::Singleton->Internals->Sessions.clear();

    vtkProcessModule::Singleton->InvokeEvent(vtkCommand::ExitEvent);
  }

  // destroy the process-module.
  vtkProcessModule::Singleton = nullptr;

  // We don't really need to call SetGlobalController(nullptr) since
  // it's really stored with a weak pointer.  We set it to nullptr anyways
  // in case it gets changed later to reference counting the pointer
  vtkMultiProcessController::SetGlobalController(nullptr);
  vtkProcessModule::GlobalController->Finalize(/*finalizedExternally*/ 1);
  vtkProcessModule::GlobalController = nullptr;

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  if (vtkProcessModule::FinalizeMPI)
  {
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    // prevent caling MPI_Finalize() twice
    vtkProcessModule::FinalizeMPI = false;
  }
#endif

  return true;
}

//----------------------------------------------------------------------------
vtkProcessModule::ProcessTypes vtkProcessModule::GetProcessType()
{
  return vtkProcessModule::ProcessType;
}

//----------------------------------------------------------------------------
void vtkProcessModule::UpdateProcessType(ProcessTypes newType, bool dontKnowWhatImDoing /*=true*/)
{
  if (dontKnowWhatImDoing)
  {
    vtkWarningMacro(
      "UpdateProcessType from " << vtkProcessModule::ProcessType << " to " << newType);
  }
  vtkProcessModule::ProcessType = newType;
}

//----------------------------------------------------------------------------
vtkProcessModule* vtkProcessModule::GetProcessModule()
{
  return vtkProcessModule::Singleton.GetPointer();
}

//----------------------------------------------------------------------------
// * vtkProcessModule non-static methods
vtkStandardNewMacro(vtkProcessModule);
vtkCxxSetObjectMacro(vtkProcessModule, NetworkAccessManager, vtkNetworkAccessManager);
//----------------------------------------------------------------------------
vtkProcessModule::vtkProcessModule()
{
  this->NetworkAccessManager = vtkTCPNetworkAccessManager::New();
  this->Options = nullptr;
  this->Internals = new vtkProcessModuleInternals();
  this->MaxSessionId = 0;
  this->ReportInterpreterErrors = true;
  this->SymmetricMPIMode = false;
  this->MultipleSessionsSupport = false; // Set MULTI-SERVER to false as DEFAULT
  this->EventCallDataSessionId = 0;
}

//----------------------------------------------------------------------------
vtkProcessModule::~vtkProcessModule()
{
  this->SetNetworkAccessManager(nullptr);
  this->SetOptions(nullptr);

  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
vtkIdType vtkProcessModule::RegisterSession(vtkSession* session)
{
  assert(session != nullptr);
  this->MaxSessionId++;
  this->Internals->Sessions[this->MaxSessionId] = session;
  this->EventCallDataSessionId = this->MaxSessionId;
  this->InvokeEvent(vtkCommand::ConnectionCreatedEvent, &this->MaxSessionId);
  this->EventCallDataSessionId = 0;
  return this->MaxSessionId;
}

//----------------------------------------------------------------------------
bool vtkProcessModule::UnRegisterSession(vtkIdType sessionID)
{
  vtkProcessModuleInternals::MapOfSessions::iterator iter =
    this->Internals->Sessions.find(sessionID);
  if (iter != this->Internals->Sessions.end())
  {
    this->EventCallDataSessionId = sessionID;
    this->InvokeEvent(vtkCommand::ConnectionClosedEvent, &sessionID);
    this->EventCallDataSessionId = 0;
    this->Internals->Sessions.erase(iter);
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkProcessModule::UnRegisterSession(vtkSession* session)
{
  vtkProcessModuleInternals::MapOfSessions::iterator iter;
  for (iter = this->Internals->Sessions.begin(); iter != this->Internals->Sessions.end(); ++iter)
  {
    if (iter->second == session)
    {
      vtkIdType sessionID = iter->first;
      this->EventCallDataSessionId = sessionID;
      this->InvokeEvent(vtkCommand::ConnectionClosedEvent, &sessionID);
      this->EventCallDataSessionId = 0;
      this->Internals->Sessions.erase(iter);
      return true;
    }
  }
  vtkErrorMacro("Session has not been registered. Cannot unregister : " << session);
  return false;
}

//----------------------------------------------------------------------------
vtkSession* vtkProcessModule::GetSession(vtkIdType sessionID)
{
  vtkProcessModuleInternals::MapOfSessions::iterator iter =
    this->Internals->Sessions.find(sessionID);
  if (iter != this->Internals->Sessions.end())
  {
    return iter->second.GetPointer();
  }

  return nullptr;
}

//----------------------------------------------------------------------------
vtkIdType vtkProcessModule::GetSessionID(vtkSession* session)
{
  vtkProcessModuleInternals::MapOfSessions::iterator iter;
  for (iter = this->Internals->Sessions.begin(); iter != this->Internals->Sessions.end(); ++iter)
  {
    if (iter->second == session)
    {
      return iter->first;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkSessionIterator* vtkProcessModule::NewSessionIterator()
{
  vtkSessionIterator* iter = vtkSessionIterator::New();
  return iter;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkProcessModule::GetGlobalController()
{
  return vtkMultiProcessController::GetGlobalController();
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetNumberOfLocalPartitions()
{
  return this->GetGlobalController() ? this->GetGlobalController()->GetNumberOfProcesses() : 1;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetPartitionId()
{
  return this->GetGlobalController() ? this->GetGlobalController()->GetLocalProcessId() : 0;
}

//----------------------------------------------------------------------------
bool vtkProcessModule::IsMPIInitialized()
{
  return (this->GetGlobalController() && this->GetGlobalController()->IsA("vtkMPIController") != 0);
}

//----------------------------------------------------------------------------
void vtkProcessModule::PushActiveSession(vtkSession* session)
{
  assert(session != nullptr);

  this->Internals->ActiveSessionStack.push_back(session);
}

//----------------------------------------------------------------------------
void vtkProcessModule::PopActiveSession(vtkSession* session)
{
  assert(session != nullptr);

  if (this->Internals->ActiveSessionStack.back() != session)
  {
    vtkErrorMacro("Mismatch in active-session stack. Aborting for debugging.");
    abort();
  }
  this->Internals->ActiveSessionStack.pop_back();
}

//----------------------------------------------------------------------------
vtkSession* vtkProcessModule::GetActiveSession()
{
  if (this->Internals->ActiveSessionStack.size() == 0)
  {
    return nullptr;
  }
  return this->Internals->ActiveSessionStack.back();
}

//----------------------------------------------------------------------------
vtkSession* vtkProcessModule::GetSession()
{
  vtkSession* activeSession = this->GetActiveSession();
  if (activeSession)
  {
    return activeSession;
  }

  vtkProcessModuleInternals::MapOfSessions::iterator iter;
  iter = this->Internals->Sessions.begin();
  return (iter != this->Internals->Sessions.end() ? iter->second.GetPointer() : nullptr);
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetOptions(vtkPVOptions* options)
{
  vtkSetObjectBodyMacro(Options, vtkPVOptions, options);
  if (options)
  {
    this->SetSymmetricMPIMode(options->GetSymmetricMPIMode() != 0);
  }
}

//----------------------------------------------------------------------------
void vtkProcessModule::DetermineExecutablePath(int argc, char* argv[])
{
  assert(argc >= 1);

  if (argc > 0)
  {
    std::string errMsg;
    if (!vtkPSystemTools::FindProgramPath(argv[0], this->ProgramPath, errMsg))
    {
      // if FindProgramPath fails. We really don't have much of an alternative
      // here. Python module importing is going to fail.
      this->ProgramPath = vtkPSystemTools::CollapseFullPath(argv[0]);
    }
    this->SelfDir = vtksys::SystemTools::GetFilenamePath(this->ProgramPath);
  }
  else
  {
    this->SelfDir = vtkPSystemTools::GetCurrentWorkingDirectory(/*collapse=*/true);
    this->ProgramPath = this->SelfDir + "/unknown_exe";
  }
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetExecutablePath(const std::string& path)
{
  this->ProgramPath = vtkPSystemTools::CollapseFullPath(path);
  this->SelfDir = vtksys::SystemTools::GetFilenamePath(this->ProgramPath);
}

//----------------------------------------------------------------------------
bool vtkProcessModule::InitializePythonEnvironment()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  if (!vtkPythonInterpreter::IsInitialized())
  {
    // If someone already initialized Python before ProcessModule was started,
    // we don't finalize it when ProcessModule finalizes. This is for the cases
    // where ParaView modules are directly imported in python (not pvpython).
    vtkProcessModule::FinalizePython = true;
  }

// Help with finding `vtk` (and `paraview`) packages. This is generally needed
// only for static builds, but no harm in doing for shared build as well.
#if BUILD_SHARED_LIBS
  vtkPythonInterpreter::PrependPythonPath(this->GetSelfDir().c_str(), "vtkmodules/__init__.py");
#else
  // for static builds, we use zipped packages.

  // add path to _vtk.zip which is also the location for mpi4py modules, if any.
  vtkPythonInterpreter::PrependPythonPath(
    this->GetSelfDir().c_str(), "_vtk.zip", /*add_landmark=*/false);

  // add path for _vtk.zip; all vtk modules will be found here.
  vtkPythonInterpreter::PrependPythonPath(
    this->GetSelfDir().c_str(), "_vtk.zip", /*add_landmark=*/true);

  // add path for _paraview.zip, these are all the ParaView modules.
  vtkPythonInterpreter::PrependPythonPath(
    this->GetSelfDir().c_str(), "_paraview.zip", /*add_landmark=*/true);
#endif

#if defined(_WIN32)
  // ParaView executables generally link with all modules built except a few
  // such as the Catalyst libraries e.g. vtkPVCatalyst.dll. Now, when importing its
  // Python module `vtkPVCatalystPython.pyd` for example, it will need to load
  // vtkPVCatalyst.dll from its standard load paths which do not include the
  // executable path where the vtkPVCatalyst.dll actually exists. Hence we
  // manually extend the PATH to include it. This is not an issue on unixes
  // since rpath (or LD_LIBRARY_PATH set by shared forwarding executables) takes
  // care of it.
  std::ostringstream stream;
  stream << "PATH=" << this->SelfDir;
  if (const char* oldpath = vtksys::SystemTools::GetEnv("PATH"))
  {
    stream << ";" << oldpath;
  }
  vtksys::SystemTools::PutEnv(stream.str());
#endif // defined(_WIN32)

#endif // VTK_MODULE_ENABLE_VTK_PythonInterpreter
  return true;
}

//----------------------------------------------------------------------------
void vtkProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NetworkAccessManager: " << endl;
  this->NetworkAccessManager->PrintSelf(os, indent.GetNextIndent());

  if (this->Options)
  {
    os << indent << "Options: " << endl;
    this->Options->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Options: "
       << "(null)" << endl;
  }
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetDefaultMinimumGhostLevelsToRequestForStructuredPipelines(int val)
{
  vtkProcessModule::DefaultMinimumGhostLevelsToRequestForStructuredPipelines = val;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines()
{
  return vtkProcessModule::DefaultMinimumGhostLevelsToRequestForStructuredPipelines;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines(int val)
{
  vtkProcessModule::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines = val;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines()
{
  return vtkProcessModule::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetNumberOfGhostLevelsToRequest(vtkInformation* info)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller == nullptr || controller->GetNumberOfProcesses() <= 1)
  {
    return 0;
  }

  vtkDataSet* ds = vtkDataSet::GetData(info);
  if (ds || vtkCompositeDataSet::GetData(info) != nullptr)
  {
    // Check if this is structured-pipeline, this includes unstructured pipelines
    // downstream from structured source e.g. Wavelet - > Clip.
    // To do that, we use a trick. If WHOLE_EXTENT() key us present, it must have
    // started as a structured dataset.
    // The is one exception to this for ExplicitStructuredGrid data, their
    // behavior is both structured and unstructured but regarding ghost cells
    // they have the behavior of an unstructured dataset.
    const bool is_structured = (info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) != 0) &&
      (!ds || (ds && !ds->IsA("vtkExplicitStructuredGrid")));
    return is_structured
      ? vtkProcessModule::GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines()
      : vtkProcessModule::GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines();
  }
  else
  {
    // the pipeline is for a non-dataset/composite-dataset filter. don't bother
    // asking for ghost levels.
    return 0;
  }
}
