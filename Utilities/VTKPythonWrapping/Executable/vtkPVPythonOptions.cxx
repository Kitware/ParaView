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

#include "vtkObjectFactory.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPythonOptions);
vtkCxxRevisionMacro(vtkPVPythonOptions, "1.2");

//----------------------------------------------------------------------------
vtkPVPythonOptions::vtkPVPythonOptions()
{
  this->PythonScriptName = 0;
  this->ServerMode = 0;
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
}

//----------------------------------------------------------------------------
int vtkPVPythonOptions::PostProcess(int argc, const char* const* argv)
{
  if ( this->PythonScriptName && 
    vtksys::SystemTools::GetFilenameLastExtension(this->PythonScriptName) != ".py")
    {
    ostrstream str;
    str << "Wrong batch script name: " << this->PythonScriptName << ends;
    this->SetErrorMessage(str.str());
    str.rdbuf()->freeze(0);
    return 0;
    }
  return this->Superclass::PostProcess(argc, argv);
}

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
void vtkPVPythonOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

