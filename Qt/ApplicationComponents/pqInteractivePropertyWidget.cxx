// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqInteractivePropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"

#include <QtDebug>

#include <cassert>

//-----------------------------------------------------------------------------
pqInteractivePropertyWidget::pqInteractivePropertyWidget(const char* widget_smgroup,
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
  vtkSMNewWidgetRepresentationProxy* wdgProxy =
    vtkSMNewWidgetRepresentationProxy::SafeDownCast(aProxy);
  if (aProxy == nullptr)
  {
    qCritical("Failed to create proxy for 3D Widget. Aborting for debugging purposes.");
    abort();
  }
  if (wdgProxy == nullptr)
  {
    qCritical() << "Proxy (" << widget_smgroup << ", " << widget_smname
                << ") must be a "
                   "vtkSMNewWidgetRepresentationProxy instance. It however is a '"
                << aProxy->GetClassName() << "'. Aborting for debugging purposes.";
    abort();
  }
  assert(wdgProxy);
  this->WidgetProxy = wdgProxy;

  this->setupConnections(this->WidgetProxy, smgroup, smproxy);

  END_UNDO_EXCLUDE();

  this->setupUserObserver(smproxy);
}

//-----------------------------------------------------------------------------
pqInteractivePropertyWidget::~pqInteractivePropertyWidget()
{
  // ensures that the widget proxy is removed from the active view, if any.
  pqView* oldview = this->view();
  if (oldview != nullptr)
  {
    vtkSMPropertyHelper(oldview->getProxy(), "HiddenRepresentations", true)
      .Remove(this->WidgetProxy);
    oldview->getProxy()->UpdateVTKObjects();

    this->pqPropertyWidget::setView(nullptr);
  }
}

//-----------------------------------------------------------------------------
std::vector<vtkVector3d> pqInteractivePropertyWidget::displayToWorldCoordinates(
  const std::vector<vtkVector3d>& displayCoordPoints)
{
  vtkRenderer* renderer = this->getRenderer();
  if (!renderer)
  {
    return {};
  }

  std::vector<vtkVector3d> worldCoordPoints(displayCoordPoints.size());
  // Compute all points from display coord to world coord.
  for (std::size_t i = 0; i < displayCoordPoints.size(); i++)
  {
    worldCoordPoints[i] = renderer->DisplayToWorld(displayCoordPoints[i]);
  }

  return worldCoordPoints;
}

//-----------------------------------------------------------------------------
double pqInteractivePropertyWidget::getFocalPointDepth()
{
  vtkRenderer* renderer = this->getRenderer();
  if (!renderer)
  {
    return -1.0;
  }

  // Recover focal point in display coordinates to get the Z coordinate for the new depth position
  // of the ruler.
  double cameraFocalPointWorldCoord[4] = { 0.0, 0.0, 0.0, 0.0 };
  renderer->GetActiveCamera()->GetFocalPoint(cameraFocalPointWorldCoord);
  renderer->WorldToDisplay(
    cameraFocalPointWorldCoord[0], cameraFocalPointWorldCoord[1], cameraFocalPointWorldCoord[2]);
  return cameraFocalPointWorldCoord[2];
}

//-----------------------------------------------------------------------------
vtkRenderer* pqInteractivePropertyWidget::getRenderer()
{
  pqRenderView* activeView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!activeView)
  {
    return nullptr;
  }

  vtkSMRenderViewProxy* renderViewProxy =
    vtkSMRenderViewProxy::SafeDownCast(activeView->getProxy());
  if (!renderViewProxy)
  {
    return nullptr;
  }

  return renderViewProxy->GetRenderer();
}
