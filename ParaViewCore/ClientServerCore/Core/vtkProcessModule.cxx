
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

#include "vtkAlgorithm.h"
#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDummyController.h"
#include "vtkDynamicLoader.h"
#include "vtkFloatingPointExceptions.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#include "vtkSessionIterator.h"
#include "vtkStdString.h"
#include "vtkTCPNetworkAccessManager.h"
#include "vtkPVConfig.h"

#include <vtksys/SystemTools.hxx>

#ifdef PARAVIEW_USE_MPI
# include "vtkMPIController.h"
# include "vtkMPI.h"
#endif

#ifdef PARAVIEW_ENABLE_PYTHON
# include "vtkProcessModuleInitializePython.h"
#endif

#ifndef _WIN32
#include <signal.h>
#endif

#include <assert.h>
#include <stdexcept> // for runtime_error

namespace
{
  // Returns true if the arguments has the specified boolean_arg.
  bool vtkFindArgument(const char* boolean_arg,
    int argc, char** &argv)
    {
    for (int cc=0; cc < argc; cc++)
      {
      if (argv[cc] != NULL && strcmp(argv[cc], boolean_arg) == 0)
        {
        return true;
        }
      }
    return false;
    }
}

//----------------------------------------------------------------------------
// * STATICS
vtkProcessModule::ProcessTypes
vtkProcessModule::ProcessType = vtkProcessModule::PROCESS_INVALID;

bool vtkProcessModule::FinalizeMPI = false;
bool vtkProcessModule::FinalizePython = false;

vtkSmartPointer<vtkProcessModule> vtkProcessModule::Singleton;
vtkSmartPointer<vtkMultiProcessController> vtkProcessModule::GlobalController;

