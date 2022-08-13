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
#include "pqMainWindowEventManager.h"
#include "pqProxyWidget.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include "QVTKOpenGLWindow.h"
#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QSignalBlocker>

#include "vtkCollection.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkLookingGlassInterface.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLState.h"
#include "vtkPVRenderView.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkTextureObject.h"

#include <array>
#include <cmath>

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

class vtkFocalDistanceObserver : public vtkCommand
{
public:
  static vtkFocalDistanceObserver* New() { return new vtkFocalDistanceObserver; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event),
    void* vtkNotUsed(callData)) override
  {
    panel->resetFocalDistanceSliderRange();
  }

  pqLookingGlassDockPanel* panel = nullptr;
};

} // end anonymous namespace

class pqLookingGlassDockPanel::pqInternal
{
public:
  Ui::pqLookingGlassDockPanel Ui;
  std::array<double, 2> FocalDistanceSliderRange;
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
  this->connect(ui.SaveQuilt, SIGNAL(clicked(bool)), SLOT(saveQuilt()));
  this->connect(ui.RecordQuilt, SIGNAL(clicked(bool)), SLOT(onRecordQuiltClicked()));
  this->connect(ui.FocalDistance, SIGNAL(valueEdited(double)), SLOT(onFocalDistanceEdited(double)));

  this->connect(activeObjects, SIGNAL(serverChanged(pqServer*)), SLOT(reset()));

  // Disable button if active view is not compatible with LG
  this->connect(activeObjects, SIGNAL(viewChanged(pqView*)), SLOT(activeViewChanged(pqView*)));

  this->updateEnableStates();

  // Populate target device dropdown
  auto devices = vtkLookingGlassInterface::GetDevices();
  for (auto device : devices)
  {
    ui.TargetDeviceComboBox->addItem(
      QString::fromStdString(device.second), QString::fromStdString(device.first));
  }
  ui.TargetDeviceComboBox->setCurrentIndex(-1);

  this->connect(ui.TargetDeviceComboBox,
    static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
    &pqLookingGlassDockPanel::onTargetDeviceChanged);

  ui.FocalDistance->setResolution(100);
  resetFocalDistanceSliderRange();
}

