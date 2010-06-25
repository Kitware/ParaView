/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPPythonHelper.h"

#include "CPSystemInformation.h"
#include "vtkCPProcessModulePythonHelper.h"
#include "vtkInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVMain.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkSMObject.h"
#include "vtkSMXMLParser.h"

#include <vtkstd/string>
using vtkstd::string;
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>
using vtksys_ios::ostringstream;

static void ParaViewInitializeInterpreter(vtkProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkInitializationHelper::InitializeInterpretor(pm);
}

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkCPPythonHelper);

vtkCPPythonHelper* vtkCPPythonHelper::Instance = 0;


//----------------------------------------------------------------------------
vtkCPPythonHelper::vtkCPPythonHelper()
{
  this->PVMain = 0;
  this->ProcessModuleHelper = 0;
  this->PythonOptions = 0;
}

//----------------------------------------------------------------------------
vtkCPPythonHelper::~vtkCPPythonHelper()
{
  if(this->ProcessModuleHelper)
    {
    this->ProcessModuleHelper->Delete();
    this->ProcessModuleHelper = 0;
    }
  if(this->PVMain)
    {
    this->PVMain->Delete();
    this->PVMain = 0;
    vtkPVMain::Finalize();
    }
  if(this->PythonOptions)
    {
    this->PythonOptions->Delete();
    this->PythonOptions = 0;
    }
}

//----------------------------------------------------------------------------
vtkCPPythonHelper* vtkCPPythonHelper::New()
{
  if(vtkCPPythonHelper::Instance == 0)
    {
    // Try the factory first
    vtkCPPythonHelper::Instance = (vtkCPPythonHelper*)
      vtkObjectFactory::CreateInstance("vtkCPPythonHelper");
    // if the factory did not provide one, then create it here
    if(!vtkCPPythonHelper::Instance)
      {
      vtkCPPythonHelper::Instance = new vtkCPPythonHelper;
      }

    vtkPVMain::SetUseMPI(1);
    int argc = 0;
    char** argv = new char*[1];
    argv[0] = new char[200];
    string CWD = vtksys::SystemTools::GetCurrentWorkingDirectory();
#ifdef COPROCESSOR_WIN32_BUILD
    strcpy_s(argv[0], strlen(CWD.c_str()), CWD.c_str());
#else
    strcpy(argv[0], CWD.c_str());
#endif
    vtkPVMain::Initialize(&argc, &argv);
    vtkCPPythonHelper::Instance->PVMain = vtkPVMain::New();
    vtkCPPythonHelper::Instance->PythonOptions = vtkPVPythonOptions::New();
    vtkCPPythonHelper::Instance->PythonOptions->SetProcessType(vtkPVOptions::PVBATCH);
    vtkCPPythonHelper::Instance->PythonOptions->SetSymmetricMPIMode(1);
    vtkCPPythonHelper::Instance->ProcessModuleHelper = vtkCPProcessModulePythonHelper::New();
    vtkCPPythonHelper::Instance->ProcessModuleHelper->SetDisableConsole(true);
    int ret = vtkCPPythonHelper::Instance->PVMain->Initialize(
      vtkCPPythonHelper::Instance->PythonOptions, 
      vtkCPPythonHelper::Instance->ProcessModuleHelper, 
      ParaViewInitializeInterpreter, argc, argv);
    delete []argv[0];
    delete []argv;
    if (ret)
      {
      vtkGenericWarningMacro("Problem with vtkPVMain::Initialize()");
      return 0;
      }
    // Tell process module that we support Multiple connections.
    // This must be set before starting the event loop.
    vtkProcessModule::GetProcessModule()->SupportMultipleConnectionsOff();
    ret = vtkCPPythonHelper::Instance->ProcessModuleHelper->Run(
      vtkCPPythonHelper::Instance->PythonOptions);

    ostringstream LoadPythonModules;
    LoadPythonModules
      << "import sys\n"
      << "from paraview.simple import *\n"
#ifndef COPROCESSOR_WIN32_BUILD
      // Not on Windows 
      << "import libvtkCoProcessorPython\n";
#else
      << "import vtkCoProcessorPython\n";
#endif

    vtkCPPythonHelper::Instance->ProcessModuleHelper->GetInterpretor()->
      RunSimpleString(LoadPythonModules.str().c_str());
    vtkCPPythonHelper::Instance->ProcessModuleHelper->GetInterpretor()->
      FlushMessages();
    }
  
  return vtkCPPythonHelper::Instance;
}

//----------------------------------------------------------------------------
vtkPVPythonInterpretor* vtkCPPythonHelper::GetPythonInterpretor()
{
  return this->ProcessModuleHelper->GetInterpretor();
}


//----------------------------------------------------------------------------
void vtkCPPythonHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}