//----------------------------------------------------------------------------
bool vtkProcessModule::Initialize(ProcessTypes type, int &argc, char** &argv)
{
  setlocale(LC_NUMERIC,"C");

  vtkProcessModule::ProcessType = type;

  vtkProcessModule::GlobalController = vtkSmartPointer<vtkDummyController>::New();

#ifdef PARAVIEW_USE_MPI
  bool use_mpi = (type != PROCESS_CLIENT);
  if (!use_mpi) // i.e. type == PROCESS_CLIENT.
    {
    // scan the arguments to determine if we need to initialize MPI on client.
    bool default_use_mpi = false;
#if defined(PARAVIEW_INITIALIZE_MPI_ON_CLIENT)
    default_use_mpi = true;
#endif

    // Refer to vtkPVOptions.cxx for details.
    if (vtkFindArgument("--mpi", argc, argv))
      {
      default_use_mpi = true;
      }
    else if (vtkFindArgument("--no-mpi", argc, argv))
      {
      default_use_mpi = false;
      }
    use_mpi = default_use_mpi;
    }

  // initialize MPI only on all processes if paraview is compiled w/MPI.
  int mpi_already_initialized = 0;
  MPI_Initialized(&mpi_already_initialized);
  if (mpi_already_initialized == 0 && use_mpi)
    {
    // MPICH changes the current working directory after MPI_Init. We fix that
    // by changing the CWD back to the original one after MPI_Init.
    std::string cwd = vtksys::SystemTools::GetCurrentWorkingDirectory(true);

    // This is here to avoid false leak messages from vtkDebugLeaks when
    // using mpich. It appears that the root process which spawns all the
    // main processes waits in MPI_Init() and calls exit() when
    // the others are done, causing apparent memory leaks for any objects
    // created before MPI_Init().
    MPI_Init(&argc, &argv);

    // restore CWD to what it was before the MPI intialization.
    vtksys::SystemTools::ChangeDirectory(cwd.c_str());

    vtkProcessModule::FinalizeMPI = true;
    } // END if MPI is already initialized

  if (use_mpi || mpi_already_initialized)
    {
    // Get number of ranks passed to mpiexec/mpirun etc.
    int numRanks = 0;
    MPI_Comm_size(MPI_COMM_WORLD,&numRanks);

    // Ensure that the user cannot run a client with more than one rank.
    if (type==PROCESS_CLIENT && numRanks > 1)
      {
      throw std::runtime_error("Client process should be run with one process!");
      }

    vtkProcessModule::GlobalController = vtkSmartPointer<vtkMPIController>::New();
    vtkProcessModule::GlobalController->Initialize(
      &argc, &argv, /*initializedExternally*/1);
    }
#else
  static_cast<void>(argc); // unused warning when MPI is off
  static_cast<void>(argv); // unused warning when MPI is off
#endif // PARAVIEW_USE_MPI
  vtkMultiProcessController::SetGlobalController(
    vtkProcessModule::GlobalController);

#ifdef PARAVIEW_USE_X
  // Hack to support -display parameter.  vtkPVOptions requires parameters to be
  // specified as -option=value, but it is generally expected that X window
  // programs allow you to set the display as -display host:port (i.e. without
  // the = between the option and value).  Unless someone wants to change
  // vtkPVOptions to work with or without the = (which may or may not be a good
  // idea), then this is the easiest way around the problem.
  for (int i = 1; i < argc-1; i++)
    {
    if (strcmp(argv[i], "-display") == 0)
      {
      char *displayenv = new char[strlen(argv[i+1]) + 10];
      sprintf(displayenv, "DISPLAY=%s", argv[i+1]);
      vtksys::SystemTools::PutEnv(displayenv);
      delete [] displayenv;
      // safe to delete since PutEnv keeps a copy of the string.
      argc -= 2;
      for (int j = i; j < argc; j++)
        {
        argv[j] = argv[j+2];
        }
      argv[argc] = NULL;
      break;
      }
    }
#endif // VTK_USE_X

#ifdef _WIN32
  // Avoid Ghost windows on windows XP
  typedef void (* VOID_FUN)();
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary("user32.dll");
  if(lib)
    {
    VOID_FUN func = (VOID_FUN)
      vtkDynamicLoader::GetSymbolAddress(lib, "DisableProcessWindowsGhosting");
    if(func)
      {
      (*func)();
      }
    }
#endif // _WIN32

#ifdef PARAVIEW_ENABLE_FPE
  vtkFloatingPointExceptions::Enable();
#endif //PARAVIEW_ENABLE_FPE

  // Don't prompt the user with startup errors on unix.
  // On windows, don't prompt when running on the dashboard.
#if defined(_WIN32) && !defined(__CYGWIN__)
  if (getenv("DASHBOARD_TEST_FROM_CTEST") || getenv("DART_TEST_FROM_DART"))
    {
    vtkOutputWindow::GetInstance()->PromptUserOff();
    }
  else
    {
    vtkOutputWindow::GetInstance()->PromptUserOn();
    }
#else
  vtkOutputWindow::GetInstance()->PromptUserOff();
#endif

#ifdef PARAVIEW_USE_MPI_SSEND
  vtkMPIController::SetUseSsendForRMI(1);
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
  vtkProcessModule::Singleton->InitializePythonEnvironment(argc, argv);
  return true;
}