pqLookingGlassDockPanel::~pqLookingGlassDockPanel()
{
  if (this->IsRecording)
  {
    this->stopRecordingQuilt();
  }

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
    this->View->getProxy()
      ->GetProperty("CameraFocalPoint")
      ->RemoveObserver(this->FocalDistanceObserver);
    this->View->getProxy()
      ->GetProperty("CameraPosition")
      ->RemoveObserver(this->FocalDistanceObserver);
    this->FocalDistanceObserver->Delete();
    this->FocalDistanceObserver = nullptr;
  }

  this->View = renderView;
  if (this->View)
  {
    QObject::connect(this->View, SIGNAL(endRender()), this, SLOT(onRender()));

    this->ViewRenderObserver = vtkViewRenderObserver::New();
    this->View->getProxy()->AddObserver(vtkCommand::StartEvent, this->ViewRenderObserver);

    auto* focalDistanceObserver = vtkFocalDistanceObserver::New();
    focalDistanceObserver->panel = this;
    this->FocalDistanceObserver = focalDistanceObserver;

    // When a property is modified that may result in the focal distance
    // changing, reset the focal distance slider.
    this->View->getProxy()
      ->GetProperty("CameraFocalPoint")
      ->AddObserver(vtkCommand::ModifiedEvent, this->FocalDistanceObserver);
    this->View->getProxy()
      ->GetProperty("CameraPosition")
      ->AddObserver(vtkCommand::ModifiedEvent, this->FocalDistanceObserver);

    auto settings = this->getSettingsForView(this->View);
    settings->UpdateVTKObjects();

    proxyWidget = new pqProxyWidget(settings, this);
    proxyWidget->setApplyChangesImmediately(true);
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(this->widget()->layout());
    layout->insertWidget(2, proxyWidget);

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
    this->Interface->SetDeviceType(
      this->Internal->Ui.TargetDeviceComboBox->currentData().toString().toStdString());
    this->Interface->Initialize();

    // Update the combo box to default to the attached device
    auto deviceType = this->Interface->GetDeviceType();
    this->setAttachedDevice(deviceType);

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

    this->updateEnableStates();
  }

  int renderSize[2];
  this->Interface->GetRenderSize(renderSize);

  std::vector<double> clippingLimits =
    vtkSMPropertyHelper(settings, "ClippingLimits").GetArray<double>();

  this->Interface->SetUseClippingLimits(true);
  this->Interface->SetNearClippingLimit(clippingLimits[0]);
  this->Interface->SetFarClippingLimit(clippingLimits[1]);

  // save the current size and temporarily set the new size to the render
  // framebuffer size
  int origSize[2];
  int* renWinSize = this->getRenderWindow()->GetSize();
  origSize[0] = renWinSize[0];
  origSize[1] = renWinSize[1];
  srcWin->UseOffScreenBuffersOn();
  srcWin->SetSize(renderSize[0], renderSize[1]);

  this->Interface->RenderQuilt(srcWin);

  // restore the original size
  srcWin->SetSize(origSize[0], origSize[1]);
  srcWin->UseOffScreenBuffersOff();

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
  bool firstRender = pxm->GetProxy("looking_glass", settingsName.toUtf8().data()) == nullptr;

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

  double directionOfProjection[3];
  double distance = this->computeFocalDistanceAndDirection(directionOfProjection);

  auto viewProxy = this->View->getProxy();

  // limit the clipping range to limit parallex
  auto settings = this->getSettingsForView(this->View);

  std::vector<double> fp = vtkSMPropertyHelper(viewProxy, "CameraFocalPointInfo").GetDoubleArray();
  std::vector<double> pos = vtkSMPropertyHelper(viewProxy, "CameraPositionInfo").GetDoubleArray();

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

  double directionOfProjection[3];
  double distance = this->computeFocalDistanceAndDirection(directionOfProjection);

  auto viewProxy = this->View->getProxy();

  // limit the clipping range to limit parallex
  auto settings = this->getSettingsForView(this->View);

  std::vector<double> fp = vtkSMPropertyHelper(viewProxy, "CameraFocalPointInfo").GetDoubleArray();
  std::vector<double> pos = vtkSMPropertyHelper(viewProxy, "CameraPositionInfo").GetDoubleArray();

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

void pqLookingGlassDockPanel::updateEnableStates()
{
  bool visible = this->Interface && this->DisplayWindow;

  auto& ui = this->Internal->Ui;
  ui.SaveQuilt->setEnabled(visible);
  ui.RecordQuilt->setEnabled(visible);
  ui.ResetToCenterOfRotationButton->setEnabled(visible);
  ui.PushFocalPlaneBackButton->setEnabled(visible);
  ui.PullFocalPlaneForwardButton->setEnabled(visible);
  ui.TargetDeviceComboBox->setEnabled(visible);
  ui.TargetDeviceLabel->setEnabled(visible);
  ui.FocalDistanceLabel->setEnabled(visible);
  ui.FocalDistance->setEnabled(visible);
}

QString pqLookingGlassDockPanel::getQuiltFileSuffix()
{
  int tiles[2];
  this->Interface->GetQuiltTiles(tiles);

  return QString("_qs%1x%2").arg(tiles[0]).arg(tiles[1]);
}

