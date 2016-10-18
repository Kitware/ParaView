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
void vtkPVCompositeOrthographicSliceRepresentation::SetVisibility(bool visible)
{
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceRepresentations[cc]->SetVisibility(visible);
  }
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::SetInputConnection(
  int port, vtkAlgorithmOutput* input)
{
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceRepresentations[cc]->SetInputConnection(port, input);
  }
  this->Superclass::SetInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceRepresentations[cc]->SetInputConnection(input);
  }
  this->Superclass::SetInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::AddInputConnection(
  int port, vtkAlgorithmOutput* input)
{
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceRepresentations[cc]->AddInputConnection(port, input);
  }
  this->Superclass::AddInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceRepresentations[cc]->AddInputConnection(input);
  }
  this->Superclass::AddInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::RemoveInputConnection(
  int port, vtkAlgorithmOutput* input)
{
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceRepresentations[cc]->RemoveInputConnection(port, input);
  }
  this->Superclass::RemoveInputConnection(port, input);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::RemoveInputConnection(int port, int index)
{
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceRepresentations[cc]->RemoveInputConnection(port, index);
  }
  this->Superclass::RemoveInputConnection(port, index);
}

//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::MarkModified()
{
  for (int cc = 0; cc < 3; cc++)
  {
    this->SliceRepresentations[cc]->MarkModified();
  }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
unsigned int vtkPVCompositeOrthographicSliceRepresentation::Initialize(
  unsigned int minIdAvailable, unsigned int maxIdAvailable)
{
  for (int cc = 0; cc < 3; cc++)
  {
    minIdAvailable = this->SliceRepresentations[cc]->Initialize(minIdAvailable, maxIdAvailable);
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

  for (int cc = 0; cc < 3; cc++)
  {
    view->AddRepresentation(this->SliceRepresentations[cc]);
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

  for (int cc = 0; cc < 3; cc++)
  {
    view->RemoveRepresentation(this->SliceRepresentations[cc]);
  }
  return true;
}
//----------------------------------------------------------------------------
void vtkPVCompositeOrthographicSliceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
