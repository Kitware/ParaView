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
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConnectDialog.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVWindow.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"

vtkCxxRevisionMacro(vtkPVProcessModuleGUIHelper, "1.5");
vtkStandardNewMacro(vtkPVProcessModuleGUIHelper);

vtkPVProcessModuleGUIHelper::vtkPVProcessModuleGUIHelper()
{
  this->PVApplication = 0;
  this->PVProcessModule = 0;
}

vtkPVProcessModuleGUIHelper::~vtkPVProcessModuleGUIHelper()
{
  this->SetPVApplication(0);
}

void vtkPVProcessModuleGUIHelper::SetPVApplication(vtkPVApplication* app)
{
  this->PVApplication = app;
}

void vtkPVProcessModuleGUIHelper::SetPVProcessModule(vtkPVProcessModule* pm)
{
  this->PVProcessModule = pm;
}

void vtkPVProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkPVProcessModuleGUIHelper::RunGUIStart(int argc, char **argv, int numServerProcs, int myId)
{
  (void)myId;
  // Start the application (UI). 
  // For SGI pipe option.
  this->PVApplication->SetNumberOfPipes(numServerProcs);
  
#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
  this->PVApplication->SetupTrapsForSignals(myId);   
#endif // PV_HAVE_TRAPS_FOR_SIGNALS
  
  if (this->PVApplication->GetStartGUI())
    {
    this->PVApplication->Script("wm withdraw .");
    this->PVApplication->Start(argc, argv);
    }
  else
    {
    this->PVApplication->Exit();
    }
  // Exiting:  CLean up.
  return this->PVApplication->GetExitStatus();
}


int vtkPVProcessModuleGUIHelper::OpenConnectionDialog(int* start)
{ 
  vtkPVApplication *pvApp = this->PVApplication;
  vtkPVClientServerModule* pm = 
    vtkPVClientServerModule::SafeDownCast(this->PVProcessModule);
  if(!pm)
    {
    vtkErrorMacro("Attempt to call OpenConnectionDialog without using a vtkPVClientServerModule");
    return 0;
    }
  
  
  char servers[1024];
  servers[0] = 0;
  pvApp->GetRegisteryValue(2, "RunTime", "Servers", servers);
  pvApp->Script("wm withdraw .");
  vtkPVConnectDialog* dialog = 
    vtkPVConnectDialog::New();
  dialog->SetHostname(pvApp->GetOptions()->GetHostName());
  dialog->SetSSHUser(pvApp->GetOptions()->GetUsername());
  dialog->SetPort(pvApp->GetOptions()->GetPort());
  dialog->SetNumberOfProcesses(pm->GetNumberOfProcesses());
  dialog->SetMultiProcessMode(pm->GetMultiProcessMode());
  dialog->Create(this->PVApplication, 0);
  dialog->SetListOfServers(servers);
  int res = dialog->Invoke();
  if ( res )
    {
    pm->GetOptions()->SetHostName(dialog->GetHostName());
    pm->GetOptions()->SetUsername(dialog->GetSSHUser());
    pm->GetOptions()->SetPort(dialog->GetPort());
    pm->SetNumberOfProcesses(dialog->GetNumberOfProcesses());
    pm->SetMultiProcessMode(dialog->GetMultiProcessMode());
    *start = 1;
    }
  pvApp->SetRegisteryValue(2, "RunTime", "Servers",
                           dialog->GetListOfServers());
  dialog->Delete();
  
  if ( !res )
    {
    return 0;
    }
  return 1;
}

  
void vtkPVProcessModuleGUIHelper::SendPrepareProgress()
{  
  if (!this->PVProcessModule->GetProgressRequests())
    {
    this->PVApplication->GetMainWindow()->StartProgress();
    }
  if ( this->PVProcessModule->GetProgressRequests() == 0 )
    {
    this->PVProcessModule->SetProgressEnabled(
      this->PVApplication->GetMainWindow()->GetEnabled());
    }
}

void vtkPVProcessModuleGUIHelper::SendCleanupPendingProgress()
{ 
  this->PVApplication->GetMainWindow()->EndProgress(this->PVProcessModule->GetProgressEnabled());
}


void vtkPVProcessModuleGUIHelper::SetLocalProgress(const char* filter, int progress)
{
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
  this->PVApplication->Exit();
}

