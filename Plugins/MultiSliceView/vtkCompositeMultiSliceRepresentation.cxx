/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeMultiSliceRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeMultiSliceRepresentation.h"

#include "vtkAlgorithm.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineRepresentation.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkCompositeMultiSliceRepresentation);
//----------------------------------------------------------------------------
vtkCompositeMultiSliceRepresentation::vtkCompositeMultiSliceRepresentation() : vtkPVCompositeRepresentation()
{
  this->OutlineRepresentation = vtkOutlineRepresentation::New();
}

//----------------------------------------------------------------------------
vtkCompositeMultiSliceRepresentation::~vtkCompositeMultiSliceRepresentation()
{
  this->OutlineRepresentation->Delete();
  this->OutlineRepresentation = NULL;
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  this->OutlineRepresentation->SetVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::SetInputConnection(port, input);
  if (port == 0)
    {
    this->OutlineRepresentation->SetInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  // port is assumed to be 0.
  this->OutlineRepresentation->SetInputConnection(input);
  this->Superclass::SetInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::AddInputConnection(
  int port, vtkAlgorithmOutput* input)
{
  this->Superclass::AddInputConnection(port, input);
 if (port == 0)
    {
    this->OutlineRepresentation->AddInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  // port is assumed to be 0.
  this->OutlineRepresentation->AddInputConnection(input);
  this->Superclass::AddInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::RemoveInputConnection(port, input);
  if (port == 0)
    {
    this->OutlineRepresentation->RemoveInputConnection(0, input);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::RemoveInputConnection(int port, int index)
{
  this->Superclass::RemoveInputConnection(port, index);
  if (port == 0)
    {
    this->OutlineRepresentation->RemoveInputConnection(0, index);
    }
}

//----------------------------------------------------------------------------
bool vtkCompositeMultiSliceRepresentation::AddToView(vtkView* view)
{
  view->AddRepresentation(this->OutlineRepresentation);
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkCompositeMultiSliceRepresentation::RemoveFromView(vtkView* view)
{
  view->RemoveRepresentation(this->OutlineRepresentation);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::MarkModified()
{
  this->OutlineRepresentation->MarkModified();
  this->Superclass::MarkModified();
}
//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetUpdateTime(double time)
{
  this->OutlineRepresentation->SetUpdateTime(time);
  this->Superclass::SetUpdateTime(time);
}
//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetUseCache(bool val)
{
  this->OutlineRepresentation->SetUseCache(val);
  this->Superclass::SetUseCache(val);
}
//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetCacheKey(double val)
{
  this->OutlineRepresentation->SetCacheKey(val);
  this->Superclass::SetCacheKey(val);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetForceUseCache(bool val)
{
  this->OutlineRepresentation->SetForceUseCache(val);
  this->Superclass::SetForceUseCache(val);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetForcedCacheKey(double val)
{
  this->OutlineRepresentation->SetForcedCacheKey(val);
  this->Superclass::SetForcedCacheKey(val);
}