//----------------------------------------------------------------------------
bool vtkProcessModule::Finalize()
{
#ifdef PARAVIEW_ENABLE_PYTHON
  // Finalize Python before anything else. This ensures that all proxy
  // references are removed before the process module disappears.
  if (vtkProcessModule::FinalizePython && vtkPythonInterpreter::IsInitialized())
    {
    vtkPythonInterpreter::Finalize();
    }
#endif

  if(vtkProcessModule::Singleton)
    {
    // Make sure no session are kept inside ProcessModule so SessionProxyManager
    // could cleanup their Proxies before the ProcessModule get deleted.
    vtkProcessModule::Singleton->Internals->Sessions.clear();

    vtkProcessModule::Singleton->InvokeEvent(vtkCommand::ExitEvent);
    }

  // destroy the process-module.
  vtkProcessModule::Singleton = NULL;

  // We don't really need to call SetGlobalController(NULL) since
  // it's really stored with a weak pointer.  We set it to null anyways
  // in case it gets changed later to reference counting the pointer
  vtkMultiProcessController::SetGlobalController(NULL);
  vtkProcessModule::GlobalController->Finalize(/*finalizedExternally*/1);
  vtkProcessModule::GlobalController = NULL;

#ifdef PARAVIEW_USE_MPI
  if (vtkProcessModule::FinalizeMPI)
    {
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
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
void vtkProcessModule::UpdateProcessType(ProcessTypes newType, bool dontKnowWhatImDoing/*=true*/)
{
  if(dontKnowWhatImDoing)
    {
    vtkWarningMacro("UpdateProcessType from "
                    << vtkProcessModule::ProcessType << " to " << newType);
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
vtkCxxSetObjectMacro(vtkProcessModule, NetworkAccessManager,
  vtkNetworkAccessManager);
//----------------------------------------------------------------------------
vtkProcessModule::vtkProcessModule()
{
  this->NetworkAccessManager = vtkTCPNetworkAccessManager::New();
  this->Options = 0;
  this->Internals = new vtkProcessModuleInternals();
  this->MaxSessionId = 0;
  this->ReportInterpreterErrors = true;
  this->SymmetricMPIMode = false;
  this->MultipleSessionsSupport = false; // Set MULTI-SERVER to false as DEFAULT
  this->EventCallDataSessionId = 0;

  vtkCompositeDataPipeline* cddp = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(cddp);
  cddp->Delete();
}

//----------------------------------------------------------------------------
vtkProcessModule::~vtkProcessModule()
{
  vtkAlgorithm::SetDefaultExecutivePrototype(NULL);

  this->SetNetworkAccessManager(NULL);
  this->SetOptions(NULL);

  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkProcessModule::RegisterSession(vtkSession* session)
{
  assert(session != NULL);
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
  for (iter = this->Internals->Sessions.begin();
    iter != this->Internals->Sessions.end(); ++iter)
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
  vtkErrorMacro("Session has not been registered. Cannot unregister : " <<
    session);
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

  return NULL;
}

//----------------------------------------------------------------------------
vtkIdType vtkProcessModule::GetSessionID(vtkSession* session)
{
  vtkProcessModuleInternals::MapOfSessions::iterator iter;
  for (iter = this->Internals->Sessions.begin();
    iter != this->Internals->Sessions.end(); ++iter)
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
  return this->GetGlobalController()?
    this->GetGlobalController()->GetNumberOfProcesses() : 1;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetPartitionId()
{
  return this->GetGlobalController()?
    this->GetGlobalController()->GetLocalProcessId() : 0;
}

//----------------------------------------------------------------------------
void vtkProcessModule::PushActiveSession(vtkSession* session)
{
  assert(session != NULL);

  this->Internals->ActiveSessionStack.push_back(session);
}

//----------------------------------------------------------------------------
void vtkProcessModule::PopActiveSession(vtkSession* session)
{
  assert(session != NULL);

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
    return NULL;
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
  return (iter != this->Internals->Sessions.end()?
    iter->second.GetPointer() : NULL);
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetOptions(vtkPVOptions* options)
{
  vtkSetObjectBodyMacro(Options, vtkPVOptions, options);
  if (options)
    {
    this->SetSymmetricMPIMode(
      options->GetSymmetricMPIMode() != 0);
    }
}

//----------------------------------------------------------------------------
bool vtkProcessModule::InitializePythonEnvironment(int argc, char** argv)
{
#ifdef PARAVIEW_ENABLE_PYTHON
  assert(argc >= 1);
  if (!vtkPythonInterpreter::IsInitialized())
    {
    // If someone already initialized Python before ProcessModule was started,
    // we don't finalize it when ProcessModule finalizes. This is for the cases
    // where ParaView modules are directly imported in python (not pvpython).
    vtkProcessModule::FinalizePython = true;
    }

  std::string self_dir, programname;

  if (argc > 0)
    {
    std::string errMsg;
    if (!vtksys::SystemTools::FindProgramPath(argv[0], programname, errMsg))
      {
      // if FindProgramPath fails. We really don't have much of an alternative
      // here. Python module importing is going to fail.
      programname = vtksys::SystemTools::CollapseFullPath(argv[0]);
      }
    self_dir = vtksys::SystemTools::GetFilenamePath(programname.c_str());
    }
  else
    {
    self_dir = vtksys::SystemTools::GetCurrentWorkingDirectory(/*collapse=*/true);
    programname = self_dir + "/unknown_exe";
    }

  vtkPythonInterpreter::SetProgramName(programname.c_str());
  vtkPythonAppInitPrependPath(self_dir.c_str());
#endif
  (void)argc;
  (void)argv;
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
    os << indent << "Options: " << "(null)" << endl;
    }
}
