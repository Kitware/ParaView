/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonStringPipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkCPPythonStringPipeline.h"

#include "vtkCPDataDescription.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"

#include <sstream>

vtkStandardNewMacro(vtkCPPythonStringPipeline);
//----------------------------------------------------------------------------
vtkCPPythonStringPipeline::vtkCPPythonStringPipeline()
{
}

//----------------------------------------------------------------------------
vtkCPPythonStringPipeline::~vtkCPPythonStringPipeline()
{
}

//----------------------------------------------------------------------------
int vtkCPPythonStringPipeline::Initialize(const char* pythonString)
{
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

  std::string line;
  std::istringstream myString(pythonString);
  std::string desiredString;
  while (getline(myString, line))
  {
    this->FixEOL(line);
    desiredString.append(line).append("\n");
  }

  // Create a unique module name for this script.
  static std::string moduleName = "ABCD";
  moduleName.append("Z");
  this->ModuleName = moduleName;
  std::ostringstream loadPythonModules;
  loadPythonModules << "import types" << std::endl;
  loadPythonModules << "_" << this->ModuleName << " = types.ModuleType('" << this->ModuleName
                    << "')" << std::endl;
  loadPythonModules << "_" << this->ModuleName << ".__file__ = '" << this->ModuleName << ".pyc'"
                    << std::endl;

  loadPythonModules << "import sys" << std::endl;
  loadPythonModules << "sys.modules['" << this->ModuleName << "'] = _" << this->ModuleName
                    << std::endl;

  loadPythonModules << "_source = \"\"\"" << std::endl;
  loadPythonModules << desiredString;
  loadPythonModules << "\"\"\"" << std::endl;

  loadPythonModules << "_code = compile(_source, \"" << this->ModuleName << ".py\", \"exec\")"
                    << std::endl;
  loadPythonModules << "exec(_code, _" << this->ModuleName << ".__dict__)" << std::endl;
  loadPythonModules << "del _source" << std::endl;
  loadPythonModules << "del _code" << std::endl;
  loadPythonModules << "import " << this->ModuleName << std::endl;
  loadPythonModules << "from paraview.modules import vtkPVCatalyst" << std::endl;

  vtkPythonInterpreter::RunSimpleString(loadPythonModules.str().c_str());
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPPythonStringPipeline::RequestDataDescription(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("dataDescription is NULL.");
    return 0;
  }

  // check the script to see if it should be run...
  std::string dataDescriptionString = this->GetPythonAddress(dataDescription);

  std::ostringstream pythonInput;
  pythonInput << "dataDescription = vtkPVCatalyst.vtkCPDataDescription('" << dataDescriptionString
              << "')\n"
              << this->ModuleName << ".RequestDataDescription(dataDescription)\n";

  vtkPythonInterpreter::RunSimpleString(pythonInput.str().c_str());

  return dataDescription->GetIfAnyGridNecessary() ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkCPPythonStringPipeline::CoProcess(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
  }

  std::string dataDescriptionString = this->GetPythonAddress(dataDescription);

  std::ostringstream pythonInput;
  pythonInput << "dataDescription = vtkPVCatalyst.vtkCPDataDescription('" << dataDescriptionString
              << "')\n"
              << this->ModuleName << ".DoCoProcessing(dataDescription)\n";

  vtkPythonInterpreter::RunSimpleString(pythonInput.str().c_str());

  return 1;
}

//----------------------------------------------------------------------------
int vtkCPPythonStringPipeline::Finalize()
{
  std::ostringstream pythonInput;
  pythonInput << "if hasattr(" << this->ModuleName << ", 'Finalize'):\n"
              << "  " << this->ModuleName << ".Finalize()\n";

  vtkPythonInterpreter::RunSimpleString(pythonInput.str().c_str());

  return 1;
}

//----------------------------------------------------------------------------
void vtkCPPythonStringPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ModuleName: " << this->ModuleName << "\n";
}
