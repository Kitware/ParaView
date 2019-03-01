/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectionRepresentation.h"

#include "vtkDataLabelRepresentation.h"
#include "vtkGeometryRepresentation.h"
#include "vtkInformation.h"
#include "vtkLabeledDataMapper.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkSelectionRepresentation);
vtkCxxSetObjectMacro(vtkSelectionRepresentation, LabelRepresentation, vtkDataLabelRepresentation);
//----------------------------------------------------------------------------
vtkSelectionRepresentation::vtkSelectionRepresentation()
{
  this->GeometryRepresentation = vtkGeometryRepresentation::New();
  this->GeometryRepresentation->SetPickable(0);
  this->GeometryRepresentation->RequestGhostCellsIfNeededOff();

  this->LabelRepresentation = vtkDataLabelRepresentation::New();
  this->LabelRepresentation->SetPointLabelMode(VTK_LABEL_FIELD_DATA);
  this->LabelRepresentation->SetCellLabelMode(VTK_LABEL_FIELD_DATA);

  vtkCommand* observer =
    vtkMakeMemberFunctionCommand(*this, &vtkSelectionRepresentation::TriggerUpdateDataEvent);
  this->GeometryRepresentation->AddObserver(vtkCommand::UpdateDataEvent, observer);
  this->LabelRepresentation->AddObserver(vtkCommand::UpdateDataEvent, observer);
  observer->Delete();
}

//----------------------------------------------------------------------------
vtkSelectionRepresentation::~vtkSelectionRepresentation()
{
  this->GeometryRepresentation->Delete();
  this->LabelRepresentation->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->GeometryRepresentation->SetInputConnection(port, input);
  this->LabelRepresentation->SetInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  this->GeometryRepresentation->SetInputConnection(input);
  this->LabelRepresentation->SetInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->GeometryRepresentation->AddInputConnection(port, input);
  this->LabelRepresentation->AddInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  this->GeometryRepresentation->AddInputConnection(input);
  this->LabelRepresentation->AddInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->GeometryRepresentation->RemoveInputConnection(port, input);
  this->LabelRepresentation->RemoveInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::RemoveInputConnection(int port, int idx)
{
  this->GeometryRepresentation->RemoveInputConnection(port, idx);
  this->LabelRepresentation->RemoveInputConnection(port, idx);
}

//----------------------------------------------------------------------------
int vtkSelectionRepresentation::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetUpdateTime(double val)
{
  this->GeometryRepresentation->SetUpdateTime(val);
  this->LabelRepresentation->SetUpdateTime(val);
  this->Superclass::SetUpdateTime(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetForceUseCache(bool val)
{
  this->GeometryRepresentation->SetForceUseCache(val);
  this->LabelRepresentation->SetForceUseCache(val);
  this->Superclass::SetForceUseCache(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetForcedCacheKey(double val)
{
  this->GeometryRepresentation->SetForcedCacheKey(val);
  this->LabelRepresentation->SetForcedCacheKey(val);
  this->Superclass::SetForcedCacheKey(val);
}

//----------------------------------------------------------------------------
bool vtkSelectionRepresentation::AddToView(vtkView* view)
{
  view->AddRepresentation(this->GeometryRepresentation);
  view->AddRepresentation(this->LabelRepresentation);
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSelectionRepresentation::RemoveFromView(vtkView* view)
{
  view->RemoveRepresentation(this->GeometryRepresentation);
  view->RemoveRepresentation(this->LabelRepresentation);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::MarkModified()
{
  this->GeometryRepresentation->MarkModified();
  this->LabelRepresentation->MarkModified();
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::TriggerUpdateDataEvent()
{
  // we need to mark the geometry as always needing to be moved. The reason
  // is that in client server mode and the first interaction with the renderer
  // is a selection the geometryrepr is properly marked for modification.
  // this shouldn't degrade performance as the geometryRepr in most other
  // cases is already dirty ( Bug #11587)
  this->GeometryRepresentation->MarkModified();

  // We fire UpdateDataEvent to notify the representation proxy that the
  // representation was updated. The representation proxty will then call
  // PostUpdateData(). We do this since now representations are not updated at
  // the proxy level.
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetColor(double r, double g, double b)
{
  this->GeometryRepresentation->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetLineWidth(double val)
{
  this->GeometryRepresentation->SetLineWidth(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetOpacity(double val)
{
  this->GeometryRepresentation->SetOpacity(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetPointSize(double val)
{
  this->GeometryRepresentation->SetPointSize(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetVisibility(bool val)
{
  this->GeometryRepresentation->SetVisibility(val);
  this->LabelRepresentation->SetVisibility(val);
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetUseOutline(int val)
{
  this->GeometryRepresentation->SetUseOutline(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetRenderPointsAsSpheres(bool val)
{
  this->GeometryRepresentation->SetRenderPointsAsSpheres(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetRenderLinesAsTubes(bool val)
{
  this->GeometryRepresentation->SetRenderLinesAsTubes(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetRepresentation(int val)
{
  this->GeometryRepresentation->SetRepresentation(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetOrientation(double x, double y, double z)
{
  this->GeometryRepresentation->SetOrientation(x, y, z);
  this->LabelRepresentation->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetOrigin(double x, double y, double z)
{
  this->GeometryRepresentation->SetOrigin(x, y, z);
  this->LabelRepresentation->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetPosition(double x, double y, double z)
{
  this->GeometryRepresentation->SetPosition(x, y, z);
  this->LabelRepresentation->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetScale(double x, double y, double z)
{
  this->GeometryRepresentation->SetScale(x, y, z);
  this->LabelRepresentation->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetUserTransform(const double matrix[16])
{
  this->GeometryRepresentation->SetUserTransform(matrix);
  this->LabelRepresentation->SetUserTransform(matrix);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetPointFieldDataArrayName(const char* val)
{
  this->LabelRepresentation->SetPointFieldDataArrayName(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetCellFieldDataArrayName(const char* val)
{
  this->LabelRepresentation->SetCellFieldDataArrayName(val);
}

//----------------------------------------------------------------------------
void vtkSelectionRepresentation::SetLogName(const std::string& name)
{
  this->Superclass::SetLogName(name);

  // we need to label GeometryRepresentation since it's not set via public API.
  this->GeometryRepresentation->SetLogName(this->GetLogName() + "/Geometry");
}

//----------------------------------------------------------------------------
unsigned int vtkSelectionRepresentation::Initialize(
  unsigned int minIdAvailable, unsigned int maxIdAvailable)
{
  unsigned int minId = minIdAvailable;
  minId = this->LabelRepresentation->Initialize(minId, maxIdAvailable);
  minId = this->GeometryRepresentation->Initialize(minId, maxIdAvailable);

  return this->Superclass::Initialize(minId, maxIdAvailable);
}
