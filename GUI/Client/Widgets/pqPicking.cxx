/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqPicking.h"

#include "pqPipelineData.h"
#include "pqPipelineObject.h"
#include "pqPipelineServer.h"
#include "pqServer.h"

#include <vtkSMSourceProxy.h>
#include <vtkSMPointWidgetProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkCommand.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyle.h>
#include <vtkSMInputProperty.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMPointLabelDisplayProxy.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkUnstructuredGrid.h>

pqPicking::pqPicking(vtkSMRenderModuleProxy* rm, QObject* p)
  : QObject(p), RenderModule(rm)
{
  // quietly setup picking and highlighting pipeline

  // create the pick filter
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  this->PickFilter = proxyManager->NewProxy("filters", "Pick");
  this->PickFilter->SetConnectionID(rm->GetConnectionID());
  //this->PickFilter->CreateParts();  // needed?

  // specify we want to use points to pick with
  vtkSMIntVectorProperty *useIdToPickProperty = vtkSMIntVectorProperty::SafeDownCast(this->PickFilter->GetProperty("UseIdToPick"));
  useIdToPickProperty->SetElements1(0);

  // specify we want to pick cell or point
  vtkSMIntVectorProperty *pickCellProperty = vtkSMIntVectorProperty::SafeDownCast(this->PickFilter->GetProperty("PickCell"));
  pickCellProperty->SetElements1(1);

  // create a display for the output of the pick filter
  this->PickDisplay = rm->CreateDisplayProxy();
  
  // add pick filter output to display proxy
  vtkSMProxyProperty *pp
    = vtkSMProxyProperty::SafeDownCast(this->PickDisplay->GetProperty("Input"));
  pp->AddProxy(this->PickFilter);
  
  // thick red wire highlighting
  vtkSMDoubleVectorProperty *doubleProp = vtkSMDoubleVectorProperty::SafeDownCast(this->PickDisplay->GetProperty("Color"));
  doubleProp->SetElements3(1,1,1);
  doubleProp = vtkSMDoubleVectorProperty::SafeDownCast(this->PickDisplay->GetProperty("LineWidth"));
  doubleProp->SetElements1(3.0);
  vtkSMDataObjectDisplayProxy::SafeDownCast(this->PickDisplay)->SetRepresentationCM(vtkSMDataObjectDisplayProxy::WIREFRAME);
  vtkSMIntVectorProperty::SafeDownCast(this->PickDisplay->GetProperty("ScalarVisibility"))->SetElement(0,0);
  
  // proxy to retrieve pick results
  this->PickRetriever = vtkSMPointLabelDisplayProxy::SafeDownCast(proxyManager->NewProxy("displays", "PointLabelDisplay"));
  this->PickRetriever->SetConnectionID(rm->GetConnectionID());
  vtkSMInputProperty *inputProp;
  inputProp = vtkSMInputProperty::SafeDownCast(this->PickRetriever->GetProperty("Input"));
  inputProp->AddProxy(this->PickFilter);
  this->PickRetriever->SetVisibilityCM(0);  // invisible

  this->EmptySet = vtkUnstructuredGrid::New();

}

pqPicking::~pqPicking()
{
  // cleanup picking and highlighting pipeline
  this->PickFilter->Delete();
  this->PickRetriever->Delete();
  this->PickDisplay->Delete();
  this->EmptySet->Delete();
}

void pqPicking::computeSelection(vtkObject* o, unsigned long, void*, void*, vtkCommand* command)
{

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(o);
  if(!iren)
    return;

  command->SetAbortFlag(1);

  int* pos = iren->GetEventPosition();

  this->computeSelection(iren, pos[0], pos[1]);

}

void pqPicking::computeSelection(vtkRenderWindowInteractor* /*iren*/, int X, int Y)
{
  // TODO: find a better way to decide the input of the pick filter.
  pqPipelineData* pipeline = pqPipelineData::instance();

  vtkSMProxy* inputProxy = pipeline->currentProxy();

  if(!inputProxy)
    return;
  
  vtkSMRenderModuleProxy* rm = pipeline->getRenderModule(pipeline->getWindowFor(inputProxy));


  // make sure the object is in the window we are picking in
  if(rm != this->RenderModule)
    return;
  
  // get world point from display point
  double Z = rm->GetZBufferValue(X, Y);

  if (Z == 1.0)
    {
    // remove from render module
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->RenderModule->GetProperty("Displays"));
    pp->RemoveProxy(this->PickDisplay);
    rm->UpdateVTKObjects();
    emit this->selectionChanged(inputProxy, this->EmptySet);
    rm->StillRender();  // TODO : replace with correct update policy
    return;
    }

  double pt[4];

  // Compute DisplayToWorld
  rm->GetRenderer()->SetDisplayPoint(double(X), double(Y), Z);
  rm->GetRenderer()->DisplayToWorld();
  rm->GetRenderer()->GetWorldPoint(pt);

  pt[0] /= pt[3];
  pt[1] /= pt[3];
  pt[2] /= pt[3];

  // give the world point to the pick filter
  vtkSMDoubleVectorProperty *worldPointProperty = vtkSMDoubleVectorProperty::SafeDownCast(this->PickFilter->GetProperty("WorldPoint"));
  worldPointProperty->SetElements3(pt[0], pt[1], pt[2]);
  
  // set the input to the pick filter
  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(this->PickFilter->GetProperty("Input"));
  inputProp->AddProxy(inputProxy);
  
  // execute filter
  this->PickFilter->UpdateVTKObjects();
  vtkSMSourceProxy::SafeDownCast(this->PickFilter)->UpdatePipeline();
  
  // get results
  this->PickRetriever->UpdateVTKObjects();
  this->PickRetriever->Update();
  
  vtkUnstructuredGrid* results = this->PickRetriever->GetCollectedData();
  emit this->selectionChanged(inputProxy, results);

  // add to render module
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->RenderModule->GetProperty("Displays"));
  pp->AddProxy(this->PickDisplay);
  rm->UpdateVTKObjects();

  rm->StillRender();  // TODO: replace with correct update policy

}

