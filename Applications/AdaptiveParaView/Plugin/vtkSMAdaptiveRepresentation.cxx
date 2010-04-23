/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAdaptiveRepresentation.h"
#include "vtkObjectFactory.h"

#include "vtkSMIntVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMAdaptiveOutlineRepresentation.h"
#include "vtkSMAdaptiveSerialStrategy.h"
#include "vtkSMAdaptiveViewProxy.h"
#include "vtkAdaptiveOptions.h"

vtkStandardNewMacro(vtkSMAdaptiveRepresentation);

#define DEBUGPRINT_REPRESENTATION(arg)\
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

inline void vtkSMAdaptiveRepresentationSetInt(
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
vtkSMAdaptiveRepresentation::vtkSMAdaptiveRepresentation()
{
  this->PieceBoundsRepresentation = NULL;
  this->PieceBoundsVisibility = 0;
  this->WendDone = 1;
  this->AllDone = 1;
  this->Locked = 0;
}

//----------------------------------------------------------------------------
vtkSMAdaptiveRepresentation::~vtkSMAdaptiveRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::SetViewInformation(vtkInformation* info)
{
  this->Superclass::SetViewInformation(info);

  if (this->PieceBoundsRepresentation)
    {
    this->PieceBoundsRepresentation->SetViewInformation(info);
    }
}

//----------------------------------------------------------------------------
bool vtkSMAdaptiveRepresentation::EndCreateVTKObjects()
{
  this->PieceBoundsRepresentation = 
    vtkSMAdaptiveOutlineRepresentation::SafeDownCast(
      this->GetSubProxy("PieceBoundsRepresentation"));

  vtkSMProxy* inputProxy = this->GetInputProxy();

  this->Connect(inputProxy, this->PieceBoundsRepresentation,
                "Input", this->OutputPort);

  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMAdaptiveRepresentation::RemoveFromView(vtkSMViewProxy* view)
{
  this->PieceBoundsRepresentation->RemoveMyselfFromView(view);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::SetVisibility(int visible)
{
  vtkSMAdaptiveRepresentationSetInt(this->PieceBoundsRepresentation, 
                                     "Visibility",
                                     visible && this->PieceBoundsVisibility);
  this->PieceBoundsRepresentation->UpdateVTKObjects();
     
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::Update(vtkSMViewProxy* view)
{
  this->PieceBoundsRepresentation->Update(view);

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMAdaptiveRepresentation::UpdateRequired()
{
  if (this->PieceBoundsRepresentation->UpdateRequired())
    {
    return true;
    }
  return this->Superclass::UpdateRequired();
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::SetUpdateTime(double time)
{
  this->Superclass::SetUpdateTime(time);
  this->PieceBoundsRepresentation->SetUpdateTime(time);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::SetUseViewUpdateTime(bool use)
{
  this->Superclass::SetUseViewUpdateTime(use);
  this->PieceBoundsRepresentation->SetUseViewUpdateTime(use);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);
  this->PieceBoundsRepresentation->SetViewUpdateTime(time);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
#define TryMethod(type,method)                        \
{ \
  type *ptr = type::SafeDownCast(iter->GetPointer());\
  if (ptr)\
    {\
    ptr->method;\
    }\
}

//---------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::SetViewState(double *camera, double *frustum)
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMAdaptiveSerialStrategy, SetViewState(camera, frustum));
    }
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::ClearStreamCache()
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMAdaptiveSerialStrategy, ClearStreamCache());
    }
}

//----------------------------------------------------------------------------
bool vtkSMAdaptiveRepresentation::AddToView(vtkSMViewProxy* view)
{
  vtkSMAdaptiveViewProxy* streamView = vtkSMAdaptiveViewProxy::SafeDownCast(view);
  if (!streamView)
    {
    vtkErrorMacro("View must be a vtkSMAdaptiveView.");
    return false;
    }

  //this tells renderview to let view create strategy
  vtkSMRenderViewProxy* renderView = streamView->GetRootView();
  renderView->SetNewStrategyHelper(view);

  //but still add this to itself
  bool ret = this->Superclass::AddToView(renderView);

  //view->AddRepresentation(this->PieceBoundsRepresentation);
  this->PieceBoundsRepresentation->AddToView(view);

  return ret;
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::PrepareFirstPass()
{
  DEBUGPRINT_REPRESENTATION(
    cerr << "SREP("<<this<<")::PrepareFirstPass" << endl;
  );
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMAdaptiveSerialStrategy, PrepareFirstPass());
    }
  this->WendDone = 0;  
  this->AllDone = 0;
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::SetPieceBoundsVisibility(int visible)
{
  this->PieceBoundsVisibility = visible;
  vtkSMAdaptiveRepresentationSetInt(this->PieceBoundsRepresentation, 
                                     "Visibility",
                                     visible && this->GetVisibility());
  if (visible)
    {
    //Unfortunatelty we have to clear both parties caches which adds momentary overhead.
    //
    //Otherwise, pbr may be told to display something that comes from the append slot,
    //which the pbr has not filled yet because the pbr was invisible.
    //
    //The alternative, to keep the cache synchronized even when the pbr is 
    //invisible adds overhead to the general case.
    this->PieceBoundsRepresentation->EmptyCache();
    vtkSMRepresentationStrategyVector strats;
    this->GetActiveStrategies(strats);
    vtkSMRepresentationStrategyVector::iterator iter; 
    for (iter = strats.begin(); iter != strats.end(); ++iter)
      {
      TryMethod(vtkSMAdaptiveSerialStrategy, EmptyCache());
      }
    }
  this->PieceBoundsRepresentation->UpdateVTKObjects();
}


//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::PrepareAnotherPass()
{
  DEBUGPRINT_REPRESENTATION(
    cerr << "SREP("<<this<<")::PrepareAnotherPass" << endl;
  );
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMAdaptiveSerialStrategy, PrepareAnotherPass());
    }
}

