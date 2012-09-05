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

#include "CPSystemInformation.h"
#include "vtkInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkSMObject.h"
#include "vtkSMProperty.h"

#include "cppythonmodules.h"

#include <string>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkCPPythonHelper);

vtkCPPythonHelper* vtkCPPythonHelper::Instance = 0;

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
  vtkCPPythonHelper::Instance = NULL;
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
    CMakeLoadAllPythonModules();

    vtkCPPythonHelper::Instance->PythonInterpretor->InitializeSubInterpretor(argc, argv);

#ifdef PARAVIEW_INSTALL_DIR
    vtkCPPythonHelper::Instance->PythonInterpretor->AddPythonPath(PARAVIEW_INSTALL_DIR "/bin/" COPROCESSOR_BUILD_DIR);
    vtkCPPythonHelper::Instance->PythonInterpretor->AddPythonPath(PARAVIEW_INSTALL_DIR "/Utilities/VTKPythonWrapping");
#else
    vtkErrorMacro("ParaView install directory is undefined.");
    return 0;
#endif

    vtksys_ios::ostringstream loadPythonModules;
    loadPythonModules
      << "import sys\n"
      << "from paraview.simple import *\n"
      << "import vtkCoProcessorPython\n";

    vtkCPPythonHelper::Instance->PythonInterpretor->RunSimpleString(
        loadPythonModules.str().c_str());
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



