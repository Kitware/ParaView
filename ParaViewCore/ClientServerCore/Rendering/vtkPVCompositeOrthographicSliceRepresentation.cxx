/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeOrthographicSliceRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeOrthographicSliceRepresentation.h"

#include "vtkCubeAxesRepresentation.h"
#include "vtkGeometrySliceRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkPVOrthographicSliceView.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkPVCompositeOrthographicSliceRepresentation);
//----------------------------------------------------------------------------
vtkPVCompositeOrthographicSliceRepresentation::vtkPVCompositeOrthographicSliceRepresentation()
{
}

//----------------------------------------------------------------------------
vtkPVCompositeOrthographicSliceRepresentation::~vtkPVCompositeOrthographicSliceRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::SetSliceRepresentation(
  int index, vtkGeometrySliceRepresentation* repr)
{
  this->SliceRepresentations[index] = repr;
  if (repr)
    {
    // Tell each of the vtkGeometrySliceRepresentation instances to only compute 1
    // slice (rather than 3 slices).
    repr->SetMode(index);
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::SetCubeAxesRepresentation(
  int index, vtkCubeAxesRepresentation* repr)
{
  this->CubeAxesRepresentations[index] = repr;
  if (repr)
    {
    // Tell each of the vtkCubeAxesRepresentation instances to add to a specific
    // renderer.
    repr->SetRendererType(index + vtkPVOrthographicSliceView::SAGITTAL_VIEW_RENDERER);
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::SetVisibility(bool visible)
{
  for(int cc=0; cc < 3; cc++)
    {
    this->SliceRepresentations[cc]->SetVisibility(visible);
    this->CubeAxesRepresentations[cc]->SetVisibility(visible && this->CubeAxesVisibility);
    }
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::SetCubeAxesVisibility(bool visible)
{
  for(int cc=0; cc < 3; cc++)
    {
    this->CubeAxesRepresentations[cc]->SetVisibility(visible && this->GetVisibility());
    }
  this->Superclass::SetCubeAxesVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  for(int cc=0; cc < 3; cc++)
    {
    this->SliceRepresentations[cc]->SetInputConnection(port, input);
    this->CubeAxesRepresentations[cc]->SetInputConnection(port, input);
    }
  this->Superclass::SetInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  for(int cc=0; cc < 3; cc++)
    {
    this->SliceRepresentations[cc]->SetInputConnection(input);
    this->CubeAxesRepresentations[cc]->SetInputConnection(input);
    }
  this->Superclass::SetInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  for(int cc=0; cc < 3; cc++)
    {
    this->SliceRepresentations[cc]->AddInputConnection(port, input);
    this->CubeAxesRepresentations[cc]->AddInputConnection(port, input);
    }
  this->Superclass::AddInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  for(int cc=0; cc < 3; cc++)
    {
    this->SliceRepresentations[cc]->AddInputConnection(input);
    this->CubeAxesRepresentations[cc]->AddInputConnection(input);
    }
  this->Superclass::AddInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  for(int cc=0; cc < 3; cc++)
    {
    this->SliceRepresentations[cc]->RemoveInputConnection(port, input);
    this->CubeAxesRepresentations[cc]->RemoveInputConnection(port, input);
    }
  this->Superclass::RemoveInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::RemoveInputConnection(int port, int index)
{
  for(int cc=0; cc < 3; cc++)
    {
    this->SliceRepresentations[cc]->RemoveInputConnection(port, index);
    this->CubeAxesRepresentations[cc]->RemoveInputConnection(port, index);
    }
  this->Superclass::RemoveInputConnection(port, index);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::MarkModified()
{
  for(int cc=0; cc < 3; cc++)
    {
    this->SliceRepresentations[cc]->MarkModified();
    this->CubeAxesRepresentations[cc]->MarkModified();
    }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
unsigned int vtkPVCompositeOrthographicSliceRepresentation::Initialize(
  unsigned int minIdAvailable, unsigned int maxIdAvailable)
{
  for(int cc=0; cc < 3; cc++)
    {
    minIdAvailable = this->SliceRepresentations[cc]->Initialize(minIdAvailable, maxIdAvailable);
    minIdAvailable = this->CubeAxesRepresentations[cc]->Initialize(minIdAvailable, maxIdAvailable);
    }
  return this->Superclass::Initialize(minIdAvailable, maxIdAvailable);
}

//----------------------------------------------------------------------------
bool vtkPVCompositeOrthographicSliceRepresentation::AddToView(vtkView* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  for (int cc=0; cc < 3; cc++)
    {
    view->AddRepresentation(this->SliceRepresentations[cc]);
    view->AddRepresentation(this->CubeAxesRepresentations[cc]);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVCompositeOrthographicSliceRepresentation::RemoveFromView(vtkView* view)
{
  if (!this->Superclass::RemoveFromView(view))
    {
    return false;
    }

  for (int cc=0; cc < 3; cc++)
    {
    view->RemoveRepresentation(this->SliceRepresentations[cc]);
    view->RemoveRepresentation(this->CubeAxesRepresentations[cc]);
    }
  return true;
}
//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
