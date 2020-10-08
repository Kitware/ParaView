/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqLookingGlassDockPanel.h"
#include "ui_pqLookingGlassDockPanel.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDebug.h"
#include "pqMainWindowEventManager.h"
#include "pqProxyWidget.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include "QVTKOpenGLWindow.h"
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkLookingGlassInterface.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkTextureObject.h"

namespace
{

class vtkEndRenderObserver : public vtkCommand
{
public:
  static vtkEndRenderObserver* New() { return new vtkEndRenderObserver; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event),
    void* vtkNotUsed(calldata)) override
  {
    if (!this->RenderWindow || !this->Interface || !this->CopyTexture)
    {
      return;
    }

    auto fb = this->RenderWindow->GetRenderFramebuffer();
    this->RenderWindow->GetState()->PushFramebufferBindings();
    fb->Bind(GL_DRAW_FRAMEBUFFER);
    fb->ActivateDrawBuffer(0);

    this->Interface->DrawLightField(this->RenderWindow, this->CopyTexture);
    this->RenderWindow->GetState()->PopFramebufferBindings();
  }

  vtkOpenGLRenderWindow* RenderWindow = nullptr;
  vtkLookingGlassInterface* Interface = nullptr;
  vtkTextureObject* CopyTexture = nullptr;

protected:
  vtkEndRenderObserver() { this->RenderWindow = nullptr; }
  ~vtkEndRenderObserver() override {}
};

class vtkViewRenderObserver : public vtkCommand
{
public:
  static vtkViewRenderObserver* New() { return new vtkViewRenderObserver; }

  void Execute(
    vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), void* callData) override
  {
    this->InteractiveRender = reinterpret_cast<int*>(callData)[0];
  }

  int InteractiveRender = 0;
};

} // end anonymous namespace

class pqLookingGlassDockPanel::pqInternal
{
public:
  Ui::pqLookingGlassDockPanel Ui;
};

void pqLookingGlassDockPanel::constructor()
{
  this->setWindowTitle("Looking Glass");
  QWidget* mainWidget = new QWidget(this);
  this->Internal = new pqLookingGlassDockPanel::pqInternal();
  Ui::pqLookingGlassDockPanel& ui = this->Internal->Ui;
  ui.setupUi(mainWidget);
  this->setWidget(mainWidget);

  pqActiveObjects* activeObjects = &pqActiveObjects::instance();

  // Set up controls
  this->connect(
    ui.RenderOnLookingGlassButton, SIGNAL(clicked(bool)), SLOT(onRenderOnLookingGlassClicked()));
  this->connect(
    ui.ResetToCenterOfRotationButton, SIGNAL(clicked(bool)), SLOT(resetToCenterOfRotation()));
  this->connect(ui.PushFocalPlaneBackButton, SIGNAL(clicked(bool)), SLOT(pushFocalPlaneBack()));
  this->connect(
    ui.PullFocalPlaneForwardButton, SIGNAL(clicked(bool)), SLOT(pullFocalPlaneForward()));

  this->connect(activeObjects, SIGNAL(serverChanged(pqServer*)), SLOT(reset()));

  // Disable button if active view is not compatible with LG
  this->connect(activeObjects, SIGNAL(viewChanged(pqView*)), SLOT(activeViewChanged(pqView*)));
}

pqLookingGlassDockPanel::~pqLookingGlassDockPanel()
{
  this->freeDisplayWindowResources();

  auto pxm = pqActiveObjects::instance().proxyManager();
  vtkNew<vtkCollection> collection;
  pxm->GetProxies("looking_glass", collection);
  for (int i = 0; i < collection->GetNumberOfItems(); ++i)
  {
    vtkSMProxy* settingsProxy = vtkSMProxy::SafeDownCast(collection->GetItemAsObject(i));
    if (!settingsProxy)
    {
      continue;
    }

    pxm->UnRegisterProxy(settingsProxy);
  }

  if (this->ViewRenderObserver)
  {
    this->ViewRenderObserver->Delete();
    this->ViewRenderObserver = nullptr;
  }

  delete this->Internal;
}

