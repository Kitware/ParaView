/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModuleGUIHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProcessModuleGUIHelper.h"
#include "vtkPVConfig.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVConnectDialog.h"
#include "vtkPVGUIClientOptions.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWindow.h"
#include "vtkWindows.h"

vtkCxxRevisionMacro(vtkPVProcessModuleGUIHelper, "1.18");
vtkStandardNewMacro(vtkPVProcessModuleGUIHelper);

vtkCxxSetObjectMacro(vtkPVProcessModuleGUIHelper, PVApplication, vtkPVApplication);

vtkPVProcessModuleGUIHelper::vtkPVProcessModuleGUIHelper()
{
  this->PVApplication = 0;
}

vtkPVProcessModuleGUIHelper::~vtkPVProcessModuleGUIHelper()
{
  this->SetPVApplication(0);
}

void vtkPVProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkPVProcessModuleGUIHelper::RunGUIStart(int argc, char **argv, int numServerProcs, int myId)
{
  (void)myId;
  // Initialize Tcl/Tk.
  Tcl_Interp *interp;
  vtkPVOptions* options = this->ProcessModule->GetOptions();

  int retVal = 0;
  int new_argc = 0;
  char** new_argv = 0;
  options->GetRemainingArguments(&new_argc, &new_argv);

  ostrstream err;
  interp = vtkPVApplication::InitializeTcl(new_argc, new_argv, &err);
  err << ends;
  if (!interp)
    {
#ifdef _WIN32
    ::MessageBox(0, err.str(), 
      "ParaView error: InitializeTcl failed", MB_ICONERROR|MB_OK);
#else
    cerr << "ParaView error: InitializeTcl failed" << endl 
      << err.str() << endl;
#endif
    err.rdbuf()->freeze(0);
#ifdef VTK_USE_MPI
    MPI_Finalize();
#endif
    return 1;
    }
  err.rdbuf()->freeze(0);

  // Create the application to parse the command line arguments.
  this->PVApplication = vtkPVApplication::New();
  this->PVApplication->SetOptions(vtkPVGUIClientOptions::SafeDownCast(options));

  if (myId == 0)
    {
    if (this->PVApplication->ParseCommandLineArguments(argc, argv))
      {
      retVal = 1;
      this->PVApplication->SetStartGUI(0);
      }
    // Get the application settings from the registry
    // It has to be called now, after ParseCommandLineArguments, which can 
    // change the registry level (also, it can not be called in the application
    // constructor or even the KWApplication constructor since we need the
    // application name to be set)

    this->PVApplication->RestoreApplicationSettingsFromRegistry();
    }

  this->PVApplication->SetProcessModule(
    vtkPVProcessModule::SafeDownCast(this->ProcessModule));
  // Start the application (UI). 
  // For SGI pipe option.
  this->PVApplication->SetNumberOfPipes(numServerProcs);

#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
  this->PVApplication->SetupTrapsForSignals(myId);   
#endif // PV_HAVE_TRAPS_FOR_SIGNALS

  int resStart = this->ActualRun(argc, argv);

  // Exiting:  CLean up.
  int resExit = this->PVApplication->GetExitStatus();
  this->PVApplication->Delete();
  this->PVApplication = 0;

  Tcl_DeleteInterp(interp);
  Tcl_Finalize();

  return resStart?resStart:(resExit?resExit:retVal);
}

int vtkPVProcessModuleGUIHelper::ActualRun(int argc, char **argv)
{
  if (this->PVApplication->GetStartGUI())
    {
    this->PVApplication->Start(argc, argv);
    }
  else
    {
    this->PVApplication->Exit();
    }
  return 0;
}

int vtkPVProcessModuleGUIHelper::OpenConnectionDialog(int* )
{ 
  // This should perhaps open a dialog and ask for where
  // the server is running, but for now just die
  vtkErrorMacro("Could not find server. Client must exit.");
  return 0;
}

  
void vtkPVProcessModuleGUIHelper::SendPrepareProgress()
{  
  if (! this->PVApplication || !this->PVApplication->GetMainWindow())
    {
    return;
    }
  if (!this->ProcessModule->GetProgressRequests())
    {
    this->PVApplication->GetMainWindow()->StartProgress();
    }
  if ( this->ProcessModule->GetProgressRequests() == 0 )
    {
    this->ProcessModule->SetProgressEnabled(
      this->PVApplication->GetMainWindow()->GetEnabled());
    }
}

void vtkPVProcessModuleGUIHelper::SendCleanupPendingProgress()
{ 
  if ( !this->PVApplication || !this->PVApplication->GetMainWindow())
    {
    return;
    }
  this->PVApplication->GetMainWindow()->EndProgress();
}


void vtkPVProcessModuleGUIHelper::SetLocalProgress(const char* filter, int progress)
{
  if ( !this->PVApplication || !this->PVApplication->GetMainWindow())
    {
    return;
    }
  if ( !filter )
    {
    vtkPVApplication::Abort();
    }
  if(!this->PVApplication->GetMainWindow())
    {
    return;
    }
  this->PVApplication->GetMainWindow()->SetProgress(filter, progress);
}

  
void vtkPVProcessModuleGUIHelper::ExitApplication()
{ 
  if ( !this->PVApplication )
    {
    return;
    }
  this->PVApplication->Exit();
}

