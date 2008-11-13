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
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMSImageDataParallelStrategy.h"
#include "vtkSMStreamingSerialStrategy.h"
#include "vtkSMSUniformGridParallelStrategy.h"
#include "vtkSMSUnstructuredDataParallelStrategy.h"
#include "vtkSMSUnstructuredGridParallelStrategy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMStreamingViewProxy.h"
#include "vtkSMStreamingHelperProxy.h"
#include "vtkSMIntVectorProperty.h"

#include "vtkSMSourceProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkPVDataInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"

vtkCxxRevisionMacro(vtkSMStreamingRepresentation, "1.3");
vtkStandardNewMacro(vtkSMStreamingRepresentation);

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

  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, "Visibility", 0);
  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, "MakeOutlineOfInput", 1);
  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, "UseOutline", 1);
  
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

  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, "Visibility",
    visible && this->PieceBoundsVisibility);
  this->PieceBoundsRepresentation->UpdateVTKObjects();
     
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetPieceBoundsVisibility(int visible)
{
  this->PieceBoundsVisibility = visible;
  vtkSMStreamingRepresentationSetInt(this->PieceBoundsRepresentation, "Visibility",
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

/*
//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::SetPassNumber(int val, int force)
{
  if (this->PieceBoundsRepresentation && this->PieceBoundsVisibility)
    {
    //this->PieceBoundsRepresentation->SetPassNumber(val, force);
    }
}
*/

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
  int nPasses = vtkSMStreamingHelperProxy::GetHelper()->GetStreamedPasses();
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
      TrySetPassNumber(vtkSMSUnstructuredDataParallelStrategy);
      TrySetPassNumber(vtkSMSUnstructuredGridParallelStrategy);
      TrySetPassNumber(vtkSMSImageDataParallelStrategy);
      TrySetPassNumber(vtkSMSUniformGridParallelStrategy);
      }
    this->Modified();
    }
}

/*
//----------------------------------------------------------------------------
int vtkSMStreamingRepresentation::ComputePriorities()
{
  if (this->PieceBoundsRepresentation && this->PieceBoundsVisibility)
    {
    //Don't compute anything with the piecebounds rep. It's a waste of effort
    //and the piece ordering will end up different because geometry is not the
    //same. Instead, just copy the piece list from the surface representation that 
    //it is attached to.
    vtkSMRepresentationStrategyVector strats1;
    this->ActiveRepresentation->GetActiveStrategies(strats1);
    vtkSMRepresentationStrategy *strat1 = strats1[0];

    vtkSMRepresentationStrategyVector strats2;
    this->PieceBoundsRepresentation->GetActiveStrategies(strats2);
    vtkSMRepresentationStrategy *strat2 = strats2[0];
    
    strat1->SharePieceList(strat2);
    }
  return 0;
}
*/

#define TryComputePriorities(type) \
{ \
  type *ptr = type::SafeDownCast(iter->GetPointer());\
  if (ptr)\
    {\
    int maxpass = ptr->ComputePriorities();\
    if (maxpass > maxPass)\
      {\
      if (doPrints)\
        {\
        cerr << "SR(" << this << ") MaxPass=" << maxpass << endl;\
        }\
      maxPass = maxpass;\
      }\
    }\
}

//----------------------------------------------------------------------------
int vtkSMStreamingRepresentation::ComputePriorities()
{
  int doPrints = vtkSMStreamingHelperProxy::GetHelper()->GetEnableStreamMessages();
  if (doPrints)
    {
    cerr << "SR(" << this << ") ComputePriorities" << endl;
    }
  int maxPass = -1;
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryComputePriorities(vtkSMStreamingSerialStrategy);
    TryComputePriorities(vtkSMSUnstructuredDataParallelStrategy);
    TryComputePriorities(vtkSMSUnstructuredGridParallelStrategy);
    TryComputePriorities(vtkSMSImageDataParallelStrategy);
    TryComputePriorities(vtkSMSUniformGridParallelStrategy);
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

/*
//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::ClearStreamCache()
{
  if (this->PieceBoundsRepresentation && this->PieceBoundsVisibility)
    {
    //this->PieceBoundsRepresentation->ClearStreamCache();
    }
}
*/

//----------------------------------------------------------------------------
void vtkSMStreamingRepresentation::ClearStreamCache()
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMStreamingSerialStrategy, ClearStreamCache());
    TryMethod(vtkSMSUnstructuredDataParallelStrategy, ClearStreamCache());
    TryMethod(vtkSMSUnstructuredGridParallelStrategy, ClearStreamCache());
    TryMethod(vtkSMSImageDataParallelStrategy, ClearStreamCache());
    TryMethod(vtkSMSUniformGridParallelStrategy, ClearStreamCache());
    }
}

/*
//----------------------------------------------------------------------------
bool vtkSMStreamingRepresentation::AddToView(vtkSMViewProxy* view)
{
  this->PieceBoundsRepresentation->AddToView(view);

  return this->Superclass::AddToView(view);
}
*/

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
    TryMethod(vtkSMSUnstructuredDataParallelStrategy, SetViewState(camera, frustum));
    TryMethod(vtkSMSUnstructuredGridParallelStrategy, SetViewState(camera, frustum));
    TryMethod(vtkSMSImageDataParallelStrategy, SetViewState(camera, frustum));
    TryMethod(vtkSMSUniformGridParallelStrategy, SetViewState(camera, frustum));
    }
}