void pqLookingGlassDockPanel::setView(pqView* view)
{
  // Remove the existing proxy widget
  pqProxyWidget* proxyWidget = this->widget()->findChild<pqProxyWidget*>();
  if (proxyWidget)
  {
    if (this->View)
    {
      QObject::disconnect(proxyWidget, SIGNAL(changeFinished()), this->View, SLOT(tryRender()));
    }
    proxyWidget->parentWidget()->layout()->removeWidget(proxyWidget);
    proxyWidget->deleteLater();
  }

  // Only works with pqRenderView or subclasses
  pqRenderView* renderView = qobject_cast<pqRenderView*>(view);
  if (!renderView)
  {
    return;
  }

  if (this->View)
  {
    QObject::disconnect(this->View, SIGNAL(endRender()), this, SLOT(onRender()));
    this->View->getProxy()->RemoveObserver(this->ViewRenderObserver);
    this->ViewRenderObserver->Delete();
    this->ViewRenderObserver = nullptr;
  }

  this->View = renderView;
  if (this->View)
  {
    QObject::connect(this->View, SIGNAL(endRender()), this, SLOT(onRender()));

    this->ViewRenderObserver = vtkViewRenderObserver::New();
    this->View->getProxy()->AddObserver(vtkCommand::StartEvent, this->ViewRenderObserver);

    auto settings = this->getSettingsForView(this->View);
    settings->UpdateVTKObjects();

    proxyWidget = new pqProxyWidget(settings, this);
    proxyWidget->setApplyChangesImmediately(true);
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(this->widget()->layout());
    layout->insertWidget(4, proxyWidget);
    QObject::connect(proxyWidget, SIGNAL(changeFinished()), this->View, SLOT(tryRender()));
  }
}

