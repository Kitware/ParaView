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

#include "vtkCubeAxesRepresentation.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionRepresentation.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkPVCompositeRepresentation);
vtkCxxSetObjectMacro(vtkPVCompositeRepresentation, SelectionRepresentation,
  vtkSelectionRepresentation);
vtkCxxSetObjectMacro(vtkPVCompositeRepresentation, CubeAxesRepresentation,
  vtkCubeAxesRepresentation);
//----------------------------------------------------------------------------
vtkPVCompositeRepresentation::vtkPVCompositeRepresentation()
{
  this->SetNumberOfInputPorts(2);
  this->SelectionRepresentation = vtkSelectionRepresentation::New();
  this->CubeAxesRepresentation = vtkCubeAxesRepresentation::New();

  this->CubeAxesVisibility = false;
  this->SelectionVisibility = false;
  this->SelectionRepresentation->SetVisibility(false);
  this->CubeAxesRepresentation->SetVisibility(false);
}

//----------------------------------------------------------------------------
vtkPVCompositeRepresentation::~vtkPVCompositeRepresentation()
{
  this->SetSelectionRepresentation(NULL);
  this->SetCubeAxesRepresentation(NULL);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  this->SetCubeAxesVisibility(this->CubeAxesVisibility);
  this->SetSelectionVisibility(this->SelectionVisibility);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetCubeAxesVisibility(bool visible)
{
  this->CubeAxesVisibility = visible;
  this->CubeAxesRepresentation->SetVisibility(this->GetVisibility() && visible);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetSelectionVisibility(bool visible)
{
  this->SelectionVisibility = visible;
  this->SelectionRepresentation->SetVisibility(
    this->GetVisibility() && visible);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  if (port == 0)
    {
    this->CubeAxesRepresentation->SetInputConnection(0, input);
    this->Superclass::SetInputConnection(0, input);
    }
  else if (port == 1)
    {
    this->SelectionRepresentation->SetInputConnection(0, input);
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  // port is assumed to be 0.
  this->CubeAxesRepresentation->SetInputConnection(input);
  this->Superclass::SetInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::AddInputConnection(
  int port, vtkAlgorithmOutput* input)
{
 if (port == 0)
    {
    this->CubeAxesRepresentation->AddInputConnection(0, input);
    this->Superclass::AddInputConnection(0, input);
    }
  else if (port == 1)
    {
    this->SelectionRepresentation->AddInputConnection(0, input);
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  // port is assumed to be 0.
  this->CubeAxesRepresentation->AddInputConnection(input);
  this->Superclass::AddInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  if (port == 0)
    {
    this->CubeAxesRepresentation->RemoveInputConnection(0, input);
    this->Superclass::RemoveInputConnection(0, input);
    }
  else if (port == 1)
    {
    this->SelectionRepresentation->RemoveInputConnection(0, input);
    }
}

//----------------------------------------------------------------------------
bool vtkPVCompositeRepresentation::AddToView(vtkView* view)
{
  view->AddRepresentation(this->CubeAxesRepresentation);
  view->AddRepresentation(this->SelectionRepresentation);
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkPVCompositeRepresentation::RemoveFromView(vtkView* view)
{
  view->RemoveRepresentation(this->CubeAxesRepresentation);
  view->RemoveRepresentation(this->SelectionRepresentation);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::MarkModified()
{
  this->CubeAxesRepresentation->MarkModified();
  this->SelectionRepresentation->MarkModified();
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
int vtkPVCompositeRepresentation::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    return this->Superclass::FillInputPortInformation(port, info);
    }

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetUpdateTime(double time)
{
  this->CubeAxesRepresentation->SetUpdateTime(time);
  this->SelectionRepresentation->SetUpdateTime(time);
  this->Superclass::SetUpdateTime(time);
}
//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetUseCache(bool val)
{
  this->CubeAxesRepresentation->SetUseCache(val);
  this->SelectionRepresentation->SetUseCache(val);
  this->Superclass::SetUseCache(val);
}
//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetCacheKey(double val)
{
  this->CubeAxesRepresentation->SetCacheKey(val);
  this->SelectionRepresentation->SetCacheKey(val);
  this->Superclass::SetCacheKey(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetForceUseCache(bool val)
{
  this->CubeAxesRepresentation->SetForceUseCache(val);
  this->SelectionRepresentation->SetForceUseCache(val);
  this->Superclass::SetForceUseCache(val);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRepresentation::SetForcedCacheKey(double val)
{
  this->CubeAxesRepresentation->SetForcedCacheKey(val);
  this->SelectionRepresentation->SetForcedCacheKey(val);
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
