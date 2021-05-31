/*=========================================================================

  Program:   ParaView
  Module:    pqInteractiveProperty2DWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqInteractiveProperty2DWidget.h"

#include "pqApplicationCore.h"
#include "pqContextView.h"
#include "pqCoreUtilities.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkPVDataInformation.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <QtDebug>

#include <cassert>

//-----------------------------------------------------------------------------
pqInteractiveProperty2DWidget::pqInteractiveProperty2DWidget(const char* widget_smgroup,
  const char* widget_smname, vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup,
  QWidget* parentObject)
  : Superclass(widget_smgroup, widget_smname, smproxy, smgroup, parentObject)
{
  BEGIN_UNDO_EXCLUDE();
  pqServer* server =
    pqApplicationCore::instance()->getServerManagerModel()->findServer(smproxy->GetSession());

  // Check is server is a Catalyst session. If so, we need to create the widget
  // proxies on the "display-session".
  server = pqLiveInsituVisualizationManager::displaySession(server);

  // Initalize the widget.
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> aProxy;
  aProxy.TakeReference(pxm->NewProxy(widget_smgroup, widget_smname));
  vtkSMNew2DWidgetRepresentationProxy* wdgProxy =
    vtkSMNew2DWidgetRepresentationProxy::SafeDownCast(aProxy);
  if (aProxy == nullptr)
  {
    qCritical("Failed to create proxy for 2D Widget. Aborting for debugging purposes.");
    abort();
  }
  if (wdgProxy == nullptr)
  {
    qCritical() << "Proxy (" << widget_smgroup << ", " << widget_smname
                << ") must be a "
                   "vtkSMNew2DWidgetRepresentationProxy instance. It however is a '"
                << aProxy->GetClassName() << "'. Aborting for debugging purposes.";
  }
  assert(wdgProxy);
  this->WidgetProxy = wdgProxy;

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
  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::StartInteractionEvent, this, SIGNAL(changeAvailable()));
  pqCoreUtilities::connect(wdgProxy, vtkCommand::InteractionEvent, this, SIGNAL(interaction()));
  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::EndInteractionEvent, this, SIGNAL(endInteraction()));

  if (vtkSMProperty* input = smgroup->GetProperty("Input"))
  {
    this->addPropertyLink(this, "dataSource", SIGNAL(dummySignal()), input);
  }
  else
  {
    this->setDataSource(nullptr);
  }

  // This ensures that when the user changes the Qt widget, we re-render to show
  // the update widget.
  this->connect(&this->links(), SIGNAL(qtWidgetChanged()), SLOT(render()));

  END_UNDO_EXCLUDE();

  this->setupUserObserver(smproxy);
}

//-----------------------------------------------------------------------------
pqInteractiveProperty2DWidget::~pqInteractiveProperty2DWidget()
{
  // ensures that the widget proxy is removed from the active view, if any.
  this->setView(nullptr);
}

//-----------------------------------------------------------------------------
void pqInteractiveProperty2DWidget::setView(pqView* pqview)
{
  if (pqview != nullptr && pqview->getServer()->session() != this->widgetProxy()->GetSession())
  {
    pqview = nullptr;
  }

  pqView* rview = qobject_cast<pqContextView*>(pqview);
  pqView* oldview = this->view();
  if (oldview != nullptr)
  {
    vtkSMPropertyHelper(oldview->getProxy(), "HiddenRepresentations").Remove(this->WidgetProxy);
    oldview->getProxy()->UpdateVTKObjects();

    this->pqPropertyWidget::setView(nullptr);
    this->updateWidgetVisibility();
  }
}

//-----------------------------------------------------------------------------
void pqInteractiveProperty2DWidget::setWidgetVisible(bool val)
{
  if (this->WidgetVisibility != val)
  {
    this->WidgetVisibility = val;
    this->updateWidgetVisibility();
    Q_EMIT this->widgetVisibilityToggled(val);
  }
}

//-----------------------------------------------------------------------------
void pqInteractiveProperty2DWidget::updateWidgetVisibility()
{
  bool visible = this->isWidgetVisible() && this->view();
  bool enabled = this->isSelected() && this->isWidgetVisible() && this->view();
  vtkSMProxy* wdgProxy = this->WidgetProxy;
  assert(wdgProxy);

  vtkSMPropertyHelper(wdgProxy, "Enabled", true).Set(enabled);
  wdgProxy->UpdateVTKObjects();
  this->render();
  Q_EMIT this->widgetVisibilityUpdated(visible);
}

//-----------------------------------------------------------------------------
vtkBoundingBox pqInteractiveProperty2DWidget::dataBounds() const
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

void pqInteractiveProperty2DWidget::hideEvent(QHideEvent*)
{
  this->VisibleState = vtkSMPropertyHelper(this->widgetProxy(), "Visibility").GetAsInt() != 0;
  this->setWidgetVisible(false);
}

void pqInteractiveProperty2DWidget::showEvent(QShowEvent*)
{
  this->setWidgetVisible(this->VisibleState);
}

//-----------------------------------------------------------------------------
void pqInteractiveProperty2DWidget::render()
{
  if (pqView* pqview = this->view())
  {
    pqview->render();
  }
}
