/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonScriptPipeline.cxx

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

#include "vtkCPPythonScriptPipeline.h"

#include "vtkCPDataDescription.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMObject.h"
#include "vtkSMProxyManager.h"

// for PARAVIEW_INSTALL_DIR and PARAVIEW_BINARY_DIR variables
#include "vtkCPPythonScriptPipelineConfig.h"

#include <string>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

extern "C" {
  void vtkPVInitializePythonModules();
}

// static member variable.
vtkSmartPointer<vtkPVPythonInterpretor> vtkCPPythonScriptPipeline::PythonInterpretor;

vtkStandardNewMacro(vtkCPPythonScriptPipeline);
//----------------------------------------------------------------------------
vtkCPPythonScriptPipeline::vtkCPPythonScriptPipeline()
{
  this->PythonScriptName = 0;
}

//----------------------------------------------------------------------------
vtkCPPythonScriptPipeline::~vtkCPPythonScriptPipeline()
{
  this->SetPythonScriptName(0);
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::Initialize(const char* fileName)
{
  if(vtksys::SystemTools::FileExists(fileName) == 0)
    {
    vtkErrorMacro("Could not find file " << fileName);
    return 0;
    }

  vtkPVPythonInterpretor* interp =
    vtkCPPythonScriptPipeline::GetPythonInterpretor();
  if (!interp)
    {
    vtkErrorMacro("Could not setup Python interpretor correctly.")
    return 0;
    }

  // for now do not check on filename extension:
  //vtksys::SystemTools::GetFilenameLastExtension(FileName) == ".py" == 0)

  std::string fileNamePath = vtksys::SystemTools::GetFilenamePath(fileName);
  std::string fileNameName = vtksys::SystemTools::GetFilenameWithoutExtension(
    vtksys::SystemTools::GetFilenameName(fileName));
  // need to save the script name as it is used as the name of the module
  this->SetPythonScriptName(fileNameName.c_str());

  vtksys_ios::ostringstream loadPythonModules;
  loadPythonModules
    << "sys.path.append('" << fileNamePath << "')\n"
    << "import " << fileNameName << "\n";

  interp->RunSimpleString(loadPythonModules.str().c_str());
  interp->FlushMessages();
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::RequestDataDescription(
  vtkCPDataDescription* dataDescription)
{
  if(!dataDescription)
    {
    vtkWarningMacro("dataDescription is NULL.");
    return 0;
    }

  vtkPVPythonInterpretor* interp =
    vtkCPPythonScriptPipeline::GetPythonInterpretor();
  if (!interp)
    {
    return 0;
    }

  // check the script to see if it should be run...
  vtkStdString dataDescriptionString = this->GetPythonAddress(dataDescription);

  vtksys_ios::ostringstream pythonInput;
  pythonInput << "dataDescription = vtkPVCatalystPython.vtkCPDataDescription('"
              << dataDescriptionString << "')\n"
              << this->PythonScriptName << ".RequestDataDescription(dataDescription)\n";

  interp->RunSimpleString(pythonInput.str().c_str());
  interp->FlushMessages();

  return dataDescription->GetIfAnyGridNecessary()? 1: 0;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::CoProcess(
  vtkCPDataDescription* dataDescription)
{
  if(!dataDescription)
    {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
    }

  vtkPVPythonInterpretor* interp =
    vtkCPPythonScriptPipeline::GetPythonInterpretor();
  if (!interp)
    {
    return 0;
    }

  vtkStdString dataDescriptionString = this->GetPythonAddress(dataDescription);

  vtksys_ios::ostringstream pythonInput;
  pythonInput
    << "dataDescription = vtkPVCatalystPython.vtkCPDataDescription('"
    << dataDescriptionString << "')\n"
    << this->PythonScriptName << ".DoCoProcessing(dataDescription)\n";

  interp->RunSimpleString(pythonInput.str().c_str());
  interp->FlushMessages();

  return 1;
}

//----------------------------------------------------------------------------
vtkStdString vtkCPPythonScriptPipeline::GetPythonAddress(void* pointer)
{
  char addressOfPointer[1024];
#ifdef COPROCESSOR_WIN32_BUILD
  sprintf_s(addressOfPointer, "%p", pointer);
#else
  sprintf(addressOfPointer, "%p", pointer);
#endif
  char *aplus = addressOfPointer;
  if ((addressOfPointer[0] == '0') && 
      ((addressOfPointer[1] == 'x') || addressOfPointer[1] == 'X'))
    {
    aplus += 2; //skip over "0x"
    }

  vtkStdString value = aplus;
  return value;
}

//----------------------------------------------------------------------------
vtkPVPythonInterpretor* vtkCPPythonScriptPipeline::GetPythonInterpretor()
{
  if (vtkCPPythonScriptPipeline::PythonInterpretor)
    {
    return vtkCPPythonScriptPipeline::PythonInterpretor;
    }

  // create and setup a new interpretor.
  vtkNew<vtkPVPythonInterpretor> interp;

  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();

  int argc = 1;
  char* argv[1];;
  std::string CWD = vtksys::SystemTools::GetCurrentWorkingDirectory();
  argv[0] = new char[CWD.size()+1];
#ifdef COPROCESSOR_WIN32_BUILD
  strcpy_s(argv[0], strlen(CWD.c_str()), CWD.c_str());
#else
  strcpy(argv[0], CWD.c_str());
#endif

  interp->InitializeSubInterpretor(argc, argv);
  interp->AddPythonPath(PARAVIEW_INSTALL_DIR "/lib");
  interp->AddPythonPath(
    PARAVIEW_INSTALL_DIR "/lib/paraview-" PARAVIEW_VERSION "/site-packages");
  interp->AddPythonPath(PARAVIEW_BINARY_DIR "/lib");
  interp->AddPythonPath(
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
  interp->RunSimpleString(loadPythonModules.str().c_str());
  interp->FlushMessages();
  delete []argv[0];

  vtkCPPythonScriptPipeline::PythonInterpretor = interp.GetPointer();
  return interp.GetPointer();
}

//----------------------------------------------------------------------------
void vtkCPPythonScriptPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PythonScriptName: " << this->PythonScriptName << "\n";
}
