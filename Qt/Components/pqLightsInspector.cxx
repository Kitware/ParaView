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
#include "pqUndoStack.h"
#include "pqView.h"

//=============================================================================
class pqLightsInspector::pqInternals
{
public:
  Ui::LightsInspector Ui;

  pqInternals(pqLightsInspector* self)
  {
    this->Ui.setupUi(self);

    // this might not be necessary
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), self,
      SLOT(setActiveView(pqView*)));

    // hook up the add light button
    connect(this->Ui.addLight, SIGNAL(pressed()), self, SLOT(addLight()));

    // and, temporily the remove light button for testing
    connect(this->Ui.removeLight, SIGNAL(pressed()), self, SLOT(removeLight()));
  }
  ~pqInternals() {}
};

//-----------------------------------------------------------------------------
pqLightsInspector::pqLightsInspector(
  QWidget* parentObject, Qt::WindowFlags f, bool arg_autotracking)
  : Superclass(parentObject, f)
  , Internals(new pqLightsInspector::pqInternals(this))
{
  pqInternals& internals = (*this->Internals);
}

//-----------------------------------------------------------------------------
pqLightsInspector::~pqLightsInspector()
{
}

//-----------------------------------------------------------------------------
void pqLightsInspector::setActiveView(pqView* newview)
{
  // we may need to populate the qt panel here
}

//-----------------------------------------------------------------------------
void pqLightsInspector::addLight()
{
  pqView* pv = pqActiveObjects::instance().activeView();
  if (pv == nullptr)
  {
    return;
  }
  vtkSMRenderViewProxy* view = vtkSMRenderViewProxy::SafeDownCast(pv->getViewProxy());
  if (view == nullptr)
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
  // todo: call update to make the change take effect now
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::removeLight()
{
  pqView* pv = pqActiveObjects::instance().activeView();
  if (pv == nullptr)
  {
    return;
  }
  vtkSMRenderViewProxy* view = vtkSMRenderViewProxy::SafeDownCast(pv->getViewProxy());
  if (view == nullptr)
  {
    return;
  }

  unsigned int nlights = vtkSMPropertyHelper(view, "ExtraLight").GetNumberOfElements();
  if (nlights < 1)
  {
    return;
  }

  // tell python trace to add the light
  SM_SCOPED_TRACE(CallFunction)
    .arg("RemoveLight")
    .arg("view", view)
    .arg("comment", "remove last light added to the view");

  // tell undo/redo and state about the new light
  BEGIN_UNDO_SET("Remove Light");

  // todo: this works but do I need to do something more?
  vtkSMProxy* light = vtkSMPropertyHelper(view, "ExtraLight").GetAsProxy(nlights - 1);
  vtkSMPropertyHelper(view, "ExtraLight").Remove(light);

  END_UNDO_SET();
}
