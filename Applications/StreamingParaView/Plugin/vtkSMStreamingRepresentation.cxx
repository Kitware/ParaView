/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStreamingRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStreamingSerialStrategy.h"
#include "vtkSMStreamingViewProxy.h"
#include "vtkSMStreamingParallelStrategy.h"
#include "vtkStreamingOptions.h"

vtkStandardNewMacro(vtkSMStreamingRepresentation);

#define DEBUGPRINT_REPRESENTATION(arg)\
  if (vtkStreamingOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

inline void vtkSMStreamingRepresentationSetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    proxy->UpdateProperty(pname);
    }
}

//----------------------------------------------------------------------------
vtkSMStreamingRepresentation::vtkSMStreamingRepresentation()
{
  this->PieceBoundsRepresentation = 0;
  this->PieceBoundsVisibility = 0;
}

//----------------------------------------------------------------------------
vtkSMStreamingRepresentation::~vtkSMStreamingRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetViewInformation(vtkInformation* info)
{
  this->Superclass::SetViewInformation(info);

  if (this->PieceBoundsRepresentation)
    {
    this->PieceBoundsRepresentation->SetViewInformation(info);
    }
}

//----------------------------------------------------------------------------
bool vtkSMStreamingRepresentation::EndCreateVTKObjects()
{
  this->PieceBoundsRepresentation = 
    vtkSMDataRepresentationProxy::SafeDownCast(
      this->GetSubProxy("PieceBoundsRepresentation"));

  vtkSMProxy* inputProxy = this->GetInputProxy();

  this->Connect(inputProxy, this->PieceBoundsRepresentation,
                "Input", this->OutputPort);

  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, 
                                     "Visibility", 0);
  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, 
                                     "MakeOutlineOfInput", 1);
  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, 
                                     "UseOutline", 1);
  
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMStreamingRepresentation::RemoveFromView(vtkSMViewProxy* view)
{
  this->PieceBoundsRepresentation->RemoveFromView(view);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetVisibility(int visible)
{
  if (!visible)
    {
    this->ClearStreamCache();
    }

  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, 
                                     "Visibility",
                                     visible && this->PieceBoundsVisibility);
  this->PieceBoundsRepresentation->UpdateVTKObjects();
     
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetPieceBoundsVisibility(int visible)
{
  this->PieceBoundsVisibility = visible;
  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, 
                                     "Visibility",
                                     visible && this->GetVisibility());
  this->PieceBoundsRepresentation->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::Update(vtkSMViewProxy* view)
{
  this->PieceBoundsRepresentation->Update(view);
  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMStreamingRepresentation::UpdateRequired()
{
  if (this->PieceBoundsRepresentation->UpdateRequired())
    {
    return true;
    }
  return this->Superclass::UpdateRequired();
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetUpdateTime(double time)
{
  this->Superclass::SetUpdateTime(time);
  this->PieceBoundsRepresentation->SetUpdateTime(time);
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetUseViewUpdateTime(bool use)
{
  this->Superclass::SetUseViewUpdateTime(use);
  this->PieceBoundsRepresentation->SetUseViewUpdateTime(use);
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);
  this->PieceBoundsRepresentation->SetViewUpdateTime(time);
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

#define TrySetPassNumber(type) \
{ \
  type *ptr = type::SafeDownCast(iter->GetPointer());\
  if (ptr)\
    {\
    ptr->SetPassNumber(val, force);\
    }\
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetPassNumber(int val, int force)
{
  int nPasses = vtkStreamingOptions::GetStreamedPasses();
  if (val<nPasses //should always be less than
      && 
      val>=0)
    {
    //pass the render pass number down to my strategies
    vtkSMRepresentationStrategyVector strats;
    this->GetActiveStrategies(strats);
    vtkSMRepresentationStrategyVector::iterator iter;
    for (iter = strats.begin(); iter != strats.end(); ++iter)
      {
      //multiple inheritance would be nice about now
      TrySetPassNumber(vtkSMStreamingSerialStrategy);
      TrySetPassNumber(vtkSMStreamingParallelStrategy);
      }
    this->Modified();
    //this->PieceBoundsRepresentation->SetPassNumber(val, 1);
    }
}

#define TryComputePriorities(type) \
{ \
  type *ptr = type::SafeDownCast(iter->GetPointer());\
  if (ptr)\
    {\
    int maxpass = ptr->ComputePriorities();\
    if (maxpass > maxPass)\
      {\
      maxPass = maxpass;\
      }\
    }\
}

//----------------------------------------------------------------------------
int vtkSMStreamingRepresentation::ComputePriorities()
{
  DEBUGPRINT_REPRESENTATION(
  cerr << "SR(" << this << ") ComputePriorities" << endl;
                            );
  int maxPass = -1;
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryComputePriorities(vtkSMStreamingSerialStrategy);
    TryComputePriorities(vtkSMStreamingParallelStrategy);
    }
  return maxPass;
}

#define TryMethod(type,method)                        \
{ \
  type *ptr = type::SafeDownCast(iter->GetPointer());\
  if (ptr)\
    {\
    ptr->method;\
    }\
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::ClearStreamCache()
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMStreamingSerialStrategy, ClearStreamCache());
    TryMethod(vtkSMStreamingParallelStrategy, ClearStreamCache());
    }
}

//----------------------------------------------------------------------------
bool vtkSMStreamingRepresentation::AddToView(vtkSMViewProxy* view)
{
  vtkSMStreamingViewProxy* streamView = vtkSMStreamingViewProxy::SafeDownCast(view);
  if (!streamView)
    {
    vtkErrorMacro("View must be a vtkSMStreamingView.");
    return false;
    }


  //this tells renderview to let view create strategy
  vtkSMRenderViewProxy* renderView = streamView->GetRootView();
  renderView->SetNewStrategyHelper(view);

  //Disabled for now, I need to be able to tell it what piece is current.
  //this->PieceBoundsRepresentation->AddToView(renderView);

  //but still add this to renderView
  return this->Superclass::AddToView(renderView);
}

//---------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetViewState(double *camera, double *frustum)
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMStreamingSerialStrategy, SetViewState(camera, frustum));
    TryMethod(vtkSMStreamingParallelStrategy, SetViewState(camera, frustum));
    }
}
