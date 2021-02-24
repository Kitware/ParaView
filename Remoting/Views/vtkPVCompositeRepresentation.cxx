/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeRepresentation.h"

#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPVGridAxes3DRepresentation.h"
#include "vtkPolarAxesRepresentation.h"
#include "vtkSelectionRepresentation.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkPVCompositeRepresentation);
vtkCxxSetObjectMacro(
  vtkPVCompositeRepresentation, SelectionRepresentation, vtkSelectionRepresentation);
vtkCxxSetObjectMacro(
  vtkPVCompositeRepresentation, PolarAxesRepresentation, vtkPolarAxesRepresentation);
vtkCxxSetObjectMacro(
  vtkPVCompositeRepresentation, GridAxesRepresentation, vtkPVGridAxes3DRepresentation);

//----------------------------------------------------------------------------
vtkPVCompositeRepresentation::vtkPVCompositeRepresentation()
{
  this->SelectionRepresentation = vtkSelectionRepresentation::New();
  this->PolarAxesRepresentation = nullptr;

  this->SelectionVisibility = false;
  this->SelectionRepresentation->SetVisibility(false);

  this->GridAxesRepresentation = vtkPVGridAxes3DRepresentation::New();
  this->GridAxesRepresentation->SetVisibility(false);
}

//----------------------------------------------------------------------------
vtkPVCompositeRepresentation::~vtkPVCompositeRepresentation()
{
  this->SetSelectionRepresentation(nullptr);
  this->SetPolarAxesRepresentation(nullptr);
  this->SetGridAxesRepresentation(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  this->SetSelectionVisibility(this->SelectionVisibility);
  this->GridAxesRepresentation->SetVisibility(visible);
  this->SetPolarAxesVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetSelectionVisibility(bool visible)
{
  this->SelectionVisibility = visible;
  this->SelectionRepresentation->SetVisibility(this->GetVisibility() && visible);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetPolarAxesVisibility(bool visible)
{
  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->SetParentVisibility(visible);
  }
}

//----------------------------------------------------------------------------
bool vtkPVCompositeRepresentation::AddToView(vtkView* view)
{
  if (!this->Superclass::AddToView(view))
  {
    return false;
  }

  view->AddRepresentation(this->SelectionRepresentation);
  view->AddRepresentation(this->GridAxesRepresentation);

  if (this->PolarAxesRepresentation)
  {
    view->AddRepresentation(this->PolarAxesRepresentation);
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVCompositeRepresentation::RemoveFromView(vtkView* view)
{
  view->RemoveRepresentation(this->SelectionRepresentation);
  view->RemoveRepresentation(this->GridAxesRepresentation);

  if (this->PolarAxesRepresentation)
  {
    view->RemoveRepresentation(this->PolarAxesRepresentation);
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::MarkModified()
{
  this->SelectionRepresentation->MarkModified();
  this->GridAxesRepresentation->MarkModified();

  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->MarkModified();
  }

  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetUpdateTime(double time)
{
  this->SelectionRepresentation->SetUpdateTime(time);
  this->GridAxesRepresentation->SetUpdateTime(time);

  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->SetUpdateTime(time);
  }

  this->Superclass::SetUpdateTime(time);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetForceUseCache(bool val)
{
  this->SelectionRepresentation->SetForceUseCache(val);
  this->GridAxesRepresentation->SetForceUseCache(val);

  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->SetForceUseCache(val);
  }

  this->Superclass::SetForceUseCache(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetForcedCacheKey(double val)
{
  this->SelectionRepresentation->SetForcedCacheKey(val);
  this->GridAxesRepresentation->SetForcedCacheKey(val);

  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->SetForcedCacheKey(val);
  }

  this->Superclass::SetForcedCacheKey(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->GridAxesRepresentation->SetInputConnection(port, input);
  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->SetInputConnection(port, input);
  }
  this->Superclass::SetInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  this->GridAxesRepresentation->SetInputConnection(input);
  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->SetInputConnection(input);
  }
  this->Superclass::SetInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->GridAxesRepresentation->AddInputConnection(port, input);
  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->AddInputConnection(port, input);
  }
  this->Superclass::AddInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  this->GridAxesRepresentation->AddInputConnection(input);
  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->AddInputConnection(input);
  }
  this->Superclass::AddInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->GridAxesRepresentation->RemoveInputConnection(port, input);
  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->RemoveInputConnection(port, input);
  }
  this->Superclass::RemoveInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::RemoveInputConnection(int port, int idx)
{
  this->GridAxesRepresentation->RemoveInputConnection(port, idx);
  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->RemoveInputConnection(port, idx);
  }
  this->Superclass::RemoveInputConnection(port, idx);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetPointFieldDataArrayName(const char* val)
{
  this->SelectionRepresentation->SetPointFieldDataArrayName(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetCellFieldDataArrayName(const char* val)
{
  this->SelectionRepresentation->SetCellFieldDataArrayName(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
unsigned int vtkPVCompositeRepresentation::Initialize(
  unsigned int minIdAvailable, unsigned int maxIdAvailable)
{
  unsigned int minId = minIdAvailable;
  minId = this->SelectionRepresentation->Initialize(minId, maxIdAvailable);
  minId = this->GridAxesRepresentation->Initialize(minId, maxIdAvailable);
  if (this->PolarAxesRepresentation)
  {
    minId = this->PolarAxesRepresentation->Initialize(minId, maxIdAvailable);
  }
  return this->Superclass::Initialize(minId, maxIdAvailable);
}
