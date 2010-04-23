/*=========================================================================

  Module:    vtkPVBatchOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBatchOptions.h"

#include "vtkObjectFactory.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVBatchOptions);

//----------------------------------------------------------------------------
vtkPVBatchOptions::vtkPVBatchOptions()
{
  this->RequireBatchScript = 1;
  this->BatchScriptName = 0;
  this->ServerMode = 0;
}

//----------------------------------------------------------------------------
vtkPVBatchOptions::~vtkPVBatchOptions()
{
  this->SetBatchScriptName(0);
}

//----------------------------------------------------------------------------
void vtkPVBatchOptions::Initialize()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
int vtkPVBatchOptions::PostProcess(int argc, const char* const* argv)
{
  if ( this->RequireBatchScript && !this->BatchScriptName )
    {
    this->SetErrorMessage("Batch script not specified");
    return 0;
    }
  if ( this->BatchScriptName && 
    vtksys::SystemTools::GetFilenameLastExtension(this->BatchScriptName) != ".pvb")
    {
    vtksys_ios::ostringstream str;
    str << "Wrong batch script name: " << this->BatchScriptName << ends;
    this->SetErrorMessage(str.str().c_str());
    return 0;
    }
  return this->Superclass::PostProcess(argc, argv);
}

int vtkPVBatchOptions::WrongArgument(const char* argument)
{
  if ( vtksys::SystemTools::FileExists(argument) &&
    vtksys::SystemTools::GetFilenameLastExtension(argument) == ".pvb")
    {
    this->SetBatchScriptName(argument);
    return 1;
    }

  return this->Superclass::WrongArgument(argument);
}

//----------------------------------------------------------------------------
void vtkPVBatchOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BatchScriptName: " << (this->BatchScriptName?this->BatchScriptName:"(none)") << endl;

}

