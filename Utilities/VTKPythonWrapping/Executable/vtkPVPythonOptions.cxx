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
#include "vtkParallelRenderManager.h"
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
  this->ServerMode = 0;
  this->EnableSymmetricScripting = false;
}

//----------------------------------------------------------------------------
vtkPVPythonOptions::~vtkPVPythonOptions()
{
  this->SetPythonScriptName(0);
}

//----------------------------------------------------------------------------
void vtkPVPythonOptions::Initialize()
{
  this->Superclass::Initialize();
  this->AddBooleanArgument("--symmetric", "-sym",
    &this->EnableSymmetricScripting,
    "When specified, the python script is processed symmetrically on all processes.",
    vtkPVOptions::PVBATCH);
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

  if (this->EnableSymmetricScripting)
    {
    // Disable render event propagation since satellites are no longer doing
    // ProcessRMIs() since symmetric script processing is enabled.
    vtkParallelRenderManager::SetDefaultRenderEventPropagation(false);
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
vtkSelfConnection* vtkPVPythonOptions::NewSelfConnection()
{
  if (this->EnableSymmetricScripting &&
    vtkProcessModule::GetProcessModule()->GetUseMPI())
    {
    return vtkSynchronousMPISelfConnection::New();
    }

  return this->Superclass::NewSelfConnection();
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
      stream << this->PythonScriptName << this->EnableSymmetricScripting;
      controller->Broadcast(stream, 0);
      }
    else
      {
      controller->Broadcast(stream, 0);
      vtkstd::string name;
      stream >> name >> this->EnableSymmetricScripting;
      this->SetPythonScriptName(name.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVPythonOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EnableSymmetricScripting: " << this->EnableSymmetricScripting << endl;
}

