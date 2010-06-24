/*=========================================================================

  Module:    vtkPVPythonOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPythonOptions.h"

#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSynchronousMPISelfConnection.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPythonOptions);

//----------------------------------------------------------------------------
vtkPVPythonOptions::vtkPVPythonOptions()
{
  this->PythonScriptName = 0;
}

//----------------------------------------------------------------------------
vtkPVPythonOptions::~vtkPVPythonOptions()
{
  this->SetPythonScriptName(0);
}

//----------------------------------------------------------------------------
int vtkPVPythonOptions::PostProcess(int argc, const char* const* argv)
{
  if ( this->PythonScriptName && 
    vtksys::SystemTools::GetFilenameLastExtension(this->PythonScriptName) != ".py")
    {
    vtksys_ios::ostringstream str;
    str << "Wrong batch script name: " << this->PythonScriptName;
    this->SetErrorMessage(str.str().c_str());
    return 0;
    }

  this->Synchronize();

  return this->Superclass::PostProcess(argc, argv);
}

//----------------------------------------------------------------------------
int vtkPVPythonOptions::WrongArgument(const char* argument)
{
  if ( vtksys::SystemTools::FileExists(argument) &&
    vtksys::SystemTools::GetFilenameLastExtension(argument) == ".py")
    {
    this->SetPythonScriptName(argument);
    return 1;
    }

  this->Superclass::WrongArgument(argument);
  // All arguments are simply passed to the python interpretor.
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPythonOptions::Synchronize()
{
  // TODO: Need to synchronize all options, for now just the script name.
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();

  if (controller && controller->GetNumberOfProcesses() > 1)
    {
    vtkMultiProcessStream stream;
    if (controller->GetLocalProcessId() == 0)
      {
      stream << this->PythonScriptName << this->GetSymmetricMPIMode();
      controller->Broadcast(stream, 0);
      }
    else
      {
      controller->Broadcast(stream, 0);
      vtkstd::string name;
      stream >> name >> this->SymmetricMPIMode;
      this->SetPythonScriptName(name.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVPythonOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

