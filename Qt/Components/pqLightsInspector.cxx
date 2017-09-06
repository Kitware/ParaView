/*=========================================================================

   Program: ParaView
   Module:  pqLightsInspector.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqLightsInspector.h"
#include "ui_pqLightsInspector.h"

#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPropertyWidget.h"
#include "pqProxyWidget.h"
#include "pqRenderView.h"
#include "pqUndoStack.h"
#include "pqView.h"

//=============================================================================
class pqLightsInspector::pqInternals
{
public:
  Ui::LightsInspector Ui;
  pqLightsInspector* self;

  pqInternals(pqLightsInspector* self)
    : self(self)
  {
    this->Ui.setupUi(self);

    // update the panel when user picks a different view
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), self,
      SLOT(setActiveView(pqView*)));

    // hook up the add light button
    connect(this->Ui.addLight, SIGNAL(pressed()), self, SLOT(addLight()));

    // and, temporily the remove light button for testing
    connect(this->Ui.removeLight, SIGNAL(pressed()), self, SLOT(removeLight()));

    // populate the area with controls for each light
    this->updateLightWidgets();
  }

  ~pqInternals()
  {
    QLayoutItem* child;
    while ((child = this->Ui.verticalLayout_2->takeAt(0)) != 0)
    {
      if (child->widget())
      {
        child->widget()->setParent(0);
        delete child->widget();
      }
      delete child;
    }
  }

  vtkSMRenderViewProxy* getActiveView()
  {
    // returns nullptr if the active view is incompatible,
    // otherwise returns the corresponding SMViewProxy
    pqView* pv = pqActiveObjects::instance().activeView();
    if (pv == nullptr)
    {
      return nullptr;
    }
    vtkSMRenderViewProxy* view = vtkSMRenderViewProxy::SafeDownCast(pv->getViewProxy());
    return view;
  }

  void updateLightWidgets()
  {
    // we populate the qt panel with child widgets for each light here

    // clear current contents
    QLayoutItem* child;
    while ((child = this->Ui.verticalLayout_2->takeAt(0)) != 0)
    {
      if (child->widget())
      {
        child->widget()->setParent(0);
        delete child->widget();
      }
      delete child;
    }
    vtkSMRenderViewProxy* view = this->getActiveView();
    if (!view)
    {
      return;
    }

    // add new contents
    unsigned int nlights = vtkSMPropertyHelper(view, "ExtraLight").GetNumberOfElements();
    for (unsigned int i = 0; i < nlights; ++i)
    {
      vtkSMProxy* light = vtkSMPropertyHelper(view, "ExtraLight").GetAsProxy(i);

      // todo: we want to customize this to make sure controls are
      // consistent with the light type
      pqProxyWidget* lightWidget = new pqProxyWidget(light, self);
      lightWidget->setApplyChangesImmediately(true);
      lightWidget->updatePanel();

      // add it to this->Ui.scrollArea
      this->Ui.verticalLayout_2->addWidget(lightWidget);
      this->Ui.verticalLayout_2->addSpacing(20);
      // listen to the 'remove me' signal that this widget emits.
      pqPropertyWidget* button = lightWidget->findChild<pqPropertyWidget*>("RemoveLight");
      if (button)
      {
        self->connect(button, SIGNAL(removeLight(vtkSMProxy*)), SLOT(removeLight(vtkSMProxy*)));
      }
    }
    this->Ui.verticalLayout_2->addStretch(1);
  }
};

//-----------------------------------------------------------------------------
pqLightsInspector::pqLightsInspector(
  QWidget* parentObject, Qt::WindowFlags f, bool /* arg_autotracking */)
  : Superclass(parentObject, f)
  , Internals(new pqLightsInspector::pqInternals(this))
{
  // pqInternals& internals = (*this->Internals);
}

//-----------------------------------------------------------------------------
pqLightsInspector::~pqLightsInspector()
{
}

//-----------------------------------------------------------------------------
void pqLightsInspector::setActiveView(pqView*)
{
  this->Internals->updateLightWidgets();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::addLight()
{
  vtkSMRenderViewProxy* view = this->Internals->getActiveView();
  if (!view)
  {
    return;
  }

  // tell python trace to add the light
  SM_SCOPED_TRACE(CallFunction)
    .arg("AddLight")
    .arg("view", view)
    .arg("comment", "add a light to the view");

  // tell undo/redo and state about the new light
  BEGIN_UNDO_SET("Add Light");
  vtkSMSessionProxyManager* pxm = view->GetSessionProxyManager();
  vtkSMProxy* light = pxm->NewProxy("extra_lights", "Light");

  // standard application level logic
  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(light);

  // makes sure the light is a child of the view, so it will be deleted by it
  pxm->RegisterProxy(controller->GetHelperProxyGroupName(view), "Lights", light);
  light->Delete();

  // call vtkPVRenderView::AddLight
  vtkSMPropertyHelper(view, "ExtraLight").Add(light);
  END_UNDO_SET();

  this->Internals->updateLightWidgets();

  // make it so...
  view->UpdateVTKObjects();
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!renderView)
  {
    return;
  }
  renderView->render();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::removeLight(vtkSMProxy* lightProxy)
{
  vtkSMRenderViewProxy* view = this->Internals->getActiveView();
  if (!view)
  {
    return;
  }

  unsigned int nlights = vtkSMPropertyHelper(view, "ExtraLight").GetNumberOfElements();
  if (nlights < 1)
  {
    return;
  }

  // tell python trace to remove the light
  SM_SCOPED_TRACE(CallFunction)
    .arg("RemoveLight")
    .arg("view", view)
    .arg("comment", "remove last light added to the view");

  // tell undo/redo and state about the new light
  BEGIN_UNDO_SET("Remove Light");

  // todo: this works but do I need to do something more?
  if (!lightProxy)
  {
    lightProxy = vtkSMPropertyHelper(view, "ExtraLight").GetAsProxy(nlights - 1);
  }
  vtkSMPropertyHelper(view, "ExtraLight").Remove(lightProxy);

  END_UNDO_SET();

  this->Internals->updateLightWidgets();

  // make it so...
  view->UpdateVTKObjects();
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!renderView)
  {
    return;
  }
  renderView->render();
}
