/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonAdaptorHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPythonAdaptorHelper.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVPythonAdapterHelper);

//----------------------------------------------------------------------------
vtkPVPythonAdapterHelper::vtkPVPythonAdapterHelper()
{
  this->SMApplication = vtkSMApplication::New();
  this->DisableConsole = false;
  this->Interpretor = 0;
}

//----------------------------------------------------------------------------
vtkPVPythonAdapterHelper::~vtkPVPythonAdapterHelper()
{
  this->SMApplication->Finalize();
  this->SMApplication->Delete();
  if (this->Interpretor)
    {
    this->Interpretor->Delete();
    this->Interpretor = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVPythonAdapterHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DisableConsole: " << this->DisableConsole << endl;
}

//----------------------------------------------------------------------------
int vtkPVPythonAdapterHelper::RunGUIStart(int argc, char **argv,
  int numServerProcs, int myId)
{
  vtkPVPythonOptions* boptions = vtkPVPythonOptions::SafeDownCast(
    this->ProcessModule->GetOptions());
  if (myId > 0 && !boptions->GetEnableSymmetricScripting())
    {
    return 0;
    }

  (void)numServerProcs;

  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);

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

  this->Interpretor = vtkPVPythonInterpretor::New();
  // Initializing the sub-interpreter because that is where RunSimpleString works.
  res = this->Interpretor->InitializeSubInterpretor(vArg.size(), &*vArg.begin());

  vtkstd::vector<char*>::iterator it;
  for ( it = vArg.begin(); it != vArg.end(); ++ it )
    {
    delete [] *it;
    }

  // Exiting:  CLean up.
  return res;
}

//----------------------------------------------------------------------------
void vtkPVPythonAdapterHelper::ExitApplication()
{ 
}

//----------------------------------------------------------------------------
void vtkPVPythonAdapterHelper::SendPrepareProgress()
{
}

//----------------------------------------------------------------------------
void vtkPVPythonAdapterHelper::SendCleanupPendingProgress()
{
}

//----------------------------------------------------------------------------
void vtkPVPythonAdapterHelper::SetLocalProgress(const char*, int)
{
}