void pqLookingGlassDockPanel::onRender()
{
  bool interactive = true;
  if (this->ViewRenderObserver)
  {
    vtkViewRenderObserver* observer =
      dynamic_cast<vtkViewRenderObserver*>(this->ViewRenderObserver);
    interactive = observer->InteractiveRender;
  }

  auto settings = this->getSettingsForView(this->View);

  int renderRate = vtkSMPropertyHelper(settings, "RenderRate").GetAsInt();

  if (!this->RenderNextFrame && (renderRate == 2 || (interactive && renderRate == 1)))
  {
    return;
  }

  auto srcWin = vtkOpenGLRenderWindow::SafeDownCast(this->getRenderWindow());

  if (this->LastRenderView && this->LastRenderView != this->View && this->DisplayWindow)
  {
    // Tear down the Looking Glass window so it can be recreated for the new
    // view. Probably a more efficient way to do this.
    this->freeDisplayWindowResources();
  }

  QOpenGLContext* ctx = QOpenGLContext::currentContext();

  if (!this->DisplayWindow)
  {
    this->Interface = vtkLookingGlassInterface::New();
    this->Interface->Initialize();

    srcWin->MakeCurrent();

    this->Widget = new QVTKOpenGLWindow(ctx);
    this->Widget->setFormat(QVTKOpenGLWindow::defaultFormat());
    // Needed in case the window picks up a non-unit device pixel ratio on the host.
    this->Widget->setCustomDevicePixelRatio(1);

    // we have to setup a close on main window close otherwise
    // qt will still think there is a toplevel window open and
    // will not close or destroy this view.
    pqMainWindowEventManager* mainWindowEventManager =
      pqApplicationCore::instance()->getMainWindowEventManager();
    QObject::connect(
      mainWindowEventManager, SIGNAL(close(QCloseEvent*)), this->Widget, SLOT(close()));

    this->DisplayWindow = vtkGenericOpenGLRenderWindow::New();
    this->Widget->setRenderWindow(this->DisplayWindow);
    this->DisplayWindow->GetState()->SetVBOCache(srcWin->GetVBOCache());

    int size[2];
    this->Interface->GetDisplaySize(size);
    int pos[2];
    this->Interface->GetDisplayPosition(pos);

    this->Widget->setFlags(Qt::FramelessWindowHint);
    this->Widget->setPosition(pos[0], pos[1]);
    this->Widget->resize(QSize(size[0], size[1]));
    this->Widget->show();

    this->CopyTexture = vtkTextureObject::New();
    this->CopyTexture->SetContext(this->DisplayWindow);

    auto* endObserver = vtkEndRenderObserver::New();
    endObserver->Interface = this->Interface;
    endObserver->RenderWindow = this->DisplayWindow;
    endObserver->CopyTexture = this->CopyTexture;
    this->EndObserver = endObserver;
    this->DisplayWindow->AddObserver(vtkCommand::RenderEvent, this->EndObserver);
  }

  vtkCollectionSimpleIterator rsit;

  // loop over the tiles, render,and blit
  vtkOpenGLFramebufferObject* renderFramebuffer;
  vtkOpenGLFramebufferObject* quiltFramebuffer;
  this->Interface->GetFramebuffers(srcWin, renderFramebuffer, quiltFramebuffer);
  auto ostate = srcWin->GetState();
  ostate->PushFramebufferBindings();
  renderFramebuffer->Bind(GL_READ_FRAMEBUFFER);

  // default to our standard alpha blend eqn, some vtk classes rely on this
  // and do not set it themselves
  ostate->vtkglEnable(GL_BLEND);
  ostate->vtkglBlendFuncSeparate(
    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  int renderSize[2];
  this->Interface->GetRenderSize(renderSize);

  int tcount = this->Interface->GetNumberOfTiles();

  // save the original camera settings
  vtkRenderer* aren;
  std::vector<vtkCamera*> Cameras;
  auto* renderers = srcWin->GetRenderers();
  for (renderers->InitTraversal(rsit); (aren = renderers->GetNextRenderer(rsit));)
  {
    auto oldCam = aren->GetActiveCamera();
    oldCam->SetLeftEye(1);
    oldCam->Register(this->DisplayWindow);
    Cameras.push_back(oldCam);
    vtkNew<vtkCamera> newCam;
    aren->SetActiveCamera(newCam);
  }

  std::vector<double> clippingLimits =
    vtkSMPropertyHelper(settings, "ClippingLimits").GetArray<double>();
  double nearClippingLimit = clippingLimits[0];
  double farClippingLimit = clippingLimits[1];

  // save the current size and temporarily set the new size to the render
  // framebuffer size
  int origSize[2];
  int* renWinSize = this->getRenderWindow()->GetSize();
  origSize[0] = renWinSize[0];
  origSize[1] = renWinSize[1];
  srcWin->UseOffScreenBuffersOn();
  srcWin->SetSize(renderSize[0], renderSize[1]);

  // loop over all the tiles and render then and blit them to the quilt
  for (int tile = 0; tile < tcount; ++tile)
  {
    renderFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    ostate->vtkglViewport(0, 0, renderSize[0], renderSize[1]);
    ostate->vtkglScissor(0, 0, renderSize[0], renderSize[1]);

    {
      int count = 0;
      for (renderers->InitTraversal(rsit); (aren = renderers->GetNextRenderer(rsit)); ++count)
      {
        // adjust camera
        auto cam = aren->GetActiveCamera();
        cam->DeepCopy(Cameras[count]);
        this->Interface->AdjustCamera(cam, tile);

        // limit the clipping range to limit parallex
        double* cRange = cam->GetClippingRange();
        double cameraDistance = cam->GetDistance();

        double newRange[2];
        newRange[0] = cRange[0];
        newRange[1] = cRange[1];
        if (cRange[0] < cameraDistance * nearClippingLimit)
        {
          newRange[0] = cameraDistance * nearClippingLimit;
        }
        if (cRange[1] > cameraDistance * farClippingLimit)
        {
          newRange[1] = cameraDistance * farClippingLimit;
        }
        cam->SetClippingRange(newRange);
      }
      renderers->Render();
    }

    quiltFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);

    int destPos[2];
    this->Interface->GetTilePosition(tile, destPos);

    // blit to quilt
    ostate->vtkglViewport(destPos[0], destPos[1], renderSize[0], renderSize[1]);
    ostate->vtkglScissor(destPos[0], destPos[1], renderSize[0], renderSize[1]);

    QOpenGLExtraFunctions* f = ctx->extraFunctions();
    if (!f)
    {
      qCritical("required glBlitFramebuffer call not available");
      break;
    }
    f->glBlitFramebuffer(0, 0, renderSize[0], renderSize[1], destPos[0], destPos[1],
      destPos[0] + renderSize[0], destPos[1] + renderSize[1], GL_COLOR_BUFFER_BIT, GL_LINEAR);
  }

  // restore the original size
  srcWin->SetSize(origSize[0], origSize[1]);
  srcWin->UseOffScreenBuffersOff();

  ostate->PopFramebufferBindings();

  // restore the original camera settings
  int count = 0;
  for (renderers->InitTraversal(rsit); (aren = renderers->GetNextRenderer(rsit)); ++count)
  {
    aren->SetActiveCamera(Cameras[count]);
    Cameras[count]->Delete();
  }

  // finally render. The callback will actually do the fullscreen quad
  // in the middle of this call
  this->DisplayWindow->Render();

  this->RenderNextFrame = false;

  this->LastRenderView = this->View;
}

