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

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolarAxesRepresentation.h"
#include "vtkSelectionRepresentation.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkPVCompositeRepresentation);
vtkCxxSetObjectMacro(
  vtkPVCompositeRepresentation, SelectionRepresentation, vtkSelectionRepresentation);
vtkCxxSetObjectMacro(
  vtkPVCompositeRepresentation, PolarAxesRepresentation, vtkPolarAxesRepresentation);

//----------------------------------------------------------------------------
vtkPVCompositeRepresentation::vtkPVCompositeRepresentation()
{
  this->SelectionRepresentation = vtkSelectionRepresentation::New();
  this->PolarAxesRepresentation = NULL;

  this->SelectionVisibility = false;
  this->SelectionRepresentation->SetVisibility(false);
}

//----------------------------------------------------------------------------
vtkPVCompositeRepresentation::~vtkPVCompositeRepresentation()
{
  this->SetSelectionRepresentation(NULL);
  this->SetPolarAxesRepresentation(NULL);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  this->SetSelectionVisibility(this->SelectionVisibility);
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
  if (this->PolarAxesRepresentation)
  {
    this->PolarAxesRepresentation->SetForcedCacheKey(val);
  }

  this->Superclass::SetForcedCacheKey(val);
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
  if (this->PolarAxesRepresentation)
  {
    minId = this->PolarAxesRepresentation->Initialize(minId, maxIdAvailable);
  }
  return  this->Superclass::Initialize(minId, maxIdAvailable);
}
