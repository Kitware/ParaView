/*=========================================================================

   Program: ParaView
   Module:    pqSelectThroughPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

#include "pqApplicationCore.h"
#include "pqSelectThroughPanel.h"
#include "pqImplicitPlaneWidget.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqRubberBandHelper.h"
#include "pqActiveView.h"
#include "pqRenderView.h"

#include <pqCollapsedGroup.h>

#include <vtkPVXMLElement.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyListDomain.h>
#include <vtkSMProxyProperty.h>
#include <vtkRenderer.h>
#include <vtkSMRenderViewProxy.h>

#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QPushButton>

//////////////////////////////////////////////////////////////////////////////
// pqSelectThroughPanel::pqImplementation
class pqSelectThroughPanel::pqImplementation
{
public:
  pqImplementation() :
    PartiallyWithinWidget(tr("Partially Within")),
    PassThroughWidget(tr("Preserve Topology")),
    ShowBoundsWidget(tr("Show Frustum")),
    InsideOutWidget(tr("Inside Out")),
    StartSelect(tr("Start Select"))
  {
  }
  QCheckBox PartiallyWithinWidget;
  QCheckBox PassThroughWidget;
  QCheckBox ShowBoundsWidget;
  QCheckBox InsideOutWidget;
  QPushButton StartSelect;
};

//----------------------------------------------------------------------------
pqSelectThroughPanel::~pqSelectThroughPanel() 
{
  delete this->Implementation;
  delete this->RubberBandHelper;
  delete[] this->Verts;
}


//----------------------------------------------------------------------------
pqSelectThroughPanel::pqSelectThroughPanel(pqProxy* object_proxy, QWidget* p) :
  Superclass(object_proxy, p),
  Implementation(new pqImplementation())
{  
  QVBoxLayout* const panel_layout = new QVBoxLayout(this);
  panel_layout->addWidget(&this->Implementation->StartSelect);
  panel_layout->addWidget(&this->Implementation->PartiallyWithinWidget);
  panel_layout->addWidget(&this->Implementation->PassThroughWidget);
  panel_layout->addWidget(&this->Implementation->ShowBoundsWidget);
  panel_layout->addWidget(&this->Implementation->InsideOutWidget);
  panel_layout->addStretch();

//  this->Mode = pqRubberBandHelper::INTERACT;

  this->propertyManager()->registerLink(
    &this->Implementation->PartiallyWithinWidget, "checked", SIGNAL(toggled(bool)),
    this->proxy(), 
    this->proxy()->GetProperty("PartiallyWithin"));

  this->propertyManager()->registerLink(
    &this->Implementation->PassThroughWidget, "checked", SIGNAL(toggled(bool)),
    this->proxy(), 
    this->proxy()->GetProperty("PreserveTopology"));

  this->propertyManager()->registerLink(
    &this->Implementation->ShowBoundsWidget, "checked", SIGNAL(toggled(bool)),
    this->proxy(), 
    this->proxy()->GetProperty("ShowBounds"));

  this->propertyManager()->registerLink(
    &this->Implementation->InsideOutWidget, "checked", SIGNAL(toggled(bool)),
    this->proxy(), 
    this->proxy()->GetProperty("InsideOut"));

  this->RubberBandHelper = new pqRubberBandHelper;

  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqView*)),
    this, SLOT(setActiveView(pqView*)));

  QObject::connect(
    &this->Implementation->StartSelect, SIGNAL(pressed()),
    this, SLOT(startSelect()));

  QObject::connect(
    this->RubberBandHelper, SIGNAL(selectionFinished()),
    this, SLOT(endSelect()));

  this->Verts = new double[32];
}

//----------------------------------------------------------------------------
void pqSelectThroughPanel::startSelect()
{
#if 0
  if (!this->RubberBandHelper->RenderModule)
    {
    //try to give the helper the active view to work on
    pqActiveView &aview = pqActiveView::instance();
    pqView *cview = aview.current();
    pqRenderView* rm = qobject_cast<pqRenderView*>(cview);
    if (!rm)
      {
      return;
      }
    this->RubberBandHelper->RenderModule = rm;
    }

  if (this->RubberBandHelper->setRubberBandOn())
    {
    this->Mode = pqRubberBandHelper::SELECT;
    }
#endif
}

//----------------------------------------------------------------------------
void pqSelectThroughPanel::endSelect()
  {
#if 0
  if (!this->RubberBandHelper->setRubberBandOff())
    {
    return;
    }
  
  this->Mode = pqRubberBandHelper::INTERACT;

  // Make sure the selection rectangle is in the right order.
  int rectangle[4];
  rectangle[0] = this->RubberBandHelper->Xs;
  rectangle[1] = this->RubberBandHelper->Ys;
  rectangle[2] = this->RubberBandHelper->Xe;
  rectangle[3] = this->RubberBandHelper->Ye;
  int displayRectangle[4];
  pqRubberBandHelper::ReorderBoundingBox(rectangle, displayRectangle);
  if (displayRectangle[0] == displayRectangle[2])
    {
    displayRectangle[2] += 1;
    }
  if (displayRectangle[1] == displayRectangle[3])
    {
    displayRectangle[3] += 1;
    }

  //convert screen rectangle to world frustum
  pqRenderView* rvm = this->RubberBandHelper->RenderModule;
  vtkRenderer *renderer = rvm->getRenderViewProxy()->GetRenderer();

  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[0]);

  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[4]);

  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[8]);

  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[12]);

  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[16]);

  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[20]);

  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[24]);
  
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[28]);    

  this->setModified();

  this->RubberBandHelper->RenderModule->render();
#endif
}

//-----------------------------------------------------------------------------
void pqSelectThroughPanel::setActiveView(pqView* aview)
{
  (void)aview;
#if 0
  pqRenderView* rm = qobject_cast<pqRenderView*>(aview);
  if (!rm)
    {
    return;
    }
  
  // make sure the active render module has the right interactor
  if (this->Mode == pqRubberBandHelper::SELECT)
    {
    // the previous view should revert to the previous interactor,
    this->RubberBandHelper->setRubberBandOff();
    // the current view then starts using the select interactor
    this->RubberBandHelper->setRubberBandOn(rm);
    }

  this->RubberBandHelper->RenderModule = rm;
#endif 
}

//----------------------------------------------------------------------------
void pqSelectThroughPanel::accept()
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
  this->proxy()->GetProperty("Frustum"));
  dvp->SetElements(this->Verts);

  this->proxy()->UpdateVTKObjects();

  pqObjectPanel::accept();
}