void pqLookingGlassDockPanel::onRenderOnLookingGlassClicked()
{
  auto view = pqActiveObjects::instance().activeView();

  // If we don't have LG settings for this view yet, reset the focal plane to the center of rotation
  QString settingsName = this->getSettingsProxyName(view);

  // See if we have a settings proxy yet
  auto pxm = view->getProxy()->GetSession()->GetSessionProxyManager();
  bool firstRender = pxm->GetProxy("looking_glass", qPrintable(settingsName)) == nullptr;

  this->setView(view);
  this->RenderNextFrame = true;

  if (firstRender)
  {
    this->resetToCenterOfRotation();
  }

  if (this->View)
  {
    this->View->forceRender();
  }
}

void pqLookingGlassDockPanel::resetToCenterOfRotation()
{
  if (!this->View)
  {
    return;
  }
  std::vector<double> cor =
    vtkSMPropertyHelper(this->View->getProxy(), "CenterOfRotation").GetDoubleArray();
  std::vector<double> fp =
    vtkSMPropertyHelper(this->View->getProxy(), "CameraFocalPointInfo").GetDoubleArray();
  vtkSMPropertyHelper(this->View->getProxy(), "CameraFocalPoint").Set(&cor[0], 3);
  std::vector<double> pos =
    vtkSMPropertyHelper(this->View->getProxy(), "CameraPositionInfo").GetDoubleArray();
  vtkSMPropertyHelper position(this->View->getProxy(), "CameraPosition");
  position.Set(0, pos[0] - fp[0] + cor[0]);
  position.Set(1, pos[1] - fp[1] + cor[1]);
  position.Set(2, pos[2] - fp[2] + cor[2]);
  this->View->getProxy()->UpdateVTKObjects();
  this->View->render();
}

void pqLookingGlassDockPanel::pushFocalPlaneBack()
{
  if (!this->View)
  {
    return;
  }

  auto viewProxy = this->View->getProxy();

  // limit the clipping range to limit parallex
  auto settings = this->getSettingsForView(this->View);

  std::vector<double> fp = vtkSMPropertyHelper(viewProxy, "CameraFocalPointInfo").GetDoubleArray();
  std::vector<double> pos = vtkSMPropertyHelper(viewProxy, "CameraPositionInfo").GetDoubleArray();

  double dx = fp[0] - pos[0];
  double dy = fp[1] - pos[1];
  double dz = fp[2] - pos[2];
  double distance = sqrt(dx * dx + dy * dy + dz * dz);

  double directionOfProjection[3];
  directionOfProjection[0] = dx / distance;
  directionOfProjection[1] = dy / distance;
  directionOfProjection[2] = dz / distance;

  double focalPlaneMovementFactor =
    vtkSMPropertyHelper(settings, "FocalPlaneMovementFactor").GetAsDouble();
  std::vector<double> clippingLimits =
    vtkSMPropertyHelper(settings, "ClippingLimits").GetArray<double>();
  double farClippingLimit = clippingLimits[1];

  distance += focalPlaneMovementFactor * distance * (farClippingLimit - 1.0);

  fp[0] = pos[0] + directionOfProjection[0] * distance;
  fp[1] = pos[1] + directionOfProjection[1] * distance;
  fp[2] = pos[2] + directionOfProjection[2] * distance;
  vtkSMPropertyHelper(viewProxy, "CameraFocalPoint").Set(&fp[0], 3);

  // vtkSMPropertyHelper(this->View->getProxy(), "CameraDistance").Set(cameraDistance +
  //  focalPlaneMovementFactor * cameraDistance * (farClippingLimit - 1.0));
  viewProxy->UpdateVTKObjects();
  this->View->render();
}

void pqLookingGlassDockPanel::pullFocalPlaneForward()
{
  if (!this->View)
  {
    return;
  }

  auto viewProxy = this->View->getProxy();

  // limit the clipping range to limit parallex
  auto settings = this->getSettingsForView(this->View);

  std::vector<double> fp = vtkSMPropertyHelper(viewProxy, "CameraFocalPointInfo").GetDoubleArray();
  std::vector<double> pos = vtkSMPropertyHelper(viewProxy, "CameraPositionInfo").GetDoubleArray();

  double dx = fp[0] - pos[0];
  double dy = fp[1] - pos[1];
  double dz = fp[2] - pos[2];
  double distance = sqrt(dx * dx + dy * dy + dz * dz);

  double directionOfProjection[3];
  directionOfProjection[0] = dx / distance;
  directionOfProjection[1] = dy / distance;
  directionOfProjection[2] = dz / distance;

  double focalPlaneMovementFactor =
    vtkSMPropertyHelper(settings, "FocalPlaneMovementFactor").GetAsDouble();
  std::vector<double> clippingLimits =
    vtkSMPropertyHelper(settings, "ClippingLimits").GetArray<double>();
  double nearClippingLimit = clippingLimits[0];

  distance -= focalPlaneMovementFactor * distance * (1.0 - nearClippingLimit);

  fp[0] = pos[0] + directionOfProjection[0] * distance;
  fp[1] = pos[1] + directionOfProjection[1] * distance;
  fp[2] = pos[2] + directionOfProjection[2] * distance;
  vtkSMPropertyHelper(viewProxy, "CameraFocalPoint").Set(&fp[0], 3);

  // vtkSMPropertyHelper(this->View->getProxy(), "CameraDistance").Set(cameraDistance -
  //  focalPlaneMovementFactor * cameraDistance * (1.0 - nearClippingLimit));
  viewProxy->UpdateVTKObjects();
  this->View->render();
}

