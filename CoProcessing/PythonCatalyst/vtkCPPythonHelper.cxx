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
// vtkPython is at the very beginning since it defines stuff
// that will get defined elsewhere if it's not already defined
#include "vtkPython.h"

#include "vtkCPPythonHelper.h"

#include "vtkCPPythonHelperConfig.h"
#include "vtkInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMObject.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"

#include <string>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

extern "C" {
  void vtkPVInitializePythonModules();
}

extern const char* cp_helper_py;

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkCPPythonHelper);

vtkWeakPointer<vtkCPPythonHelper> vtkCPPythonHelper::Instance;

//----------------------------------------------------------------------------
vtkCPPythonHelper::vtkCPPythonHelper()
{
  this->PythonOptions = 0;
  this->PythonInterpretor = 0;
}

//----------------------------------------------------------------------------
vtkCPPythonHelper::~vtkCPPythonHelper()
{
  if(this->PythonOptions)
    {
    this->PythonOptions->Delete();
    this->PythonOptions = 0;
    }
  if(this->PythonInterpretor)
    {
    this->PythonInterpretor->Delete();
    this->PythonInterpretor = 0;
    }
  vtkInitializationHelper::Finalize();
}

const char* vtkCPPythonHelper::GetPythonHelperScript()
{
  return cp_helper_py;
}

//----------------------------------------------------------------------------
vtkCPPythonHelper* vtkCPPythonHelper::New()
{
  if (vtkCPPythonHelper::Instance.GetPointer() == 0)
    {
    // Try the factory first
    vtkCPPythonHelper* instance = (vtkCPPythonHelper*)
      vtkObjectFactory::CreateInstance("vtkCPPythonHelper");
    // if the factory did not provide one, then create it here
    if(!instance)
      {
      instance = new vtkCPPythonHelper;
      }
    vtkCPPythonHelper::Instance = instance;

    vtkCPPythonHelper::Instance->PythonOptions = vtkPVPythonOptions::New();
    vtkCPPythonHelper::Instance->PythonOptions->SetSymmetricMPIMode(1);
    vtkCPPythonHelper::Instance->PythonOptions->SetProcessType(
      vtkProcessModule::PROCESS_BATCH);

    int argc = 1;
    char* argv[1];;
    std::string CWD = vtksys::SystemTools::GetCurrentWorkingDirectory();
    argv[0] = new char[CWD.size()+1];
#ifdef COPROCESSOR_WIN32_BUILD
    strcpy_s(argv[0], strlen(CWD.c_str()), CWD.c_str());
#else
    strcpy(argv[0], CWD.c_str());
#endif
    vtkInitializationHelper::Initialize(
      argc, argv, vtkProcessModule::PROCESS_BATCH,
      vtkCPPythonHelper::Instance->PythonOptions);


    // Initialize the sub-interpreter because that is where RunSimpleString
    // works.
    vtkCPPythonHelper::Instance->PythonInterpretor = vtkPVPythonInterpretor::New();

    // register callback to initialize modules statically. The callback is
    // empty when BUILD_SHARED_LIBS is ON.
    vtkPVInitializePythonModules();

    vtkCPPythonHelper::Instance->PythonInterpretor->InitializeSubInterpretor(argc, argv);
    vtkCPPythonHelper::Instance->PythonInterpretor->AddPythonPath(
      PARAVIEW_INSTALL_DIR "/lib");
    vtkCPPythonHelper::Instance->PythonInterpretor->AddPythonPath(
      PARAVIEW_INSTALL_DIR "/lib/paraview-" PARAVIEW_VERSION "/site-packages");

    vtkCPPythonHelper::Instance->PythonInterpretor->AddPythonPath(
      PARAVIEW_BINARY_DIR "/lib");
    vtkCPPythonHelper::Instance->PythonInterpretor->AddPythonPath(
      PARAVIEW_BINARY_DIR "/lib/paraview-" PARAVIEW_VERSION "/site-packages");

    vtksys_ios::ostringstream loadPythonModules;
    loadPythonModules
      << "import sys\n"
      << "import paraview\n"
      << "f1 = paraview.print_error\n"
      << "f2 = paraview.print_debug_info\n"
      << "def print_dummy(text):\n"
      << "  pass\n"
      << "paraview.print_error = print_dummy\n"
      << "paraview.print_debug_info = print_dummy\n"
      << "from paraview.simple import *\n"
      << "paraview.print_error = f1\n"
      << "paraview.print_debug_info = f2\n"
      << "import vtkPVCatalystPython\n";
    vtkCPPythonHelper::Instance->PythonInterpretor->RunSimpleString(
        loadPythonModules.str().c_str());
    vtkCPPythonHelper::Instance->PythonInterpretor->RunSimpleString(
      vtkCPPythonHelper::GetPythonHelperScript());
    vtkCPPythonHelper::Instance->PythonInterpretor->FlushMessages();
    delete []argv[0];
    }
  else
    {
    vtkCPPythonHelper::Instance->Register(NULL);
    }

  return vtkCPPythonHelper::Instance;
}

//----------------------------------------------------------------------------
vtkPVPythonInterpretor* vtkCPPythonHelper::GetPythonInterpretor()
{
  return Instance->PythonInterpretor;
}

//----------------------------------------------------------------------------
void vtkCPPythonHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
