/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModulePythonHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProcessModulePythonHelper.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkPVProcessModulePythonHelper, "1.11");
vtkStandardNewMacro(vtkPVProcessModulePythonHelper);

//----------------------------------------------------------------------------
vtkPVProcessModulePythonHelper::vtkPVProcessModulePythonHelper()
{
  this->SMApplication = vtkSMApplication::New();
  this->ShowProgress = 0;
  this->Filter = 0;
  this->CurrentProgress = 0;
}

//----------------------------------------------------------------------------
vtkPVProcessModulePythonHelper::~vtkPVProcessModulePythonHelper()
{
  this->SMApplication->Finalize();
  this->SMApplication->Delete();
  this->SetFilter(0);
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVProcessModulePythonHelper::RunGUIStart(int argc, char **argv, int numServerProcs, int myId)
{
  (void)myId;
  (void)numServerProcs;

  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);

  vtkPVPythonOptions* boptions = vtkPVPythonOptions::SafeDownCast(this->ProcessModule->GetOptions());
  int res = 0; 


  vtkstd::vector<char*> vArg;
#define vtkPVStrDup(x) \
  strcpy(new char[ strlen(x) + 1], x)

  vArg.push_back(vtkPVStrDup(argv[0]));
  if ( boptions->GetPythonScriptName() )
    {
    vArg.push_back(vtkPVStrDup(boptions->GetPythonScriptName()));
    }
  else if (argc > 1)
    {
    vArg.push_back(vtkPVStrDup("-"));
    }
  for (int cc=1; cc < argc; cc++)
    {
    vArg.push_back(vtkPVStrDup(argv[cc]));
    }

  vtkPVPythonInterpretor* interpretor = vtkPVPythonInterpretor::New();
  res = interpretor->PyMain(vArg.size(), &*vArg.begin());
  interpretor->Delete();

  vtkstd::vector<char*>::iterator it;
  for ( it = vArg.begin(); it != vArg.end(); ++ it )
    {
    delete [] *it;
    }

  this->ProcessModule->Exit();

  // Exiting:  CLean up.
  return res;
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::ExitApplication()
{ 
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SendPrepareProgress()
{
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::CloseCurrentProgress()
{
  if ( this->ShowProgress )
    {
    while ( this->CurrentProgress <= 10 )
      {
      cout << ".";
      this->CurrentProgress ++;
      }
    cout << "]" << endl;
    }
  this->CurrentProgress = 0;
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SendCleanupPendingProgress()
{
  this->CloseCurrentProgress();
  this->ShowProgress = 0;
  this->SetFilter(0);
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SetLocalProgress(const char* filter, int val)
{
  val /= 10;
  int new_progress = 0;
  if ( !filter || !this->Filter || strcmp(filter, this->Filter) != 0 )
    {
    this->CloseCurrentProgress();
    this->SetFilter(filter);
    new_progress = 1;
    }
  if ( !this->ShowProgress )
    {
    new_progress = 1;
    this->ShowProgress = 1;
    }
  if ( new_progress )
    {
    if ( filter[0] == 'v' && filter[1] == 't' && filter[2] == 'k' )
      {
      filter += 3;
      }
    cout << "Process " << filter << " [";
    cout.flush();
    }
  while ( this->CurrentProgress <= val )
    {
    cout << ".";
    cout.flush();
    this->CurrentProgress ++;
    }
}

