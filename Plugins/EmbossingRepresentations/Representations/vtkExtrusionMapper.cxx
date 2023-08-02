// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtrusionMapper.h"

#include "vtkMultiProcessController.h"
#include "vtkOpenGLExtrusionMapperDelegator.h"
#include "vtkPolyData.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkExtrusionMapper);
vtkSetObjectImplementationMacro(vtkExtrusionMapper, Controller, vtkMultiProcessController);

//-----------------------------------------------------------------------------
vtkExtrusionMapper::vtkExtrusionMapper()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->ResetDataRange();
  this->UserRange[0] = 0.;
  this->UserRange[1] = 1.;
}

//-----------------------------------------------------------------------------
vtkExtrusionMapper::~vtkExtrusionMapper()
{
  if (this->Controller)
  {
    this->Controller->Delete();
    this->Controller = nullptr;
  }
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NormalizeData: " << this->NormalizeData << std::endl;
  os << indent << "ExtrusionFactor: " << this->ExtrusionFactor << std::endl;
  os << indent << "BasisVisibility: " << this->BasisVisibility << std::endl;
  os << indent << "AutoScaling: " << this->AutoScaling << std::endl;
  if (!this->AutoScaling)
  {
    os << indent << "UserRange: " << this->UserRange[0] << ", " << this->UserRange[1] << std::endl;
  }
  os << indent << "BasisVisibility: " << this->BasisVisibility << std::endl;
}

//-----------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator* vtkExtrusionMapper::CreateADelegator()
{
  return vtkOpenGLExtrusionMapperDelegator::New();
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::ComputeBounds()
{
  vtkMTimeType time = this->BoundsMTime.GetMTime();

  this->Superclass::ComputeBounds();

  // if bounds are modified
  if (time < this->BoundsMTime.GetMTime())
  {
    vtkBoundingBox bbox(this->Bounds);
    this->MaxBoundsLength = bbox.GetMaxLength();
    // inflate bounds to handle extrusion in all directions
    bbox.Inflate(this->MaxBoundsLength);
    bbox.GetBounds(this->Bounds);
  }
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::SetExtrusionFactor(float factor)
{
  if (this->ExtrusionFactor == factor)
  {
    return;
  }

  this->ExtrusionFactor = factor;
  this->Modified();
}

// ---------------------------------------------------------------------------
void vtkExtrusionMapper::ResetDataRange()
{
  this->LocalDataRange[0] = VTK_DOUBLE_MAX;
  this->LocalDataRange[1] = VTK_DOUBLE_MIN;
  this->GlobalDataRange[0] = VTK_DOUBLE_MAX;
  this->GlobalDataRange[1] = VTK_DOUBLE_MIN;
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::PreRender(
  const std::vector<vtkSmartPointer<vtkCompositePolyDataMapperDelegator>>& delegators, vtkRenderer*,
  vtkActor*)
{
  // Compute the local range of the data that will be used as extrusion factors
  double range[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  if (!this->NormalizeData == 0)
  {
    return;
  }

  for (auto& delegator : delegators)
  {
    vtkOpenGLExtrusionMapperDelegator* glDelegator =
      static_cast<vtkOpenGLExtrusionMapperDelegator*>(delegator.Get());
    if (this->ExtrusionFactor == 0.f)
    {
      glDelegator->SetNeedRebuild(true);
    }
    double lrange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
    glDelegator->GetDataRange(lrange);
    range[0] = std::min(range[0], lrange[0]);
    range[1] = std::max(range[1], lrange[1]);
  }

  if (range[0] != this->LocalDataRange[0] || range[1] != this->LocalDataRange[1])
  {
    this->GlobalDataRange[0] = range[0];
    this->GlobalDataRange[1] = range[1];
    this->LocalDataRange[0] = range[0];
    this->LocalDataRange[1] = range[1];

    // In parallel, we need to reduce the local ranges to get the global ones.
    if (!this->Controller)
    {
      this->Controller = vtkMultiProcessController::GetGlobalController();
    }
    if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
    {
      this->Controller->AllReduce(&range[0], &this->GlobalDataRange[0], 1, vtkCommunicator::MIN_OP);
      this->Controller->AllReduce(&range[1], &this->GlobalDataRange[1], 1, vtkCommunicator::MAX_OP);
    }
  }
}

//----------------------------------------------------------------------------
void vtkExtrusionMapper::SetInputArrayToProcess(int idx, vtkInformation* inInfo)
{
  this->Superclass::SetInputArrayToProcess(idx, inInfo);

  this->FieldAssociation = inInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
  this->ResetDataRange();
}

//----------------------------------------------------------------------------
void vtkExtrusionMapper::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, int attributeType)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, attributeType);

  this->FieldAssociation = fieldAssociation;
  this->ResetDataRange();
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);

  this->FieldAssociation = fieldAssociation;
  this->ResetDataRange();
}
