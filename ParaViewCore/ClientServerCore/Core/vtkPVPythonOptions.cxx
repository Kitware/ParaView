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
#include "vtkPSystemTools.h"
#include "vtkProcessModule.h"

#include <sstream>
#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>

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
  if (this->PythonScriptName &&
    vtksys::SystemTools::GetFilenameLastExtension(this->PythonScriptName) != ".py")
  {
    std::ostringstream str;
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
  if (vtkPSystemTools::FileExists(argument) &&
    vtksys::SystemTools::GetFilenameLastExtension(argument) == ".py")
  {
    this->SetPythonScriptName(argument);
    return 1;
  }

  // All arguments are simply passed to the python interpretor.
  // Returning 0 tells CommandLineArguments that the "argument" was not a
  // handled and hence it leaves it in the "remaining arguments" collection. We
  // query the remaining arguments and simply pass them to the python
  // interpretor.
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVPythonOptions::Synchronize()
{
  // TODO: Need to synchronize all options, for now just the script name.
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    vtkMultiProcessStream stream;
    if (controller->GetLocalProcessId() == 0)
    {
      if (this->PythonScriptName)
      {
        stream << (int)1 << this->PythonScriptName << this->GetSymmetricMPIMode();
      }
      else
      {
        stream << (int)0 << this->GetSymmetricMPIMode();
      }
      controller->Broadcast(stream, 0);
    }
    else
    {
      controller->Broadcast(stream, 0);
      int hasScriptName;
      stream >> hasScriptName;
      if (hasScriptName == 0)
      {
        this->SetPythonScriptName(NULL);
      }
      else
      {
        std::string name;
        stream >> name;
        this->SetPythonScriptName(name.c_str());
      }
      stream >> this->SymmetricMPIMode;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVPythonOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
