/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonScriptV2Pipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPPythonScriptV2Pipeline.h"

#include "vtkCPDataDescription.h"
#include "vtkCPPythonScriptV2Helper.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCPPythonScriptV2Pipeline);
//----------------------------------------------------------------------------
vtkCPPythonScriptV2Pipeline::vtkCPPythonScriptV2Pipeline()
  : CoProcessHasBeenCalled(false)
{
}

//----------------------------------------------------------------------------
vtkCPPythonScriptV2Pipeline::~vtkCPPythonScriptV2Pipeline()
{
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Pipeline::Initialize(const char* path)
{
  return this->Helper->PrepareFromScript(path);
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptV2Pipeline::RequestDataDescription(vtkCPDataDescription* dataDescription)
{
  if (!this->CoProcessHasBeenCalled)
  {
    /**
     * The new style Catalyst scripts may create pipeline immediately with the
     * script is imported. If that happens, the pipeline creation may fail if the
     * data producers are not ready with data. To avoid that case, for the first
     * call to `RequestDataDescription` we skip the Python script entirely and
     * simply request all meshes and fields. Since we do this only for the 1st
     * timestep, it should not impact too much.
     */
    return 1;
  }

  return (this->Helper->IsImported() && this->Helper->RequestDataDescription(dataDescription) &&
           dataDescription->GetIfAnyGridNecessary())
    ? 1
    : 0;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptV2Pipeline::CoProcess(vtkCPDataDescription* dataDescription)
{
  if (!this->CoProcessHasBeenCalled)
  {
    // i.e. first invocation.
    this->CoProcessHasBeenCalled = true;

    if (!this->Helper->Import(dataDescription))
    {
      return false;
    }
    if (!this->Helper->CatalystInitialize(dataDescription))
    {
      return 0;
    }

    // let's make sure if the script has custom request_data_description
    // it gets called since we skipped it earlier.
    if (!this->RequestDataDescription(dataDescription))
    {
      return 0;
    }
  }

  return this->Helper->IsImported() && this->Helper->CatalystExecute(dataDescription) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptV2Pipeline::Finalize()
{
  this->Helper->CatalystFinalize();
  return this->Superclass::Finalize();
}

//----------------------------------------------------------------------------
void vtkCPPythonScriptV2Pipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
