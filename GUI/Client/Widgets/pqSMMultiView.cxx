/*=========================================================================

   Program:   ParaQ
   Module:    pqSMMultiView.cxx

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

#include "pqSMMultiView.h"

#include "pqMultiViewFrame.h"
#include "pqPipelineData.h"
#include "pqPipelineModel.h"
#include "pqRenderViewProxy.h"
#include "pqServer.h"
#include "pqSetName.h"
#include "pqSMAdaptor.h"

#include <QObject>

#include "vtkCommand.h"
#include "vtkCollection.h"
#include "vtkRenderer.h"
#include "QVTKWidget.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkPVAxesWidget.h"
#include "vtkSMRenderModuleProxy.h"


// delete items automatically when another object is deleted
class vtkPiggyBackDelete : public vtkCommand
{
public:
  vtkTypeMacro(vtkPiggyBackDelete,vtkCommand)
  static vtkPiggyBackDelete* New()
    {
    return new vtkPiggyBackDelete;
    }
  
  void AddRider(vtkObject* o)
    {
    this->Riders->AddItem(o);
    }
  void RemoveRider(vtkObject* o)
    {
    this->Riders->RemoveItem(o);
    }

protected:
  vtkPiggyBackDelete() 
    {
    this->Riders = vtkCollection::New();
    }
  ~vtkPiggyBackDelete() 
    {
    this->Riders->Delete();
    }

  virtual void Execute(vtkObject* pig, unsigned long, void*)
    {
    this->Riders->InitTraversal();
    vtkObject* o = Riders->GetNextItemAsObject();
    for( ; o != NULL; o = Riders->GetNextItemAsObject())
      {
      o->Delete();
      }
    pig->RemoveObserver(this);
    this->Delete();
    }

  vtkCollection* Riders;

};


class pqMultiViewRenderModuleUpdater : public QObject
{
public:
  pqMultiViewRenderModuleUpdater(vtkSMProxy* view, QWidget* topWidget, QWidget* p)
    : QObject(p), View(view), TopWidget(topWidget) {}

protected:
  bool eventFilter(QObject* caller, QEvent* e)
    {
    // TODO, apparently, this should watch for window position changes, not resizes
    if(e->type() == QEvent::Resize)
      {
      // find top level window;
      QWidget* me = qobject_cast<QWidget*>(caller);
      
      vtkSMIntVectorProperty* prop = 0;
      
      // set size of main window
      prop = vtkSMIntVectorProperty::SafeDownCast(this->View->GetProperty("GUISize"));
      if(prop)
        {
        prop->SetElements2(this->TopWidget->width(), this->TopWidget->height());
        }
      
      // position relative to main window
      prop = vtkSMIntVectorProperty::SafeDownCast(this->View->GetProperty("WindowPosition"));
      if(prop)
        {
        QPoint pos(0,0);
        pos = me->mapTo(this->TopWidget, pos);
        prop->SetElements2(pos.x(), pos.y());
        }
      }
    return false;
    }

  vtkSMProxy* View;
  QWidget* TopWidget;

};


QVTKWidget *ParaQ::AddQVTKWidget(pqMultiViewFrame *frame, QWidget *topWidget,
    pqServer *server)
{
  if(!frame || !topWidget || !server)
    {
    return 0;
    }

  vtkSMMultiViewRenderModuleProxy* rm = server->GetRenderModule();
  vtkSMRenderModuleProxy* view = vtkSMRenderModuleProxy::SafeDownCast(
      rm->NewRenderModule());

  // if this property exists (server/client mode), render remotely
  // this should change to a user controlled setting, but this is here for testing
  vtkSMProperty* prop = view->GetProperty("CompositeThreshold");
  if(prop)
    {
    pqSMAdaptor::setElementProperty(view, prop, 0.0);  // remote render
    }
  view->UpdateVTKObjects();
  
  // turn on vtk light kit
  view->SetUseLight(1);
  // turn off main light
  vtkSMIntVectorProperty::SafeDownCast(view->GetProperty("LightSwitch"))->SetElement(0, 0);

  
  QVTKWidget* widget = new QVTKWidget(frame) << pqSetName("Viewport");
  frame->setMainWidget(widget);

  // gotta tell SM about window positions
  pqMultiViewRenderModuleUpdater* updater = new pqMultiViewRenderModuleUpdater(
      view, topWidget, widget);
  widget->installEventFilter(updater);

  widget->SetRenderWindow(view->GetRenderWindow());

  pqRenderViewProxy* vp = pqRenderViewProxy::New();
  vp->SetRenderModule(view);
  vtkPVGenericRenderWindowInteractor* iren =
      vtkPVGenericRenderWindowInteractor::SafeDownCast(view->GetInteractor());
  iren->SetPVRenderView(vp);
  vp->Delete();
  iren->Enable();
  
  // add a orientation axes
  vtkPVAxesWidget* axes = vtkPVAxesWidget::New();
  axes->SetParentRenderer(view->GetRenderer());
  axes->SetViewport(0,0,0.25,0.25);
  axes->SetInteractor(iren);
  axes->SetEnabled(1);
  axes->SetInteractive(0);

  vtkPiggyBackDelete* del = vtkPiggyBackDelete::New();
  del->AddRider(axes);
  view->AddObserver(vtkCommand::DeleteEvent, del);
  
  // Keep a map of window to render module. Add the new window to the
  // pipeline data structure.
  pqPipelineData *pipeline = pqPipelineData::instance();
  if(pipeline)
    {
    pipeline->getModel()->addWindow(widget, server);
    pipeline->addViewMapping(widget, view);
    }

  return widget;
}