vtkSMProxy* pqLookingGlassDockPanel::getActiveCamera()
{
  if (!this->View)
  {
    return nullptr;
  }

  return vtkSMPropertyHelper(this->View->getProxy(), "ActiveCamera").GetAsProxy();
}

vtkRenderWindow* pqLookingGlassDockPanel::getRenderWindow()
{
  if (!this->View)
  {
    return nullptr;
  }

  auto viewProxy = this->View->getRenderViewProxy();
  if (!viewProxy)
  {
    qCritical() << "No view proxy available";
    return nullptr;
  }

  auto view = viewProxy->GetClientSideObject();
  auto renderView = vtkPVRenderView::SafeDownCast(view);
  if (!renderView)
  {
    qCritical() << "RenderView not available";
    return nullptr;
  }

  return renderView->GetRenderWindow();
}

vtkSMProxy* pqLookingGlassDockPanel::getSettingsForView(pqRenderView* view)
{
  if (!view)
  {
    return nullptr;
  }

  QString settingsName = this->getSettingsProxyName(view);

  // See if we have a settings proxy yet
  auto pxm = view->getProxy()->GetSession()->GetSessionProxyManager();
  auto settings = pxm->GetProxy("looking_glass", qPrintable(settingsName));
  if (!settings)
  {
    // Create a Looking Glass settings proxy for this view
    vtkSmartPointer<vtkSMProxy> newSettings;
    newSettings.TakeReference(pxm->NewProxy("looking_glass", "LookingGlassSettings"));
    settings = newSettings;

    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->PreInitializeProxy(settings);
    vtkSMPropertyHelper(settings, "View").Set(view->getProxy());
    controller->PostInitializeProxy(settings);
    pxm->RegisterProxy("looking_glass", qPrintable(settingsName), settings);

    // Set up a connection to remove the settings when the associated view is deleted
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    QObject::connect(smmodel, SIGNAL(preViewRemoved(pqView*)), this, SLOT(viewRemoved(pqView*)));
  }

  return settings;
}

QString pqLookingGlassDockPanel::getSettingsProxyName(pqView* view)
{
  QString settingsName = view->getSMName();
  settingsName += "-LookingGlassSettings";

  return settingsName;
}

void pqLookingGlassDockPanel::reset()
{
  this->setView(nullptr);
  this->LastRenderView = nullptr;
  this->freeDisplayWindowResources();
}

void pqLookingGlassDockPanel::viewRemoved(pqView* view)
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(view);
  if (!renderView)
  {
    return;
  }

  // Unregister the settings for this view
  auto settings = this->getSettingsForView(renderView);
  if (settings)
  {
    auto pxm = view->getProxy()->GetSession()->GetSessionProxyManager();
    pxm->UnRegisterProxy(settings);
  }

  this->freeDisplayWindowResources();
}

void pqLookingGlassDockPanel::activeViewChanged(pqView* view)
{
  if (!view)
  {
    return;
  }
  bool enabled = strcmp(view->getProxy()->GetXMLName(), "RenderView") == 0;
  this->widget()->setEnabled(enabled);

  // TODO - this could be cleaned up some more in terms of which widgets are enabled disabled
}

void pqLookingGlassDockPanel::freeDisplayWindowResources()
{
  if (this->Interface)
  {
    this->Interface->ReleaseGraphicsResources(this->getRenderWindow());
    this->Interface->Delete();
    this->Interface = nullptr;
  }

  if (this->DisplayWindow)
  {
    this->DisplayWindow->RemoveObserver(this->EndObserver);
    this->EndObserver->Delete();
    this->DisplayWindow->Delete();
    this->DisplayWindow = nullptr;
  }
  if (this->CopyTexture)
  {
    this->CopyTexture->Delete();
    this->CopyTexture = nullptr;
  }
  if (this->Widget)
  {
    this->Widget->destroy();
    delete this->Widget;
    this->Widget = nullptr;
  }
}
