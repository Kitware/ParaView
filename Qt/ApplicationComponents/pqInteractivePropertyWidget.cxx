/*=========================================================================

   Program: ParaView
   Module:  pqInteractivePropertyWidget.cxx

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
#include "pqInteractivePropertyWidget.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqPropertyLinks.h"
#include "pqRenderViewBase.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QtDebug>

#include <cassert>

class pqInteractivePropertyWidget::pqInternals
{
public:
  vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> WidgetProxy;
  vtkWeakPointer<vtkSMProxy> DataSource;
  vtkSmartPointer<vtkSMPropertyGroup> SMGroup;
  bool WidgetVisibility;
  unsigned long UserEventObserverId;

  pqInternals()
    : WidgetVisibility(false)
    , UserEventObserverId(0)
  {
  }
};

//-----------------------------------------------------------------------------
pqInteractivePropertyWidget::pqInteractivePropertyWidget(const char* widget_smgroup,
  const char* widget_smname, vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup,
  QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInteractivePropertyWidget::pqInternals())
{
  assert(widget_smgroup);
  assert(widget_smname);
  assert(smproxy);
  assert(smgroup);

  BEGIN_UNDO_EXCLUDE();

  pqInternals& internals = (*this->Internals);
  internals.SMGroup = smgroup;

  pqServer* server =
    pqApplicationCore::instance()->getServerManagerModel()->findServer(smproxy->GetSession());

  // Check is server is a Catalyst session. If so, we need to create the widget
  // proxies on the "display-session".
  server = pqLiveInsituVisualizationManager::displaySession(server);

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> aProxy;
  aProxy.TakeReference(pxm->NewProxy(widget_smgroup, widget_smname));
  vtkSMNewWidgetRepresentationProxy* wdgProxy =
    vtkSMNewWidgetRepresentationProxy::SafeDownCast(aProxy);
  if (aProxy == NULL)
  {
    qCritical("Failed to create proxy for 3D Widget. Aborting for debugging purposes.");
    abort();
  }
  if (wdgProxy == NULL)
  {
    qCritical() << "Proxy (" << widget_smgroup << ", " << widget_smname
                << ") must be a "
                   "vtkSMNewWidgetRepresentationProxy instance. It however is a '"
                << aProxy->GetClassName() << "'. Aborting for debugging purposes.";
    abort();
  }
  assert(wdgProxy);

  internals.WidgetProxy = wdgProxy;

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(wdgProxy);

  // Setup links between the proxy that the widget is going to be controlling
  wdgProxy->LinkProperties(smproxy, smgroup);

  wdgProxy->UpdateVTKObjects();

  // Marking this as a prototype ensures that the undo/redo system doesn't track
  // changes to the widget.
  wdgProxy->PrototypeOn();

  pqCoreUtilities::connect(wdgProxy, vtkCommand::InteractionEvent, this, SIGNAL(changeAvailable()));
  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::EndInteractionEvent, this, SIGNAL(changeFinished()));

  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::StartInteractionEvent, this, SIGNAL(startInteraction()));
  pqCoreUtilities::connect(wdgProxy, vtkCommand::InteractionEvent, this, SIGNAL(interaction()));
  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::EndInteractionEvent, this, SIGNAL(endInteraction()));

  if (vtkSMProperty* input = smgroup->GetProperty("Input"))
  {
    this->addPropertyLink(this, "dataSource", SIGNAL(dummySignal()), input);
  }
  else
  {
    this->setDataSource(NULL);
  }

  // This ensures that when the user changes the Qt widget, we re-render to show
  // the update widget.
  this->connect(&this->links(), SIGNAL(qtWidgetChanged()), SLOT(render()));

  END_UNDO_EXCLUDE();

  internals.UserEventObserverId = smproxy->AddObserver(
    vtkCommand::UserEvent, this, &pqInteractivePropertyWidget::handleUserEvent);
}

//-----------------------------------------------------------------------------
pqInteractivePropertyWidget::~pqInteractivePropertyWidget()
{
  pqInternals& internals = (*this->Internals);
  if (internals.UserEventObserverId > 0 && this->proxy())
  {
    this->proxy()->RemoveObserver(internals.UserEventObserverId);
    internals.UserEventObserverId = 0;
  }

  // ensures that the widget proxy is removed from the active view, if any.
  this->setView(NULL);
}

//-----------------------------------------------------------------------------
vtkSMPropertyGroup* pqInteractivePropertyWidget::propertyGroup() const
{
  return this->Internals->SMGroup;
}

//-----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy* pqInteractivePropertyWidget::widgetProxy() const
{
  return this->Internals->WidgetProxy;
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setView(pqView* pqview)
{
  if (pqview != NULL && pqview->getServer()->session() != this->widgetProxy()->GetSession())
  {
    pqview = NULL;
  }

  pqView* rview = qobject_cast<pqRenderViewBase*>(pqview);
  pqView* oldview = this->view();
  if (oldview == rview)
  {
    return;
  }

  if (oldview)
  {
    vtkSMPropertyHelper(oldview->getProxy(), "HiddenRepresentations").Remove(this->widgetProxy());
    oldview->getProxy()->UpdateVTKObjects();
  }
  this->Superclass::setView(rview);
  if (rview)
  {
    vtkSMPropertyHelper(rview->getProxy(), "HiddenRepresentations").Add(this->widgetProxy());
    rview->getProxy()->UpdateVTKObjects();
  }
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::select()
{
  this->Superclass::select();
  this->placeWidget();
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::deselect()
{
  this->Superclass::deselect();
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
bool pqInteractivePropertyWidget::isWidgetVisible() const
{
  return this->Internals->WidgetVisibility;
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setWidgetVisible(bool val)
{
  pqInternals& internals = (*this->Internals);
  if (internals.WidgetVisibility != val)
  {
    SM_SCOPED_TRACE(CallFunction)
      .arg(val ? "Show3DWidgets" : "Hide3DWidgets")
      .arg("proxy", this->proxy())
      .arg("comment", "toggle 3D widget visibility (only when running from the GUI)");

    internals.WidgetVisibility = val;
    this->updateWidgetVisibility();
    emit this->widgetVisibilityToggled(val);
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::updateWidgetVisibility()
{
  bool visible = this->isSelected() && this->isWidgetVisible() && this->view();
  vtkSMProxy* wdgProxy = this->widgetProxy();
  assert(wdgProxy);

  vtkSMPropertyHelper(wdgProxy, "Visibility", true).Set(visible);
  vtkSMPropertyHelper(wdgProxy, "Enabled", true).Set(visible);
  wdgProxy->UpdateVTKObjects();
  this->render();
  emit this->widgetVisibilityUpdated(visible);
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setDataSource(vtkSMProxy* dsource)
{
  this->Internals->DataSource = dsource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqInteractivePropertyWidget::dataSource() const
{
  return this->Internals->DataSource;
}

//-----------------------------------------------------------------------------
vtkBoundingBox pqInteractivePropertyWidget::dataBounds() const
{
  if (vtkSMSourceProxy* dsrc = vtkSMSourceProxy::SafeDownCast(this->dataSource()))
  {
    // FIXME: we need to get the output port number correctly. For now, just use
    // 0.
    vtkPVDataInformation* dataInfo = dsrc->GetDataInformation(0);
    vtkBoundingBox bbox(dataInfo->GetBounds());
    return bbox;
  }
  else
  {
    vtkBoundingBox bbox;
    return bbox;
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::render()
{
  if (pqView* pqview = this->view())
  {
    pqview->render();
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::reset()
{
  this->Superclass::reset();
  this->render();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::handleUserEvent(
  vtkObject* caller, unsigned long eventid, void* calldata)
{
  Q_UNUSED(caller);
  Q_UNUSED(eventid);

  assert(caller == this->proxy());
  assert(eventid == vtkCommand::UserEvent);

  const char* message = reinterpret_cast<const char*>(calldata);
  if (message != NULL && strcmp("HideWidget", message) == 0)
  {
    this->setWidgetVisible(false);
  }
  else if (message != NULL && strcmp("ShowWidget", message) == 0)
  {
    this->setWidgetVisible(true);
  }
}
