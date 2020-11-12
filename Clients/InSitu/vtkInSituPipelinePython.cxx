/*=========================================================================

  Program:   ParaView
  Module:    vtkInSituPipelinePython.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "vtkPython.h" // must be first
#endif

#include "vtkInSituPipelinePython.h"

#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "vtkCPPythonScriptV2Helper.h"
#else
// dummy implementation.
class vtkCPPythonScriptV2Helper : public vtkObject
{
public:
  static vtkCPPythonScriptV2Helper* New();

private:
  vtkCPPythonScriptV2Helper() = default;
  ~vtkCPPythonScriptV2Helper() = default;
  vtkCPPythonScriptV2Helper(const vtkCPPythonScriptV2Helper&) = delete;
  void operator=(const vtkCPPythonScriptV2Helper&) = delete;
};
vtkStandardNewMacro(vtkCPPythonScriptV2Helper);
#endif

vtkStandardNewMacro(vtkInSituPipelinePython);
//----------------------------------------------------------------------------
vtkInSituPipelinePython::vtkInSituPipelinePython()
  : FileName(nullptr)
{
}

//----------------------------------------------------------------------------
vtkInSituPipelinePython::~vtkInSituPipelinePython()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Initialize()
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  // call some Python API.
  auto status = Py_IsInitialized();
  (void)status;
  return this->Helper->PrepareFromScript(this->FileName) && this->Helper->Import() &&
    this->Helper->CatalystInitialize();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Execute(int timestep, double time)
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  return this->Helper->CatalystExecute(timestep, time);
#else
  (void)time;
  (void)timestep;
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Finalize()
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  return this->Helper->CatalystFinalize();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkInSituPipelinePython::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
}
