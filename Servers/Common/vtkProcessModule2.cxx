/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessModule2.h"
#include "vtkProcessModule2Internals.h"

#include "vtkDummyController.h"
#include "vtkDynamicLoader.h"
#include "vtkFloatingPointExceptions.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#include "vtkSessionIterator.h"
#include "vtkTCPNetworkAccessManager.h"
#include "vtkToolkits.h"

#include <vtksys/SystemTools.hxx>

#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#include <assert.h>

//----------------------------------------------------------------------------
// * STATICS
vtkProcessModule2::ProcessTypes
vtkProcessModule2::ProcessType = vtkProcessModule2::PROCESS_INVALID;

bool vtkProcessModule2::FinalizeMPI = false;

vtkSmartPointer<vtkProcessModule2> vtkProcessModule2::Singleton;
vtkSmartPointer<vtkMultiProcessController> vtkProcessModule2::GlobalController;

//----------------------------------------------------------------------------
bool vtkProcessModule2::Initialize(ProcessTypes type, int &argc, char** &argv)
{
  setlocale(LC_NUMERIC,"C");

  vtkProcessModule2::ProcessType = type;

  vtkProcessModule2::GlobalController = vtkSmartPointer<vtkDummyController>::New();
#ifdef VTK_USE_MPI
  bool use_mpi = (type != PROCESS_CLIENT);
  // initialize MPI only on non-client processes.
  if (use_mpi)
    {
    int mpi_already_initialized = 0;
    MPI_Initialized(&mpi_already_initialized);
    if (mpi_already_initialized == 0)
      {
      // MPICH changes the current working directory after MPI_Init. We fix that
      // by changing the CWD back to the original one after MPI_Init.
      vtkstd::string cwd = vtksys::SystemTools::GetCurrentWorkingDirectory(true);

      // This is here to avoid false leak messages from vtkDebugLeaks when
      // using mpich. It appears that the root process which spawns all the
      // main processes waits in MPI_Init() and calls exit() when
      // the others are done, causing apparent memory leaks for any objects
      // created before MPI_Init().
      MPI_Init(&argc, &argv);

      // restore CWD to what it was before the MPI intialization.
      vtksys::SystemTools::ChangeDirectory(cwd.c_str());

      vtkProcessModule2::FinalizeMPI = true;
      }

    vtkProcessModule2::GlobalController = vtkSmartPointer<vtkMPIController>::New();
    vtkProcessModule2::GlobalController->Initialize(
      &argc, &argv, /*initializedExternally*/1);
    }
#endif // VTK_USE_MPI
  vtkMultiProcessController::SetGlobalController(
    vtkProcessModule2::GlobalController);

#ifdef VTK_USE_X
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

  // Create the process module.
  vtkProcessModule2::Singleton = vtkSmartPointer<vtkProcessModule2>::New();
  return true;
}

//----------------------------------------------------------------------------
bool vtkProcessModule2::Finalize()
{
  // destroy the process-module.
  vtkProcessModule2::Singleton = NULL;

  // release the controller reference.
  vtkMultiProcessController::SetGlobalController(NULL);
  vtkProcessModule2::GlobalController->Finalize(/*finalizedExternally*/1);
  vtkProcessModule2::GlobalController = NULL;

#ifdef VTK_USE_MPI
  if (vtkProcessModule2::FinalizeMPI)
    {
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    }
#endif
  return true;
}

//----------------------------------------------------------------------------
// * vtkProcessModule2 non-static methods
vtkStandardNewMacro(vtkProcessModule2);
vtkCxxSetObjectMacro(vtkProcessModule2, NetworkAccessManager,
  vtkNetworkAccessManager);
vtkCxxSetObjectMacro(vtkProcessModule2, Options, vtkPVOptions);
//----------------------------------------------------------------------------
vtkProcessModule2::vtkProcessModule2()
{
  this->NetworkAccessManager = vtkTCPNetworkAccessManager::New();
  this->Options = 0;
  this->Internals = new vtkInternals();
  this->MaxSessionId = 0;
}

//----------------------------------------------------------------------------
vtkProcessModule2::~vtkProcessModule2()
{
  this->SetNetworkAccessManager(NULL);
  this->SetOptions(NULL);

  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkProcessModule2::RegisterSession(vtkSession* session)
{
  assert(session != NULL);
  this->MaxSessionId++;
  this->Internals->Sessions[this->MaxSessionId] = session;
  return this->MaxSessionId;
}

//----------------------------------------------------------------------------
bool vtkProcessModule2::UnRegisterSession(vtkIdType sessionID)
{
  vtkInternals::MapOfSessions::iterator iter =
    this->Internals->Sessions.find(sessionID);
  if (iter != this->Internals->Sessions.end())
    {
    this->Internals->Sessions.erase(iter);
    return true;
    }

  return false;
}


//----------------------------------------------------------------------------
bool vtkProcessModule2::UnRegisterSession(vtkSession* session)
{
 // not implemented yet.
  abort();
//  return this->UnRegisterSession(session->GetSessionId());
  return false;
}

//----------------------------------------------------------------------------
vtkSession* vtkProcessModule2::GetSession(vtkIdType sessionID)
{
  vtkInternals::MapOfSessions::iterator iter =
    this->Internals->Sessions.find(sessionID);
  if (iter != this->Internals->Sessions.end())
    {
    return iter->second.GetPointer();
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkSessionIterator* vtkProcessModule2::NewSessionIterator()
{
  vtkSessionIterator* iter = vtkSessionIterator::New();
  return iter;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkProcessModule2::GetGlobalController()
{
  return vtkProcessModule2::GlobalController;
}

//----------------------------------------------------------------------------
void vtkProcessModule2::PrintSelf(ostream& os, vtkIndent indent)
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
