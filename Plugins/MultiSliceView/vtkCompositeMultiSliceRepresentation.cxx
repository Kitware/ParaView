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
#include "vtkMultiSliceRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineRepresentation.h"
#include "vtkPVQuadRenderView.h"
#include "vtkSliceExtractorRepresentation.h"
#include "vtkStringArray.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkCompositeMultiSliceRepresentation);
//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::ModifiedInternalCallback(vtkObject*, unsigned long, void*)
{
  if(this && this->OutlineRepresentation && this->GetActiveRepresentation())
    {
    this->OutlineRepresentation->SetVisibility(
          this->GetActiveRepresentation()->GetVisibility() &&
          this->GetActiveRepresentation()->IsA("vtkMultiSliceRepresentation"));
    }
}
//----------------------------------------------------------------------------
vtkCompositeMultiSliceRepresentation::vtkCompositeMultiSliceRepresentation() : vtkPVCompositeRepresentation()
{
  this->OutlineRepresentation = vtkOutlineRepresentation::New();
  this->Slice1 = vtkSliceExtractorRepresentation::New();
  this->Slice2 = vtkSliceExtractorRepresentation::New();
  this->Slice3 = vtkSliceExtractorRepresentation::New();

  this->AddObserver(vtkCommand::ModifiedEvent, this, &vtkCompositeMultiSliceRepresentation::ModifiedInternalCallback);
}

//----------------------------------------------------------------------------
vtkCompositeMultiSliceRepresentation::~vtkCompositeMultiSliceRepresentation()
{
  this->OutlineRepresentation->Delete();
  this->OutlineRepresentation = NULL;
  this->Slice1->Delete();
  this->Slice1 = NULL;
  this->Slice2->Delete();
  this->Slice2 = NULL;
  this->Slice3->Delete();
  this->Slice3 = NULL;
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
  this->Slice1->SetVisibility(visible);
//  this->Slice2->SetVisibility(visible);
//  this->Slice3->SetVisibility(visible);
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

  vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(view);
  if(quadView)
    {
    cout << "Add slice to view" << endl;

    quadView->GetOrthoRenderView(vtkPVQuadRenderView::BOTTOM_LEFT)->AddRepresentation(this->Slice1);
//    quadView->GetOrthoRenderView(vtkPVQuadRenderView::TOP_LEFT)->AddRepresentation(this->Slice2);
//    quadView->GetOrthoRenderView(vtkPVQuadRenderView::TOP_RIGHT)->AddRepresentation(this->Slice3);
    }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkCompositeMultiSliceRepresentation::RemoveFromView(vtkView* view)
{
  view->RemoveRepresentation(this->OutlineRepresentation);

  vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(view);
  if(quadView)
    {
    cout << "Remove slice to view" << endl;

    quadView->GetOrthoRenderView(vtkPVQuadRenderView::BOTTOM_LEFT)->RemoveRepresentation(this->Slice1);
//    quadView->GetOrthoRenderView(vtkPVQuadRenderView::TOP_LEFT)->RemoveRepresentation(this->Slice2);
//    quadView->GetOrthoRenderView(vtkPVQuadRenderView::TOP_RIGHT)->RemoveRepresentation(this->Slice3);
    }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::MarkModified()
{
  this->OutlineRepresentation->MarkModified();
  this->Slice1->MarkModified();
  this->Slice2->MarkModified();
  this->Slice3->MarkModified();
  this->Superclass::MarkModified();
}
//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetUpdateTime(double time)
{
  this->OutlineRepresentation->SetUpdateTime(time);
  this->Slice1->SetUpdateTime(time);
  this->Slice2->SetUpdateTime(time);
  this->Slice3->SetUpdateTime(time);
  this->Superclass::SetUpdateTime(time);
}
//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetUseCache(bool val)
{
  this->OutlineRepresentation->SetUseCache(val);
  this->Slice1->SetUseCache(val);
  this->Slice2->SetUseCache(val);
  this->Slice3->SetUseCache(val);
  this->Superclass::SetUseCache(val);
}
//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetCacheKey(double val)
{
  this->OutlineRepresentation->SetCacheKey(val);
  this->Slice1->SetCacheKey(val);
  this->Slice2->SetCacheKey(val);
  this->Slice3->SetCacheKey(val);
  this->Superclass::SetCacheKey(val);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetForceUseCache(bool val)
{
  this->OutlineRepresentation->SetForceUseCache(val);
  this->Slice1->SetForceUseCache(val);
  this->Slice2->SetForceUseCache(val);
  this->Slice3->SetForceUseCache(val);
  this->Superclass::SetForceUseCache(val);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiSliceRepresentation::SetForcedCacheKey(double val)
{
  this->OutlineRepresentation->SetForcedCacheKey(val);
  this->Slice1->SetForcedCacheKey(val);
  this->Slice2->SetForcedCacheKey(val);
  this->Slice3->SetForcedCacheKey(val);
  this->Superclass::SetForcedCacheKey(val);
}