void pqLookingGlassDockPanel::saveQuilt()
{
  // Don't confirm overwrite, since we will be checking that later, after
  // we ensure the right suffix is attached...
  QString extension = ".png";
  auto filepath = QFileDialog::getSaveFileName(this, "Save Quilt Image", "",
    QString("Images (*%1)").arg(extension), nullptr, QFileDialog::DontConfirmOverwrite);
  if (filepath.isEmpty())
  {
    // User canceled
    return;
  }

  auto suffix = this->getQuiltFileSuffix() + extension;
  if (!filepath.endsWith(suffix))
  {
    // We will add the suffix
    if (filepath.endsWith(extension))
    {
      // Remove the extension, if it exists
      filepath.chop(extension.size());
    }
    // Add the suffix
    filepath += suffix;
  }

  if (QFile(filepath).exists())
  {
    auto title = QString("Overwrite file?");
    auto text = QString("\"%1\" already exists.\n\n"
                        "Would you like to overwrite it?")
                  .arg(filepath);
    if (QMessageBox::question(this, title, text) == QMessageBox::No)
    {
      // User does not want to over-write the file...
      return;
    }
  }

  // Update the interface with the GUI values
  this->Interface->SaveQuilt(filepath.toUtf8().data());

  auto text = QString("Saved to \"%1\"").arg(filepath);
  QMessageBox::information(this, "Quilt Saved", filepath);
}

void pqLookingGlassDockPanel::onRecordQuiltClicked()
{
  auto& ui = this->Internal->Ui;

  if (!this->IsRecording)
  {
    this->startRecordingQuilt();
  }
  else
  {
    this->stopRecordingQuilt();
  }

  // Update the text in a separate logic block, so we can see
  // if the recording state actually changed.
  if (this->IsRecording)
  {
    ui.RecordQuilt->setText("Stop Recording Quilt");
  }
  else
  {
    ui.RecordQuilt->setText("Record Quilt");
  }
}

double pqLookingGlassDockPanel::computeFocalDistanceAndDirection(double directionOfProjection[3])
{
  if (!this->View)
  {
    return -1;
  }

  auto viewProxy = this->View->getProxy();
  std::vector<double> fp = vtkSMPropertyHelper(viewProxy, "CameraFocalPoint").GetDoubleArray();
  std::vector<double> pos = vtkSMPropertyHelper(viewProxy, "CameraPosition").GetDoubleArray();

  double dx = fp[0] - pos[0];
  double dy = fp[1] - pos[1];
  double dz = fp[2] - pos[2];
  double distance = sqrt(dx * dx + dy * dy + dz * dz);

  directionOfProjection[0] = dx / distance;
  directionOfProjection[1] = dy / distance;
  directionOfProjection[2] = dz / distance;

  return distance;
}

void pqLookingGlassDockPanel::onFocalDistanceEdited(double distance)
{
  if (!this->View)
  {
    return;
  }

  auto viewProxy = this->View->getProxy();

  std::vector<double> fp = vtkSMPropertyHelper(viewProxy, "CameraFocalPointInfo").GetDoubleArray();
  std::vector<double> pos = vtkSMPropertyHelper(viewProxy, "CameraPositionInfo").GetDoubleArray();

  double directionOfProjection[3];
  double oldDistance = this->computeFocalDistanceAndDirection(directionOfProjection);

  fp[0] = pos[0] + directionOfProjection[0] * distance;
  fp[1] = pos[1] + directionOfProjection[1] * distance;
  fp[2] = pos[2] + directionOfProjection[2] * distance;
  vtkSMPropertyHelper(viewProxy, "CameraFocalPoint").Set(&fp[0], 3);

  viewProxy->UpdateVTKObjects();
  this->View->render();
}

void pqLookingGlassDockPanel::resetFocalDistanceSliderRange()
{
  if (!this->View)
  {
    return;
  }

  auto& ui = this->Internal->Ui;
  if (this->sender() == ui.FocalDistance)
  {
    // If this was caused by the focal distance slider being edited, ignore it
    return;
  }

  double directionOfProjection[3];
  double distance = this->computeFocalDistanceAndDirection(directionOfProjection);

  // The slider will move 1/4 backward/forward
  this->Internal->FocalDistanceSliderRange[0] = distance * 3 / 4;
  this->Internal->FocalDistanceSliderRange[1] = distance * 5 / 4;

  QSignalBlocker blocked(ui.FocalDistance);

  ui.FocalDistance->setMinimum(this->Internal->FocalDistanceSliderRange[0]);
  ui.FocalDistance->setMaximum(this->Internal->FocalDistanceSliderRange[1]);
  ui.FocalDistance->setValue(distance);
}

