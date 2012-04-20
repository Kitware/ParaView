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
#include "vtkInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkPython.h"  // needed before including cppythonmodules.h
#include "vtkSMProxyManager.h"
#include "vtkSMObject.h"
#include "pvpython.h"
#include "vtkSMProperty.h"

#define EXCLUDE_LOAD_ALL_FUNCTION
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

    int argc = 1;
    char** argv = new char*[1];
    std::string CWD = vtksys::SystemTools::GetCurrentWorkingDirectory();
    argv[0] = new char[CWD.size()+1];
#ifdef COPROCESSOR_WIN32_BUILD
    strcpy_s(argv[0], strlen(CWD.c_str()), CWD.c_str());
#else
    strcpy(argv[0], CWD.c_str());
#endif

    vtkCPPythonHelper::Instance->PythonOptions = vtkPVPythonOptions::New();
    vtkCPPythonHelper::Instance->PythonOptions->SetSymmetricMPIMode(1);
    vtkCPPythonHelper::Instance->PythonOptions->SetProcessType(vtkProcessModule::PROCESS_BATCH);

    vtkInitializationHelper::Initialize(
        argc, argv, vtkProcessModule::PROCESS_BATCH,
        vtkCPPythonHelper::Instance->PythonOptions);


    // Do static initialization of python libraries
    cppythonmodules_h_LoadAllPythonModules();

    // Initialize the sub-interpreter because that is where RunSimpleString
    // works.
    vtkCPPythonHelper::Instance->PythonInterpretor = vtkPVPythonInterpretor::New();
    //int interpOk =
    vtkCPPythonHelper::Instance->PythonInterpretor->InitializeSubInterpretor(1, argv);

    delete []argv[0];
    delete []argv;

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
    }
  
  return vtkCPPythonHelper::Instance;
}

//----------------------------------------------------------------------------
vtkPVPythonInterpretor* vtkCPPythonHelper::GetPythonInterpretor()
{
  return this->PythonInterpretor;
}


//----------------------------------------------------------------------------
void vtkCPPythonHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}