//---------------------------------------------------------------------------
#define TryChooseNextPiece(type) \
{ \
  type *ptr = type::SafeDownCast(iter->GetPointer());\
  if (ptr)\
    {\
    ptr->ChooseNextPiece();\
    ptr->GetPieceInfo(&P, &NP, &R, &PRIORITY, &HIT, &APPEND);   \
    ptr->GetStateInfo(&ALLDONE, &WENDDONE);\
    }\
  }

//---------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::ChooseNextPiece()
{
  DEBUGPRINT_REPRESENTATION(
  cerr << "SREP("<<this<<")::ChooseNextPiece" << endl;
  );
  int P, NP;
  double R, PRIORITY;
  bool HIT;
  bool APPEND;

  bool ALLDONE;
  bool WENDDONE;

  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryChooseNextPiece(vtkSMAdaptiveSerialStrategy);
    }
  DEBUGPRINT_REPRESENTATION(
  cerr << "STRAT(" << this << ") CHOSE " << P << "/" << NP << "@" << R << endl;
  );

  this->PieceBoundsRepresentation->SetNextPiece(P, NP, R, PRIORITY, HIT, APPEND);
  this->AllDone = ALLDONE;
  this->WendDone = WENDDONE;
}

//---------------------------------------------------------------------------
#define TryFinishPass(type) \
{ \
  type *ptr = type::SafeDownCast(iter->GetPointer());\
  if (ptr)\
    {\
    if (!this->Locked) \
      { \
      ptr->FinishPass();\
      } \
    ptr->GetStateInfo(&ALLDONE, &WENDDONE);\
    }\
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::FinishPass()
{
  bool ALLDONE;
  bool WENDDONE;
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryFinishPass(vtkSMAdaptiveSerialStrategy);
    }
  this->AllDone = ALLDONE;
  this->WendDone = WENDDONE;

  if (this->Locked)
    {
    //cerr << "SREP(" << this << ")::REP IS LOCKED" << endl;
    this->AllDone = this->WendDone;
    }
}

//---------------------------------------------------------------------------
#define TryRefine(type) \
{ \
  type *ptr = type::SafeDownCast(iter->GetPointer());\
  if (ptr)\
    {\
    if (!this->Locked) \
      { \
      ptr->Refine();\
      } \
    ptr->GetStateInfo(&ALLDONE, &WENDDONE);\
    }\
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::Refine()
{
  bool ALLDONE;
  bool WENDDONE;
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryRefine(vtkSMAdaptiveSerialStrategy);
    }
  this->AllDone = ALLDONE;
  this->WendDone = WENDDONE;

  if (this->Locked)
    {
    //cerr << "SREP(" << this << ")::REP IS LOCKED" << endl;
    this->AllDone = this->WendDone;
    }
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::SetMaxDepth(int maxD)
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMAdaptiveSerialStrategy, SetMaxDepth(maxD));
    }
}


//----------------------------------------------------------------------------
int vtkSMAdaptiveRepresentation::GetMaxDepth()
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  int maxD = -1;
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMAdaptiveSerialStrategy, GetMaxDepth(&maxD));
    }
  return maxD;
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveRepresentation::Coarsen()
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    TryMethod(vtkSMAdaptiveSerialStrategy, Coarsen());
    }
}