void pqLookingGlassDockPanel::onTargetDeviceChanged(int index)
{
  // Check if we are currently rendering to a device
  if (this->Interface != nullptr)
  {
    // Reset
    this->reset();

    // Restart
    onRenderOnLookingGlassClicked();
  }
}

void pqLookingGlassDockPanel::setAttachedDevice(const std::string& deviceType)
{
  auto& ui = this->Internal->Ui;

  // Only set if the user hasn't made a selection
  if (!ui.TargetDeviceComboBox->currentData().isValid())
  {

    // Set the selection in the combo box
    if (!deviceType.empty())
    {
      auto index = ui.TargetDeviceComboBox->findData(QString::fromStdString(deviceType));
      if (index < 0)
      {
        qWarning() << "Unrecognized device type:" << QString::fromStdString(deviceType);
      }
      else
      {
        // Block the signal so we don't trigger currentIndexChanged
        ui.TargetDeviceComboBox->blockSignals(true);
        ui.TargetDeviceComboBox->setCurrentIndex(index);
        ui.TargetDeviceComboBox->blockSignals(false);
      }
    }
  }
}

void pqLookingGlassDockPanel::startRecordingQuilt()
{
  if (!this->Interface || !this->DisplayWindow || this->IsRecording)
  {
    return;
  }

  auto extension = QString(".%1").arg(this->Interface->MovieFileExtension());

  // Don't confirm overwrite, since we will be checking that later, after
  // we ensure the right suffix is attached...
  auto filepath = QFileDialog::getSaveFileName(this, "Save Quilt Movie", "",
    QString("Movies (*%1)").arg(extension), nullptr, QFileDialog::DontConfirmOverwrite);
  if (filepath.isEmpty())
  {
    // User canceled
    return;
  }

  auto suffix = this->getQuiltFileSuffix() + extension;
  if (!filepath.endsWith(suffix))
  {
    // We will add the suffix
    if (filepath.endsWith(extension))
    {
      // Remove the extension, if it exists
      filepath.chop(extension.size());
    }
    // Add the suffix
    filepath += suffix;
  }

  if (QFile(filepath).exists())
  {
    auto title = QString("Overwrite file?");
    auto text = QString("\"%1\" already exists.\n\n"
                        "Would you like to overwrite it?")
                  .arg(filepath);
    if (QMessageBox::question(this, title, text) == QMessageBox::No)
    {
      // User does not want to over-write the file...
      return;
    }
  }
  // Update the interface with the GUI values
  auto& ui = this->Internal->Ui;
  ui.TargetDeviceLabel->setEnabled(false);
  ui.TargetDeviceComboBox->setEnabled(false);

  this->Interface->StartRecordingQuilt(filepath.toUtf8().data());
  this->IsRecording = true;
  this->MovieFilepath = filepath;

  // Record the first frame...
  onRender();
}

void pqLookingGlassDockPanel::stopRecordingQuilt()
{
  if (!this->Interface || !this->IsRecording)
  {
    return;
  }

  auto& ui = this->Internal->Ui;
  ui.TargetDeviceLabel->setEnabled(true);
  ui.TargetDeviceComboBox->setEnabled(true);

  this->Interface->StopRecordingQuilt();
  this->IsRecording = false;
  auto filepath = this->MovieFilepath;
  this->MovieFilepath.clear();

  auto text = QString("Saved to \"%1\"").arg(filepath);
  QMessageBox::information(this, "Quilt Saved", text);
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
  auto settings = pxm->GetProxy("looking_glass", settingsName.toUtf8().data());
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
    pxm->RegisterProxy("looking_glass", settingsName.toUtf8().data(), settings);

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
