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
#include "vtkCPPythonScriptPipeline.h"

#include "vtkCPDataDescription.h"
#include "vtkDataObject.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVPythonOptions.h"
#include "vtkPythonInterpreter.h"
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

namespace
{
  //----------------------------------------------------------------------------
  void InitializePython()
    {
    static bool initialized = false;
    if (initialized) { return; }
    initialized = true;

    vtkPythonInterpreter::PrependPythonPath(PARAVIEW_INSTALL_DIR "/lib");
    vtkPythonInterpreter::PrependPythonPath(
      PARAVIEW_INSTALL_DIR "/lib/paraview-" PARAVIEW_VERSION);
    vtkPythonInterpreter::PrependPythonPath(
      PARAVIEW_INSTALL_DIR "/lib/paraview-" PARAVIEW_VERSION "/site-packages");

#if defined(_WIN32)
# if defined(CMAKE_INTDIR)
    std::string bin_dir = PARAVIEW_BINARY_DIR "/bin/" CMAKE_INTDIR;
# else
    std::string bin_dir = PARAVIEW_BINARY_DIR "/bin";
# endif
    // Also update PATH to include the bin_dir so when the DLLs for the Python
    // modules are searched, they can located.
    std::string cpathEnv = vtksys::SystemTools::GetEnv("PATH");
    cpathEnv = "PATH=" + bin_dir + ";" + cpathEnv;
    vtksys::SystemTools::PutEnv(cpathEnv.c_str());

    vtkPythonInterpreter::PrependPythonPath(bin_dir.c_str());
    vtkPythonInterpreter::PrependPythonPath(PARAVIEW_BINARY_DIR "/lib/site-packages");
    vtkPythonInterpreter::PrependPythonPath(PARAVIEW_BINARY_DIR "/lib");

#else // UNIX/OsX
    vtkPythonInterpreter::PrependPythonPath(PARAVIEW_BINARY_DIR "/lib");
    vtkPythonInterpreter::PrependPythonPath(PARAVIEW_BINARY_DIR "/lib/site-packages");
#endif

    // register callback to initialize modules statically. The callback is
    // empty when BUILD_SHARED_LIBS is ON.
    vtkPVInitializePythonModules();

    vtkPythonInterpreter::Initialize();

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
      << "paraview.print_error = f1\n"
      << "paraview.print_debug_info = f2\n"
      << "import vtkPVCatalystPython\n";
    vtkPythonInterpreter::RunSimpleString(loadPythonModules.str().c_str());
    }
}

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
  // only process 0 checks if the file exists and broadcasts that information
  // to the other processes
  int fileExists = 0;
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  if(controller->GetLocalProcessId()==0)
    {
    fileExists = vtksys::SystemTools::FileExists(fileName, true);
    }
  controller->Broadcast(&fileExists, 1, 0);
  if(fileExists == 0)
    {
    vtkErrorMacro("Could not find file " << fileName);
    return 0;
    }

  InitializePython();

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

  vtkPythonInterpreter::RunSimpleString(loadPythonModules.str().c_str());
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

  InitializePython();

  // check the script to see if it should be run...
  vtkStdString dataDescriptionString = this->GetPythonAddress(dataDescription);

  vtksys_ios::ostringstream pythonInput;
  pythonInput << "dataDescription = vtkPVCatalystPython.vtkCPDataDescription('"
              << dataDescriptionString << "')\n"
              << this->PythonScriptName << ".RequestDataDescription(dataDescription)\n";

  vtkPythonInterpreter::RunSimpleString(pythonInput.str().c_str());

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

  InitializePython();

  vtkStdString dataDescriptionString = this->GetPythonAddress(dataDescription);

  vtksys_ios::ostringstream pythonInput;
  pythonInput
    << "dataDescription = vtkPVCatalystPython.vtkCPDataDescription('"
    << dataDescriptionString << "')\n"
    << this->PythonScriptName << ".DoCoProcessing(dataDescription)\n";

  vtkPythonInterpreter::RunSimpleString(pythonInput.str().c_str());

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
void vtkCPPythonScriptPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PythonScriptName: " << this->PythonScriptName << "\n";
}
