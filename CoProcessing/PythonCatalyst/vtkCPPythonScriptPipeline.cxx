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
#include "vtkPVConfig.h"
#include "vtkPVPythonOptions.h"
#include "vtkProcessModule.h"
#include "vtkPythonInterpreter.h"
#include "vtkSMObject.h"
#include "vtkSMProxyManager.h"

#include <sstream>
#include <string>
#include <vtksys/SystemTools.hxx>

extern "C" {
void vtkPVInitializePythonModules();
}

namespace
{
//----------------------------------------------------------------------------
void InitializePython()
{
  static bool initialized = false;
  if (initialized)
  {
    return;
  }
  initialized = true;

  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();

  vtkPythonInterpreter::Initialize();

  std::ostringstream loadPythonModules;
  loadPythonModules << "import sys\n"
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

//----------------------------------------------------------------------------
// for things like programmable filters that have a '\n' in their strings,
// we need to fix them to have \\n so that everything works smoothly
void fixEOL(std::string& str)
{
  const std::string from = "\\n";
  const std::string to = "\\\\n";
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  return;
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
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller->GetLocalProcessId() == 0)
  {
    fileExists = vtksys::SystemTools::FileExists(fileName, true);
  }
  controller->Broadcast(&fileExists, 1, 0);
  if (fileExists == 0)
  {
    vtkErrorMacro("Could not find file " << fileName);
    return 0;
  }

  InitializePython();

  // for now do not check on filename extension:
  // vtksys::SystemTools::GetFilenameLastExtension(FileName) == ".py" == 0)

  std::string fileNamePath = vtksys::SystemTools::GetFilenamePath(fileName);
  std::string fileNameName = vtksys::SystemTools::GetFilenameWithoutExtension(
    vtksys::SystemTools::GetFilenameName(fileName));
  // need to save the script name as it is used as the name of the module
  this->SetPythonScriptName(fileNameName.c_str());

  // only process 0 reads the actual script and then broadcasts it out
  char* scriptText = NULL;
  // we need to add the script path to PYTHONPATH
  char* scriptPath = NULL;

  int rank = controller->GetLocalProcessId();
  int scriptSizes[2] = { 0, 0 };
  if (rank == 0)
  {
    std::string line;
    std::ifstream myfile(fileName);
    std::string desiredString;
    if (myfile.is_open())
    {
      while (getline(myfile, line))
      {
        fixEOL(line);
        desiredString.append(line).append("\n");
      }
      myfile.close();
    }

    if (fileNamePath.empty())
    {
      fileNamePath = ".";
    }
    scriptSizes[0] = static_cast<int>(fileNamePath.size() + 1);
    scriptPath = new char[scriptSizes[0]];
    memcpy(scriptPath, fileNamePath.c_str(), sizeof(char) * scriptSizes[0]);

    scriptSizes[1] = static_cast<int>(desiredString.size() + 1);
    scriptText = new char[scriptSizes[1]];
    memcpy(scriptText, desiredString.c_str(), sizeof(char) * scriptSizes[1]);
  }

  controller->Broadcast(scriptSizes, 2, 0);

  if (rank != 0)
  {
    scriptPath = new char[scriptSizes[0]];
    scriptText = new char[scriptSizes[1]];
  }

  controller->Broadcast(scriptPath, scriptSizes[0], 0);
  controller->Broadcast(scriptText, scriptSizes[1], 0);

  vtkPythonInterpreter::PrependPythonPath(scriptPath);

  // The code below creates a module from the scriptText string.
  // This requires the manual creation of a module object like this:
  //
  // import types
  // _foo = types.ModuleType('foo')
  // _foo.__file__ = 'foo.pyc'
  // import sys
  // sys.module['foo'] = _foo
  // _source= scriptText
  // _code = compile(_source, 'foo.py', 'exec')
  // exec _code in _foo.__dict__
  // del _source
  // del _code
  // import foo
  std::ostringstream loadPythonModules;
  loadPythonModules << "import types" << std::endl;
  loadPythonModules << "_" << fileNameName << " = types.ModuleType('" << fileNameName << "')"
                    << std::endl;
  loadPythonModules << "_" << fileNameName << ".__file__ = '" << fileNameName << ".pyc'"
                    << std::endl;

  loadPythonModules << "import sys" << std::endl;
  loadPythonModules << "sys.modules['" << fileNameName << "'] = _" << fileNameName << std::endl;

  loadPythonModules << "_source = \"\"\"" << std::endl;
  loadPythonModules << scriptText;
  loadPythonModules << "\"\"\"" << std::endl;

  loadPythonModules << "_code = compile(_source, \"" << fileNameName << ".py\", \"exec\")"
                    << std::endl;
  loadPythonModules << "exec(_code, _" << fileNameName << ".__dict__)" << std::endl;
  loadPythonModules << "del _source" << std::endl;
  loadPythonModules << "del _code" << std::endl;
  loadPythonModules << "import " << fileNameName << std::endl;

  delete[] scriptPath;
  delete[] scriptText;

  vtkPythonInterpreter::RunSimpleString(loadPythonModules.str().c_str());
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::RequestDataDescription(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("dataDescription is NULL.");
    return 0;
  }

  InitializePython();

  // check the script to see if it should be run...
  vtkStdString dataDescriptionString = this->GetPythonAddress(dataDescription);

  std::ostringstream pythonInput;
  pythonInput << "dataDescription = vtkPVCatalystPython.vtkCPDataDescription('"
              << dataDescriptionString << "')\n"
              << this->PythonScriptName << ".RequestDataDescription(dataDescription)\n";

  vtkPythonInterpreter::RunSimpleString(pythonInput.str().c_str());

  return dataDescription->GetIfAnyGridNecessary() ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::CoProcess(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
  }

  InitializePython();

  vtkStdString dataDescriptionString = this->GetPythonAddress(dataDescription);

  std::ostringstream pythonInput;
  pythonInput << "dataDescription = vtkPVCatalystPython.vtkCPDataDescription('"
              << dataDescriptionString << "')\n"
              << this->PythonScriptName << ".DoCoProcessing(dataDescription)\n";

  vtkPythonInterpreter::RunSimpleString(pythonInput.str().c_str());

  return 1;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::Finalize()
{
  InitializePython();

  std::ostringstream pythonInput;
  pythonInput << "if hasattr(" << this->PythonScriptName << ", 'Finalize'):\n"
              << "  " << this->PythonScriptName << ".Finalize()\n";

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
  char* aplus = addressOfPointer;
  if ((addressOfPointer[0] == '0') && ((addressOfPointer[1] == 'x') || addressOfPointer[1] == 'X'))
  {
    aplus += 2; // skip over "0x"
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
