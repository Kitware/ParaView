/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveOutlineRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAdaptiveOutlineRepresentation.h"
#include "vtkObjectFactory.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMAdaptiveSerialStrategy.h"
#include "vtkSMAdaptiveViewProxy.h"
#include "vtkAdaptiveOptions.h"

vtkCxxRevisionMacro(vtkSMAdaptiveOutlineRepresentation, "1.1");
vtkStandardNewMacro(vtkSMAdaptiveOutlineRepresentation);

#define DEBUGPRINT_REPRESENTATION(arg)\
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

//------------------------------------------------------------------------------
void vtkSMAdaptiveOutlineRepresentation::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
vtkSMAdaptiveOutlineRepresentation::vtkSMAdaptiveOutlineRepresentation()
{
  this->Piece = 0;
  this->Pieces = 1;
  this->Resolution = 0.0;
}

//----------------------------------------------------------------------------
vtkSMAdaptiveOutlineRepresentation::~vtkSMAdaptiveOutlineRepresentation()
{
}

//----------------------------------------------------------------------------
bool vtkSMAdaptiveOutlineRepresentation::EndCreateVTKObjects()
{
  bool ret = this->Superclass::EndCreateVTKObjects();

  vtkSMIntVectorProperty *ivp;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  ivp->SetElement(0,0);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("MakeOutlineOfInput"));
  ivp->SetElement(0,1);

  vtkSMDoubleVectorProperty *dvp;  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("LineWidth"));
  dvp->SetElement(0,3);    

  return ret;
}

//----------------------------------------------------------------------------
bool vtkSMAdaptiveOutlineRepresentation::AddToView(vtkSMViewProxy* view)
{
  vtkSMAdaptiveViewProxy* streamView = 
    vtkSMAdaptiveViewProxy::SafeDownCast(view);
  if (!streamView)
    {
    vtkErrorMacro("View must be a vtkSMAdaptiveView.");
    return false;
    }

  //this tells renderview to let view create strategy
  vtkSMRenderViewProxy* renderView = streamView->GetRootView();
  renderView->SetNewStrategyHelper(view);

  this->SetNextPiece(0,1, 0.0, 1.0, false, false);

  //but still add this to itself
  return this->Superclass::AddToView(renderView);
}

//-----------------------------------------------------------------------------
void vtkSMAdaptiveOutlineRepresentation::SetNextPiece(
  int P, int NP, double R, double pri, bool hit, bool append)
{
  DEBUGPRINT_REPRESENTATION(
  cerr << "OREP TOLD TO DO " << P << "/" << NP << "@" << R << endl;
                            );
  if (P == this->Piece && NP == this->Pieces && R != this->Resolution)
    {
    return;
    }
  this->Modified();
  this->Piece = P;
  this->Pieces = NP;
  this->Resolution = R;
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    vtkSMAdaptiveSerialStrategy *strat = 
      vtkSMAdaptiveSerialStrategy::SafeDownCast(iter->GetPointer());
    strat->ClearStreamCache();
    strat->SetNextPiece(P,NP,R,pri,hit,append);
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("AmbientColor"));
  if (!dvp)
    {
    return;
    }
  //greylevel to show resolution
  dvp->SetElement(0, 1.0*R);
  dvp->SetElement(1, 1.0*R);
  dvp->SetElement(2, 1.0*R);
  //but yellow to make full res pieces unmistakable
  if (R >= 1.0)
    {
    dvp->SetElement(0, 1.0);
    dvp->SetElement(1, 1.0);
    dvp->SetElement(2, 0.0);
    }

  //if piece was cached, show it as red instead
  if (hit)
    {
    dvp->SetElement(0, 1.0);
    dvp->SetElement(1, 0.0);
    dvp->SetElement(2, 0.0);
    }

  //if the piece command came from the append slot show it as green
  if (append)
    {
    dvp->SetElement(0, 0.0);
    dvp->SetElement(1, 1.0);
    dvp->SetElement(2, 0.0);
    }

  this->UpdateProperty("AmbientColor");
}

//------------------------------------------------------------------------------
bool vtkSMAdaptiveOutlineRepresentation::RemoveMyselfFromView(
  vtkSMViewProxy* view)
{
  return this->RemoveFromView(view);
}

//------------------------------------------------------------------------------
bool vtkSMAdaptiveOutlineRepresentation::EmptyCache()
{
  vtkSMRepresentationStrategyVector strats;
  this->GetActiveStrategies(strats);
  vtkSMRepresentationStrategyVector::iterator iter; 
  for (iter = strats.begin(); iter != strats.end(); ++iter)
    {
    vtkSMAdaptiveSerialStrategy *strat = 
      vtkSMAdaptiveSerialStrategy::SafeDownCast(iter->GetPointer());
    strat->EmptyCache();
    strat->SetNextPiece(0,1,0.0,1.0,false,false);
    }
  return true;
}
