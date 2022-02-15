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

  this->setupConnections(this->WidgetProxy, smgroup, smproxy);

  END_UNDO_EXCLUDE();

  this->setupUserObserver(smproxy);
}

//-----------------------------------------------------------------------------
pqInteractiveProperty2DWidget::~pqInteractiveProperty2DWidget()
{
  // ensures that the widget proxy is removed from the active view, if any.
  pqView* oldview = this->view();
  if (oldview != nullptr)
  {
    vtkSMPropertyHelper(oldview->getProxy(), "HiddenRepresentations").Remove(this->WidgetProxy);
    oldview->getProxy()->UpdateVTKObjects();

    this->pqPropertyWidget::setView(nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqInteractiveProperty2DWidget::setWidgetVisible(bool val)
{
  if (this->WidgetVisibility != val)
  {
    SM_SCOPED_TRACE(CallFunction)
      .arg(val ? "Show2DWidgets" : "Hide2DWidgets")
      .arg("proxy", this->proxy())
      .arg("comment", "toggle 2D widget visibility (only when running from the GUI)");

    this->WidgetVisibility = val;
    this->updateWidgetVisibility();
    Q_EMIT this->widgetVisibilityToggled(val);
  }
}

//-----------------------------------------------------------------------------
void pqInteractiveProperty2DWidget::updateWidgetVisibility()
{
  bool visible = this->isSelected() && this->isWidgetVisible() && this->view();
  vtkSMProxy* wdgProxy = this->WidgetProxy;
  assert(wdgProxy);

  vtkSMPropertyHelper(wdgProxy, "Enabled", true).Set(visible);
  vtkSMPropertyHelper(wdgProxy, "Visibility", true).Set(visible);
  wdgProxy->UpdateVTKObjects();

  this->render();
  Q_EMIT this->widgetVisibilityUpdated(visible);
}
