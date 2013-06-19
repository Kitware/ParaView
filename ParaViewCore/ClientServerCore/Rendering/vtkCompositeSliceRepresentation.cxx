/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeSliceRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeSliceRepresentation.h"

#include "vtkAlgorithm.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineRepresentation.h"
#include "vtkPVMultiSliceView.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSliceFriendGeometryRepresentation.h"
#include "vtkStringArray.h"
#include "vtkThreeSliceFilter.h"
#include "vtkView.h"

#include <assert.h>

vtkStandardNewMacro(vtkCompositeSliceRepresentation);
//----------------------------------------------------------------------------
vtkCompositeSliceRepresentation::vtkCompositeSliceRepresentation() : vtkPVCompositeRepresentation()
{
  this->OutlineVisibility = true;
  this->ViewObserverId = 0;
  this->InternalSliceFilter = vtkThreeSliceFilter::New();
  this->OutlineRepresentation = vtkOutlineRepresentation::New();
  for(int i=0; i < 4; ++i)
    {
    this->Slices[i] = NULL;
    }

  this->AddObserver(vtkCommand::ModifiedEvent, this, &vtkCompositeSliceRepresentation::ModifiedInternalCallback);
}
//----------------------------------------------------------------------------
vtkCompositeSliceRepresentation::~vtkCompositeSliceRepresentation()
{
  this->OutlineRepresentation->Delete();
  this->OutlineRepresentation = NULL;
  for(int i=0; i < 4; ++i)
    {
    if(this->Slices[i] == NULL)
      {
      continue;
      }
    this->Slices[i]->Delete();
    this->Slices[i] = NULL;
    }
  this->InternalSliceFilter->Delete();
  this->InternalSliceFilter = NULL;
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  this->OutlineRepresentation->SetVisibility(visible && this->OutlineVisibility);

  // Update outline visibility
  this->ModifiedInternalCallback(NULL, 0, NULL);

  // Need to skip the main one that is already register inside our base class
  for(int i=1; i < 4; ++i)
    {
    if(this->Slices[i] == NULL)
      {
      continue;
      }
    this->Slices[i]->SetVisibility(visible);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::SetInputConnection(port, input);
  if (port == 0)
    {
    this->OutlineRepresentation->SetInputConnection(port, input);
    this->InternalSliceFilter->SetInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  // port is assumed to be 0.
  this->OutlineRepresentation->SetInputConnection(input);
  this->InternalSliceFilter->SetInputConnection(input);
  this->Superclass::SetInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::AddInputConnection(
    int port, vtkAlgorithmOutput* input)
{
  this->Superclass::AddInputConnection(port, input);
  if (port == 0)
    {
    this->OutlineRepresentation->AddInputConnection(port, input);
    this->InternalSliceFilter->SetInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  // port is assumed to be 0.
  this->OutlineRepresentation->AddInputConnection(input);
  this->InternalSliceFilter->AddInputConnection(input);
  this->Superclass::AddInputConnection(input);
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::RemoveInputConnection(port, input);
  if (port == 0)
    {
    this->OutlineRepresentation->RemoveInputConnection(0, input);
    this->InternalSliceFilter->RemoveInputConnection(0, input);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::RemoveInputConnection(int port, int index)
{
  this->Superclass::RemoveInputConnection(port, index);
  if (port == 0)
    {
    this->OutlineRepresentation->RemoveInputConnection(0, index);
    this->InternalSliceFilter->RemoveInputConnection(0, index);
    }
}

//----------------------------------------------------------------------------
bool vtkCompositeSliceRepresentation::AddToView(vtkView* view)
{
  view->AddRepresentation(this->OutlineRepresentation);

  // Add observer to the view to Synch up view slice info with us
  assert("A representation CAN NOT be added to 2 views" &&
         (this->ViewObserverId == 0));
  vtkPVMultiSliceView* sliceView = vtkPVMultiSliceView::SafeDownCast(view);
  if(sliceView)
    {
    this->ViewObserverId = sliceView->AddObserver(
          vtkCommand::ConfigureEvent, this,
          &vtkCompositeSliceRepresentation::UpdateSliceConfigurationCallBack);
    this->UpdateSliceConfigurationCallBack(sliceView, 0, NULL);
    this->ViewObserverId = sliceView->AddObserver(
          vtkCommand::ModifiedEvent, this,
          &vtkCompositeSliceRepresentation::UpdateFromViewConfigurationCallBack);
    }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkCompositeSliceRepresentation::RemoveFromView(vtkView* view)
{
  view->RemoveRepresentation(this->OutlineRepresentation);

  // Remove observer to the view to Synch up view slice info with us
  vtkPVMultiSliceView* sliceView = vtkPVMultiSliceView::SafeDownCast(view);
  if(sliceView && this->ViewObserverId)
    {
    sliceView->RemoveObserver(this->ViewObserverId);
    this->ViewObserverId = 0;
    }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::MarkModified()
{
  this->OutlineRepresentation->MarkModified();

  // Need to skip the main one that is already register inside our base class
  for(int i = 1; i < 4; ++i)
    {
    if(this->Slices[i] == NULL)
      {
      continue;
      }
    this->Slices[i]->MarkModified();
    }
  this->Superclass::MarkModified();
}
//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetUpdateTime(double time)
{
  this->OutlineRepresentation->SetUpdateTime(time);

  // Need to skip the main one that is already register inside our base class
  for(int i = 1; i < 4; ++i)
    {
    if(this->Slices[i] == NULL)
      {
      continue;
      }
    this->Slices[i]->SetUpdateTime(time);
    }
  this->Superclass::SetUpdateTime(time);
}
//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetUseCache(bool val)
{
  this->OutlineRepresentation->SetUseCache(val);

  // Need to skip the main one that is already register inside our base class
  for(int i = 1; i < 4; ++i)
    {
    if(this->Slices[i] == NULL)
      {
      continue;
      }
    this->Slices[i]->SetUseCache(val);
    }
  this->Superclass::SetUseCache(val);
}
//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetCacheKey(double val)
{
  this->OutlineRepresentation->SetCacheKey(val);

  // Need to skip the main one that is already register inside our base class
  for(int i = 1; i < 4; ++i)
    {
    if(this->Slices[i] == NULL)
      {
      continue;
      }
    this->Slices[i]->SetCacheKey(val);
    }
  this->Superclass::SetCacheKey(val);
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetForceUseCache(bool val)
{
  this->OutlineRepresentation->SetForceUseCache(val);

  // Need to skip the main one that is already register inside our base class
  for(int i = 1; i < 4; ++i)
    {
    if(this->Slices[i] == NULL)
      {
      continue;
      }
    this->Slices[i]->SetForceUseCache(val);
    }
  this->Superclass::SetForceUseCache(val);
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetForcedCacheKey(double val)
{
  this->OutlineRepresentation->SetForcedCacheKey(val);

  // Need to skip the main one that is already register inside our base class
  for(int i = 1; i < 4; ++i)
    {
    if(this->Slices[i] == NULL)
      {
      continue;
      }
    this->Slices[i]->SetForcedCacheKey(val);
    }
  this->Superclass::SetForcedCacheKey(val);
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::AddRepresentation(const char *key, vtkPVDataRepresentation *repr)
{
  // Figure out if our representation should use the filter (We have the real data)
  // Or if we should rely on the delivery mechanism for client/server mode
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();
  vtkPVSession* session = vtkPVSession::SafeDownCast(processModule->GetActiveSession());
  bool allowDynaInput = !(session->GetProcessRoles() & vtkPVSession::SERVERS);

  // Retreive internal representation that could have been created by a proxy
  bool addToParent = true;
  int index = -1;
  vtkSliceFriendGeometryRepresentation* sliceRep = vtkSliceFriendGeometryRepresentation::SafeDownCast(repr);
  if(key && sliceRep)
    {
    if(!strcmp("Slices", key))
      {
      index = 0;
      }
    else if(!strcmp("Slice1", key))
      {
      index = 1;
      addToParent = false;
      }
    else if(!strcmp("Slice2", key))
      {
      index = 2;
      addToParent = false;
      }
    else if(!strcmp("Slice3", key))
      {
      index = 3;
      addToParent = false;
      }
    }

  if(index != -1)
    {
    this->Slices[index] = sliceRep;
    this->Slices[index]->SetInputConnection(this->InternalSliceFilter->GetOutputPort(index));
    this->Slices[index]->SetAllowInputConnectionSetting(allowDynaInput); // Make slices immutable input wise
    this->Slices[index]->Register(this);
    this->Slices[index]->InitializeMapperForSliceSelection();
    this->Slices[index]->SetRepresentationForRenderedDataObject(this);
    }

  if(addToParent)
    {
    this->Superclass::AddRepresentation(key, repr);
    }
}
//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::ModifiedInternalCallback(vtkObject*, unsigned long, void*)
{
  if(this && this->OutlineRepresentation && this->GetActiveRepresentation() && this->Slices[0])
    {
    this->OutlineRepresentation->SetVisibility( this->OutlineVisibility &&
          this->GetActiveRepresentation()->GetVisibility() &&
          this->GetActiveRepresentation() == this->Slices[0]);
    }
}
//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::UpdateSliceConfigurationCallBack(vtkObject* view, unsigned long, void*)
{
  vtkPVMultiSliceView* sliceView = vtkPVMultiSliceView::SafeDownCast(view);
  if(sliceView)
    {
    for(int i=0; i < 3; i++)
      {
      int nbSlices = sliceView->GetNumberOfSlice(i);
      const double* values = sliceView->GetSlice(i);
      this->InternalSliceFilter->SetNumberOfSlice(i, nbSlices);
      for(int index=0; index < nbSlices; ++index)
        {
        this->InternalSliceFilter->SetCutValue(i,index,values[index]);
        }
      this->InternalSliceFilter->SetCutNormal(i, sliceView->GetSliceNormal(i));
      }
    // We just take the origin of X
    this->InternalSliceFilter->SetCutOrigins(sliceView->GetSliceOrigin(0));
    }

  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::UpdateFromViewConfigurationCallBack(vtkObject* view, unsigned long, void*)
{
  vtkPVMultiSliceView* sliceView = vtkPVMultiSliceView::SafeDownCast(view);
  if(sliceView)
    {
    this->SetOutlineVisibility(sliceView->GetShowOutline() != 0);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeSliceRepresentation::GetRenderedDataObject(int vtkNotUsed(port))
{
  return this->InternalSliceFilter->GetInputDataObject(0,0);
}

//----------------------------------------------------------------------------
unsigned int vtkCompositeSliceRepresentation::Initialize(unsigned int minIdAvailable,
                                                         unsigned int maxIdAvailable)
{
  unsigned int newMin =
      this->OutlineRepresentation->Initialize(minIdAvailable, maxIdAvailable);

  return  this->Superclass::Initialize(newMin, maxIdAvailable);
}
//----------------------------------------------------------------------------
void vtkCompositeSliceRepresentation::SetOutlineVisibility(bool visible)
{
  this->OutlineVisibility = visible;
  this->SetVisibility(this->GetVisibility());
}
